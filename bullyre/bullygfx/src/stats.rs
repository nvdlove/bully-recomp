//! Call counts per watched method and per caller, written to `bullygfx_stats.txt`
//! next to the game every few seconds.

use std::collections::HashMap;
use std::fmt::Write;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Mutex, OnceLock};
use std::time::{Duration, Instant};

use crate::hook::{DEVICE_METHODS, RENDERER_METHODS};
use crate::names::{DEVICE, renderer};
use crate::control::{Action, Control};
use crate::replay::Toggle;
use crate::{rtti, scene};

thread_local! {
    /// The replay renderer and the control script live on the game's render thread.
    static NEW_RENDERER: std::cell::RefCell<Toggle> = const { std::cell::RefCell::new(Toggle::new()) };
    static CONTROL: std::cell::RefCell<Control> = const { std::cell::RefCell::new(Control::new()) };
}
static OLD_SHOT: Mutex<Option<std::path::PathBuf>> = Mutex::new(None);

pub const RENDERER_BASE: u32 = 0;
pub const DEVICE_BASE: u32 = 128;
const IDS: usize = 256;
const PRESENT: u32 = DEVICE_BASE + 17;
const DUMP_EVERY: Duration = Duration::from_secs(5);
/// The exe the menu launches: only that one is watched.
/// The game's exes: the original (32-bit) and bullyre's 64-bit build.
const WATCHED_EXES: [&str; 2] = ["bully_unpacked.exe", "bullyre_x64.exe"];

static CALLS: [AtomicU64; IDS] = [const { AtomicU64::new(0) }; IDS];
static FRAMES: AtomicU64 = AtomicU64::new(0);
/// Frame times since the last report: over 25, 50 and 100 ms, and the longest (us).
static HITCHES: [AtomicU64; 3] = [AtomicU64::new(0), AtomicU64::new(0), AtomicU64::new(0)];
static LONGEST: AtomicU64 = AtomicU64::new(0);
static LAST_PRESENT: Mutex<Option<Instant>> = Mutex::new(None);
/// Calls looked at closely: all of the first `SAMPLE_ALL`, then 1 in `SAMPLE_EVERY`.
static SAMPLED: [AtomicU64; IDS] = [const { AtomicU64::new(0) }; IDS];
const SAMPLE_ALL: u64 = 4000;
const SAMPLE_EVERY: u64 = 512;
/// How many argument patterns to keep per method.
const PATTERNS: usize = 8;
const ARGS: usize = 4;

struct Log {
    callers: HashMap<(u32, u32), u64>, // (id, return address) -> calls
    // id -> (this and argument kinds, calls, one example in detail)
    args: HashMap<u32, Vec<(String, u64, String)>>,
    notes: Vec<String>,
}

fn log() -> &'static Mutex<Log> {
    static LOG: OnceLock<Mutex<Log>> = OnceLock::new();
    LOG.get_or_init(|| Mutex::new(Log { callers: HashMap::new(), args: HashMap::new(), notes: Vec::new() }))
}

pub fn enabled() -> bool {
    crate::win::exe_path().file_name().is_some_and(|n| WATCHED_EXES.iter().any(|w| n.to_string_lossy().eq_ignore_ascii_case(w)))
}

pub fn note(s: &str) {
    if let Ok(mut l) = log().lock() {
        l.notes.push(s.to_string());
    }
}

const DEV_RESET: u32 = 16;

/// The per-call diagnostics (callers, argument kinds, the old mesh-level scene): off
/// unless asked for (`diag on`), as they cost on every one of ~6000 calls a frame.
pub static DIAGNOSTICS: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

static AFTER_RESET: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

/// Testing: log the next N SetCameraData calls.
pub static CAMERA_PROBE: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);

/// The real device behind our wrapper (hook.rs); the wrapper is ours, always there.
fn real_device(wrapper: u32) -> u32 {
    crate::win::obj32(crate::hook::real_device_of(wrapper))
}
const DEV_CREATE_VERTEX_BUFFER: u32 = 26;
const DEV_CREATE_INDEX_BUFFER: u32 = 27;

/// A buffer the game would make write-only in video memory made readable instead:
/// static ones MANAGED (a system memory copy), dynamic ones without WRITEONLY.
fn readable_buffer(usage: &mut u32, pool: &mut u32) {
    const D3DUSAGE_WRITEONLY: u32 = 0x8;
    const D3DUSAGE_DYNAMIC: u32 = 0x200;
    const D3DPOOL_DEFAULT: u32 = 0;
    const D3DPOOL_MANAGED: u32 = 1;
    let _ = (D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, D3DPOOL_MANAGED, &pool);
    *usage &= !D3DUSAGE_WRITEONLY;
}

/// The x64 thunks' entry (hook.rs). `regs` is the thunk's rbp: [0] the caller's rbp, [1]
/// the return address, [2..6] rcx, rdx, r8, r9 (in their home slots), [6..] the stack
/// arguments. `record` gets the 32-bit thunks' frame made of them (renderer methods: `this`
/// where pushad keeps ecx, the arguments from 9; device methods: `this` at 9, then the
/// arguments), and what it changes there (a buffer's usage, a draw's count) goes back to the
/// register or stack slot it came from. The values are the game's, so they fit 32 bits.
#[cfg(target_arch = "x86_64")]
pub extern "C" fn record64(id: u32, regs: *mut u64) {
    const SLOTS: usize = 9 + 4 + 12;
    let reg = |i: usize| unsafe { *regs.add(i) };
    let mut frame = [0u32; SLOTS];
    let mut from = [usize::MAX; SLOTS];
    frame[8] = reg(1) as u32;
    let first = if id < DEVICE_BASE {
        frame[6] = reg(2) as u32;
        from[6] = 2;
        9
    } else {
        frame[9] = reg(2) as u32;
        from[9] = 2;
        10
    };
    for k in 0..3 + 12 {
        let i = first + k;
        if i < SLOTS {
            frame[i] = reg(3 + k) as u32;
            from[i] = 3 + k;
        }
    }
    let before = frame;
    record(id, frame.as_mut_ptr());
    for i in 0..SLOTS {
        if frame[i] != before[i] && from[i] != usize::MAX {
            unsafe { *regs.add(from[i]) = frame[i] as u64 };
        }
    }
}

/// Called by every thunk, from whatever thread the game called on. `frame` is what
/// pushad saved (edi, esi, ebp, esp, ebx, edx, ecx, eax), then the return address,
/// then the stack arguments. A panic in here turns the new renderer off rather than
/// taking the game down; D3D9 draws again.
pub extern "C" fn record(id: u32, frame: *mut u32) {
    if std::panic::catch_unwind(|| record_call(id, frame as *const u32)).is_err() {
        crate::capture::ENABLED.store(false, Ordering::SeqCst);
        crate::capture::EXCLUSIVE.store(false, Ordering::SeqCst);
        let _ = NEW_RENDERER.try_with(|t| {
            if let Ok(mut t) = t.try_borrow_mut() {
                t.fail();
            }
        });
        crate::control::log("the new renderer failed (see bullygfx_panic.txt): back to D3D9");
    }
}

fn record_call(id: u32, frame: *const u32) {
    let at = |i: usize| unsafe { *frame.add(i) };
    let ret = at(8);
    if (id as usize) < IDS {
        CALLS[id as usize].fetch_add(1, Ordering::Relaxed);
    }
    if id == PRESENT {
        FRAMES.fetch_add(1, Ordering::Relaxed);
        if let Ok(mut last) = LAST_PRESENT.lock() {
            let now = Instant::now();
            if let Some(t) = last.replace(now) {
                let us = (now - t).as_micros() as u64;
                for (i, limit) in [25_000, 50_000, 100_000].into_iter().enumerate() {
                    if us > limit {
                        HITCHES[i].fetch_add(1, Ordering::Relaxed);
                    }
                }
                LONGEST.fetch_max(us, Ordering::Relaxed);
            }
        }
        // the finished frame is in the back buffer now
        if let Some(path) = OLD_SHOT.lock().ok().and_then(|mut p| p.take()) {
            let device = real_device(at(9));
            let r = unsafe { crate::control::shot_old(device, &path) };
            crate::control::log(&match r {
                Ok(()) => format!("old shot {}", path.display()),
                Err(e) => format!("old shot failed: {e}"),
            });
        }
        NEW_RENDERER.with(|t| t.borrow_mut().present(real_device(at(9))));
    }
    let diagnostics = DIAGNOSTICS.load(Ordering::Relaxed);
    if diagnostics && let Ok(mut l) = log().try_lock() {
        *l.callers.entry((id, ret)).or_insert(0) += 1;
    }
    if id < DEVICE_BASE {
        if id == 86 {
            // SetCameraData(eye, dir, up, right, NiFrustum*, NiRect*): FOV and aspect
            crate::camera::adjust(at(13));
            crate::capture::camera(at(9), at(10), at(11), at(12), at(13));
        }
        if id == 86 && CAMERA_PROBE.load(Ordering::Relaxed) > 0 {
            // SetCameraData(eye, dir, up, right, NiFrustum*, NiRect* port)
            CAMERA_PROBE.fetch_sub(1, Ordering::Relaxed);
            let f = at(13);
            let v: Vec<f32> = (0..7).map(|i| f32::from_bits(crate::win::read_u32(f + 4 * i).unwrap_or(0))).collect();
            let owner = (0..0x200u32).step_by(4).find_map(|k| {
                let vt = crate::win::read_u32(f.wrapping_sub(k))?;
                (vt == 0x9428e4 || vt == 0x951df0).then_some(k)
            });
            let fov = f32::from_bits(crate::win::read_u32(0xC3CD04).unwrap_or(0));
            crate::control::log(&format!("camera: frustum {f:#x} {v:?} NiCamera at -{owner:x?} fov {fov} 16:9 {:?} ret {ret:#x}", crate::win::read_u32(0xC1870C).map(|x| (x >> 16) & 0xFF)));
        }
        if (id == scene::DRAW_MESH || id == scene::DRAW_SKINNED) && crate::capture::ENABLED.load(Ordering::Relaxed) {
            crate::textures::register(at(9));
        }
        if diagnostics {
            scene::renderer_call(id, at(6), &[at(9), at(10), at(11), at(12)]);
        }
        if id == scene::DISPLAY_FRAME {
            let action = CONTROL.with(|c| c.borrow_mut().frame());
            NEW_RENDERER.with(|t| {
                let mut t = t.borrow_mut();
                match action {
                    Action::Renderer(on) => t.set(on),
                    Action::ShotNew(path) => t.request_shot(path),
                    Action::Stage(_) => crate::control::log("stage: not used by the replay renderer"),
                    Action::SkinSwap(_) => crate::control::log("skinswap: not used by the replay renderer"),
                    Action::LightMode(_) => crate::control::log("lightmode: not used by the replay renderer"),
                    Action::ShotOld(path) => *OLD_SHOT.lock().unwrap() = Some(path),
                    Action::ShotBoth(old, new) => {
                        // the new one renders now, the old one at this frame's Present
                        *OLD_SHOT.lock().unwrap() = Some(old);
                        t.request_shot(new);
                    }
                    Action::None => {}
                }
            });
        }
    } else {
        let method = id - DEVICE_BASE;
        if AFTER_RESET.swap(false, Ordering::SeqCst) {
            // the call after a Reset: did it take?
            let device = real_device(at(9));
            type Tcl = unsafe extern "system" fn(usize) -> i32;
            let tcl = unsafe { std::mem::transmute::<usize, Tcl>(*(*(device as *const *const usize)).add(3))(device as usize) };
            crate::control::log(&format!("after reset: device state {:#010x}", tcl as u32));
        }
        if method == DEV_RESET {
            NEW_RENDERER.with(|t| t.borrow_mut().before_reset());
            crate::replay::Toggle::before_reset_check(real_device(at(9)));
            AFTER_RESET.store(true, Ordering::SeqCst);
        }
        if method == DEV_CREATE_VERTEX_BUFFER || method == DEV_CREATE_INDEX_BUFFER {
            // (length, usage, fvf or format, pool, ...): buffers the replay can read back
            let args = unsafe { frame.add(10) as *mut u32 };
            unsafe { readable_buffer(&mut *args.add(1), &mut *args.add(3)) };
        }
        if crate::capture::ENABLED.load(Ordering::Relaxed) {
            let device = real_device(at(9));
            crate::capture::device_call(device, method, &[at(10), at(11), at(12), at(13), at(14), at(15), at(16), at(17)], ret);
            if crate::capture::EXCLUSIVE.load(Ordering::Relaxed)
                && (!crate::capture::SCREEN_ONLY.load(Ordering::Relaxed) || crate::capture::LAST_TO_SCREEN.load(Ordering::Relaxed))
                && let Some(count) = crate::capture::primitive_count_arg(method)
            {
                // the replay draws it: D3D9 draws nothing (0 primitives)
                unsafe { *(frame.add(10 + count) as *mut u32) = 0 };
            }
        }
        if method == crate::shaders::DEV_CREATE_VS || method == crate::shaders::DEV_CREATE_PS {
            crate::shaders::created(method, at(10));
        }
        if diagnostics {
            scene::device_call(method, &[at(10), at(11), at(12), at(13), at(14), at(15)], ret);
        }
    }
    let sampled = diagnostics && (id < DEVICE_BASE || DEVICE_SAMPLED.contains(&(id - DEVICE_BASE)));
    if sampled {
        let n = SAMPLED[id as usize].fetch_add(1, Ordering::Relaxed);
        if n < SAMPLE_ALL || n % SAMPLE_EVERY == 0 {
            // renderer methods are thiscall (`this` in ecx), device methods stdcall
            // (`this` is the first stack argument)
            let (this, args) = if id < DEVICE_BASE {
                (at(6), [at(9), at(10), at(11), at(12)])
            } else {
                (at(9), [at(10), at(11), at(12), at(13)])
            };
            sample_args(id, this, &args);
        }
    }
}

/// Device methods whose arguments are worth seeing: resource creation (pools),
/// transforms, and what a draw uses.
const DEVICE_SAMPLED: [u32; 9] = [23, 26, 27, 44, 82, 86, 87, 94, 100];

fn sample_args(id: u32, this: u32, args: &[u32; ARGS]) {
    let pattern = std::iter::once(rtti::kind(this)).chain(args.iter().map(|&a| rtti::kind(a))).collect::<Vec<_>>().join(", ");
    let Ok(mut l) = log().try_lock() else { return };
    let list = l.args.entry(id).or_default();
    if let Some(p) = list.iter_mut().find(|p| p.0 == pattern) {
        p.1 += 1;
    } else if list.len() < PATTERNS {
        let example = args.iter().map(|&a| rtti::detail(a)).collect::<Vec<_>>().join(", ");
        list.push((pattern, 1, example));
    }
}

pub fn start() {
    let path = crate::win::game_file("bullygfx_stats.txt");
    let started = Instant::now();
    std::thread::spawn(move || {
        loop {
            std::thread::sleep(DUMP_EVERY);
            let _ = std::fs::write(&path, report(started.elapsed()));
        }
    });
}

fn report(elapsed: Duration) -> String {
    let frames = FRAMES.load(Ordering::Relaxed).max(1);
    let mut out = String::new();
    let _ = writeln!(out, "bullygfx stage 1: {frames} frames in {:.0} s", elapsed.as_secs_f32());
    let mb = |n: usize| n as f64 / 1048576.0;
    let _ = writeln!(
        out,
        "memory: bullygfx heap {:.0} MB (peak {:.0} MB), process private {:.0} MB",
        mb(crate::HEAP.load(Ordering::Relaxed)),
        mb(crate::HEAP_PEAK.load(Ordering::Relaxed)),
        mb(crate::win::private_bytes())
    );
    let _ = writeln!(
        out,
        "hitches since the last report: {} frames over 25 ms, {} over 50 ms, {} over 100 ms, longest {:.0} ms",
        HITCHES[0].swap(0, Ordering::Relaxed),
        HITCHES[1].swap(0, Ordering::Relaxed),
        HITCHES[2].swap(0, Ordering::Relaxed),
        LONGEST.swap(0, Ordering::Relaxed) as f64 / 1000.0
    );
    let (callers, args, notes) = match log().lock() {
        Ok(l) => (l.callers.clone(), l.args.clone(), l.notes.clone()),
        Err(_) => (HashMap::new(), HashMap::new(), Vec::new()),
    };
    for n in notes {
        let _ = writeln!(out, "note: {n}");
    }
    let section = |out: &mut String, title: &str, base: u32, n: usize, name: &dyn Fn(usize) -> String| {
        let _ = writeln!(out, "\n== {title}: calls/frame, total, top callers");
        for i in 0..n {
            let id = base + i as u32;
            let total = CALLS[id as usize].load(Ordering::Relaxed);
            if total == 0 {
                continue;
            }
            let mut from: Vec<(u32, u64)> = callers.iter().filter(|((c, _), _)| *c == id).map(|((_, r), n)| (*r, *n)).collect();
            from.sort_by(|a, b| b.1.cmp(&a.1));
            let tops: Vec<String> = from.iter().take(8).map(|(r, n)| format!("{r:#010x}x{n}")).collect();
            let _ = writeln!(out, "{i:3} {:<34} {:>10.2} {:>10} {}", name(i), total as f64 / frames as f64, total, tops.join(" "));
        }
    };
    section(&mut out, "BullyNiDX9Renderer vtable", RENDERER_BASE, RENDERER_METHODS, &renderer);
    section(&mut out, "IDirect3DDevice9", DEVICE_BASE, DEVICE_METHODS, &|i| DEVICE[i].to_string());
    let _ = writeln!(out, "\n== renderer arguments (sampled): this, arg0..arg3 -> calls | one example of arg0..arg3");
    let ids = (0..RENDERER_METHODS as u32).map(|i| (RENDERER_BASE + i, renderer(i as usize))).chain(DEVICE_SAMPLED.iter().map(|&i| (DEVICE_BASE + i, format!("device {}", DEVICE[i as usize]))));
    for (id, name) in ids {
        let Some(list) = args.get(&id) else { continue };
        let _ = writeln!(out, "{:3} {name}", id % DEVICE_BASE);
        for (pattern, n, example) in list {
            let _ = writeln!(out, "      {pattern} -> {n} | {example}");
        }
    }
    out
}
