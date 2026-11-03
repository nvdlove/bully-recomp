//! Driving the game without touching the desktop: commands in `bullygfx_cmd.txt`
//! next to the game are run on its render thread, one frame at a time. Keys go into
//! the game's DirectInput keyboard directly (no window focus needed), teleports go
//! through the game's own request, and screenshots are read back from the renderers
//! (so they never capture whatever is on screen). What happened is appended to
//! `bullygfx_log.txt`.
//!
//! Commands, one per line:
//!   key <name> <ms>          hold a key (ENTER ESC W A S D SPACE UP DOWN LEFT RIGHT)
//!   wait <ms>                pause the script
//!   renderer on|off          the new renderer
//!   stage <n>                which texture stage is the base colour
//!   shot old|new <file.png>  the game's picture / the new renderer's
//!   shot both <name>         both, from the same frame: <name>_old.png, <name>_new.png
//!   tp <x> <y> <z> <area> [heading]
//!   where                    log the player's position and area
//!   dumpskin                 log the next skinned draw in full
//!   hideskinned on|off       the peds left out of the replay (testing: their pixels)

use std::collections::VecDeque;
use std::ffi::c_void;
use std::path::PathBuf;
use std::sync::Mutex;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::{Duration, Instant};

use crate::win::*;

const CMD_FILE: &str = "bullygfx_cmd.txt";
const LOG_FILE: &str = "bullygfx_log.txt";

/// The game's input manager: keyboard and mouse IDirectInputDevice8 pointers.
const KEYBOARD: u32 = 0x020C_ECF4;
// IDirectInputDevice8 vtable
const GET_DEVICE_STATE: usize = 9;
const GET_DEVICE_DATA: usize = 10;

/// wgpu's own diagnostics (warnings and errors) into the log.
struct Logger;

impl ::log::Log for Logger {
    fn enabled(&self, m: &::log::Metadata) -> bool {
        m.level() <= ::log::Level::Warn
    }
    fn log(&self, r: &::log::Record) {
        if self.enabled(r.metadata()) {
            let mut text = format!("[{} {}] {}", r.level(), r.target(), r.args());
            text.truncate(2000);
            log(&text);
        }
    }
    fn flush(&self) {}
}

pub fn install_logger() {
    static LOGGER: Logger = Logger;
    if ::log::set_logger(&LOGGER).is_ok() {
        ::log::set_max_level(::log::LevelFilter::Warn);
    }
}

/// The command file: `bullygfx_cmd.txt`, or the name in BULLYGFX_CMD (test runs of the
/// 64-bit game side by side, each with its own).
fn cmd_file() -> std::path::PathBuf {
    crate::win::game_file(std::env::var("BULLYGFX_CMD").unwrap_or_else(|_| CMD_FILE.to_string()))
}

/// Commands left over from before this game started would run at the wrong time.
pub fn clear_stale_commands() {
    let _ = std::fs::remove_file(cmd_file());
}

pub fn log(s: &str) {
    use std::io::Write;
    if let Ok(mut f) = std::fs::OpenOptions::new().create(true).append(true).open(crate::win::game_file(LOG_FILE)) {
        let _ = writeln!(f, "{s}");
    }
}

fn dik(name: &str) -> Option<u8> {
    Some(match name.to_ascii_uppercase().as_str() {
        "ESC" => 0x01,
        "ENTER" => 0x1C,
        "W" => 0x11,
        "A" => 0x1E,
        "S" => 0x1F,
        "D" => 0x20,
        "SPACE" => 0x39,
        "UP" => 0xC8,
        "DOWN" => 0xD0,
        "LEFT" => 0xCB,
        "RIGHT" => 0xCD,
        _ => return None,
    })
}

// ---- the virtual keyboard ----------------------------------------------------

struct Keys {
    held: Vec<(u8, Instant)>, // key, release at
    reported: [bool; 256],    // what GetDeviceData has told the game
}

static KEYS: Mutex<Keys> = Mutex::new(Keys { held: Vec::new(), reported: [false; 256] });
static REAL_STATE: AtomicUsize = AtomicUsize::new(0);
static REAL_DATA: AtomicUsize = AtomicUsize::new(0);
static HOOKED_VTABLE: AtomicUsize = AtomicUsize::new(0);

fn held_now() -> Vec<u8> {
    let Ok(mut k) = KEYS.lock() else { return Vec::new() };
    let now = Instant::now();
    k.held.retain(|&(_, until)| until > now);
    k.held.iter().map(|&(key, _)| key).collect()
}

/// IDirectInputDevice8::Acquire: whether the keyboard is ours again (the game in front).
unsafe fn acquire(device: u32) -> bool {
    type Acquire = unsafe extern "system" fn(usize) -> i32;
    let f: Acquire = unsafe { std::mem::transmute(*(*(device as *const *const usize)).add(7)) };
    (unsafe { f(device as usize) }) >= 0
}

fn is_keyboard(this: u32) -> bool {
    read_u32(KEYBOARD) == Some(this)
}

unsafe extern "system" fn get_device_state(this: u32, size: u32, data: *mut u8) -> i32 {
    type F = unsafe extern "system" fn(usize, u32, *mut u8) -> i32;
    let real: F = unsafe { std::mem::transmute(REAL_STATE.load(Ordering::SeqCst)) };
    let mut hr = unsafe { real(this as usize, size, data) };
    if is_keyboard(this) && size == 256 && !data.is_null() {
        let held = held_now();
        if hr < 0 && unsafe { acquire(this) } {
            hr = unsafe { real(this as usize, size, data) };
        }
        if hr < 0 {
            // not acquired (the game isn't in front): no keys, and the virtual ones.
            // The game isn't told (it doesn't cope while running in the background);
            // acquire() above gets the keyboard back as soon as it's in front again.
            unsafe { std::ptr::write_bytes(data, 0, 256) };
            hr = 0;
        }
        for k in held {
            unsafe { *data.add(k as usize) = 0x80 };
        }
    }
    hr
}

unsafe extern "system" fn get_device_data(this: u32, item: u32, out: *mut u32, count: *mut u32, flags: u32) -> i32 {
    type F = unsafe extern "system" fn(usize, u32, *mut u32, *mut u32, u32) -> i32;
    let real: F = unsafe { std::mem::transmute(REAL_DATA.load(Ordering::SeqCst)) };
    let room = if count.is_null() { 0 } else { unsafe { *count } };
    let mut hr = unsafe { real(this as usize, item, out, count, flags) };
    if !is_keyboard(this) || count.is_null() || item < 16 {
        return hr;
    }
    if hr < 0 && unsafe { acquire(this) } {
        unsafe { *count = room };
        hr = unsafe { real(this as usize, item, out, count, flags) };
    }
    if hr < 0 {
        // as in get_device_state: no events, not an error
        unsafe { *count = 0 };
        hr = 0;
    }
    // add press/release events for the virtual keys that changed
    let held = held_now();
    let Ok(mut k) = KEYS.lock() else { return hr };
    let mut n = unsafe { *count };
    for key in 0..=255u8 {
        let down = held.contains(&key);
        if down != k.reported[key as usize] && !out.is_null() && n < room {
            let rec = unsafe { (out as *mut u8).add((n * item) as usize) as *mut u32 };
            unsafe {
                *rec = key as u32; // dwOfs
                *rec.add(1) = if down { 0x80 } else { 0 }; // dwData
                *rec.add(2) = 0; // timestamp
                *rec.add(3) = 0; // sequence
                if item >= 20 {
                    *rec.add(4) = 0; // app data
                }
            }
            k.reported[key as usize] = down;
            n += 1;
        }
    }
    unsafe { *count = n };
    hr
}

/// Hooks the keyboard's GetDeviceState/GetDeviceData once the game has made it.
fn hook_keyboard() {
    let Some(dev) = read_u32(KEYBOARD).filter(|&d| d != 0) else { return };
    let vtable = unsafe { *(dev as usize as *const usize) };
    if vtable == 0 || HOOKED_VTABLE.load(Ordering::SeqCst) == vtable {
        return;
    }
    let slots = vtable as *mut usize;
    let mut old = 0;
    let size = 2 * std::mem::size_of::<usize>();
    unsafe {
        VirtualProtect(slots.add(GET_DEVICE_STATE) as *mut c_void, size, PAGE_EXECUTE_READWRITE, &mut old);
        let (s, d) = (*slots.add(GET_DEVICE_STATE), *slots.add(GET_DEVICE_DATA));
        if s != get_device_state as *const () as usize {
            REAL_STATE.store(s, Ordering::SeqCst);
            REAL_DATA.store(d, Ordering::SeqCst);
            *slots.add(GET_DEVICE_STATE) = get_device_state as *const () as usize;
            *slots.add(GET_DEVICE_DATA) = get_device_data as *const () as usize;
        }
        VirtualProtect(slots.add(GET_DEVICE_STATE) as *mut c_void, size, old, &mut old);
    }
    HOOKED_VTABLE.store(vtable, Ordering::SeqCst);
    log("keyboard hooked");
}

// ---- teleport ----------------------------------------------------------------

/// What the script function PlayerSetPosXYZArea (0x5D23B0) does: flag, then queue
/// the request with the teleport manager (the main loop carries it out).
fn teleport(x: f32, y: f32, z: f32, area: i32, heading: f32) {
    if read_u32(0x00C1_AEA8).is_none_or(|p| p == 0) {
        return log("tp: no player yet");
    }
    let Some(flags) = read_u32(0x009E_4F4C).filter(|&p| p != 0) else { return log("tp: no game state") };
    unsafe { *((flags + 0x8DC) as *mut u8) = 1 };
    let pos = [x, y, z];
    let area = [area];
    // (thiscall in the 32-bit game; in the 64-bit one the game's functions take `this`
    // first in the x64 convention)
    #[cfg(target_arch = "x86")]
    type Request = unsafe extern "thiscall" fn(usize, *const f32, f32, *const i32);
    #[cfg(target_arch = "x86_64")]
    type Request = unsafe extern "C" fn(usize, *const f32, f32, *const i32);
    let request: Request = unsafe { std::mem::transmute(0x0041_5880usize) };
    unsafe { request(0x00BC_C580, pos.as_ptr(), heading, area.as_ptr()) };
}

fn where_is_player() -> String {
    let Some(ped) = read_u32(0x00C1_AEA8).filter(|&p| p != 0) else { return "no player".into() };
    let m = read_u32(ped + 0x14).unwrap_or(0);
    let at = if m != 0 { m + 0x30 } else { ped + 4 };
    let f = |o: u32| f32::from_bits(read_u32(at + o).unwrap_or(0));
    let area = read_u32(0x00BD_1008).unwrap_or(0) as i32;
    format!("pos {:.2} {:.2} {:.2} area {area}", f(0), f(4), f(8))
}

// ---- the command queue -------------------------------------------------------

pub enum Action {
    None,
    Renderer(bool),
    ShotNew(PathBuf),
    ShotOld(PathBuf),
    /// Both, from the same frame: (old, new).
    ShotBoth(PathBuf, PathBuf),
    Stage(usize),
    SkinSwap(bool),
    LightMode(u32),
}

pub struct Control {
    queue: VecDeque<String>,
    not_before: Option<Instant>,
    frames: u64,
}

impl Control {
    pub const fn new() -> Control {
        Control { queue: VecDeque::new(), not_before: None, frames: 0 }
    }

    /// Once a frame on the render thread: pick up new commands, run what's due.
    /// Returns what the renderer side has to do.
    pub fn frame(&mut self) -> Action {
        self.frames += 1;
        if self.frames % 30 == 0 {
            hook_keyboard();
        }
        if self.frames % 10 == 0 {
            let path = cmd_file();
            if let Ok(text) = std::fs::read_to_string(&path) {
                let _ = std::fs::remove_file(&path);
                self.queue.extend(text.lines().map(str::trim).filter(|l| !l.is_empty()).map(String::from));
            }
        }
        if self.not_before.is_some_and(|t| Instant::now() < t) {
            return Action::None;
        }
        let Some(line) = self.queue.pop_front() else { return Action::None };
        log(&format!("> {line}"));
        let w: Vec<&str> = line.split_whitespace().collect();
        let num = |i: usize| w.get(i).and_then(|v| v.parse::<f32>().ok());
        match w.first().copied() {
            Some("key") => match (w.get(1).and_then(|n| dik(n)), num(2)) {
                (Some(k), Some(ms)) => {
                    let until = Instant::now() + Duration::from_millis(ms as u64);
                    if let Ok(mut keys) = KEYS.lock() {
                        keys.held.push((k, until));
                    }
                    self.not_before = Some(until + Duration::from_millis(100));
                }
                _ => log("bad key command"),
            },
            Some("wait") => self.not_before = Some(Instant::now() + Duration::from_millis(num(1).unwrap_or(0.) as u64)),
            Some("renderer") => return Action::Renderer(w.get(1) == Some(&"on")),
            Some("stage") => return Action::Stage(num(1).unwrap_or(0.) as usize),
            Some("skinswap") => return Action::SkinSwap(w.get(1) == Some(&"on")),
            Some("lightmode") => return Action::LightMode(num(1).unwrap_or(0.) as u32),
            Some("shot") => {
                let file = crate::win::game_file(w.get(2).copied().unwrap_or("shot.png"));
                return match w.get(1).copied() {
                    Some("new") => Action::ShotNew(file),
                    Some("final") => {
                        crate::replay::SHOT_FINAL.store(true, std::sync::atomic::Ordering::Relaxed);
                        Action::ShotNew(file)
                    }
                    Some("both") => {
                        let name = w.get(2).copied().unwrap_or("shot");
                        Action::ShotBoth(crate::win::game_file(format!("{name}_old.png")), crate::win::game_file(format!("{name}_new.png")))
                    }
                    _ => Action::ShotOld(file),
                };
            }
            Some("tp") => match (num(1), num(2), num(3), num(4)) {
                (Some(x), Some(y), Some(z), Some(a)) => teleport(x, y, z, a as i32, num(5).unwrap_or(0.)),
                _ => log("bad tp command"),
            },
            Some("where") => log(&where_is_player()),
            Some("weather") => match num(1) {
                // what the script function WeatherSet does
                Some(n) if read_u32(0x00C1_AEA8).is_some_and(|p| p != 0) => {
                    type SetWeather = unsafe extern "C" fn(u32, u32);
                    let set: SetWeather = unsafe { std::mem::transmute(0x0053_60E0usize) };
                    unsafe { set(n as u32, 0) };
                    log(&format!("weather {n}"));
                }
                _ => log("weather: needs N and a player"),
            },
            Some("getweather") => log(&format!("weather is {:?}", read_u32(0x00CE_756C).map(|v| v as i16))),
            Some("clock") => match (num(1), num(2)) {
                // what the script function ClockSet does
                (Some(h), Some(m)) if read_u32(0x00C1_AEA8).is_some_and(|p| p != 0) => {
                    type SetClock = unsafe extern "C" fn(u32, u32);
                    let set: SetClock = unsafe { std::mem::transmute(0x0042_6070usize) };
                    unsafe { set(h as u32, m as u32) };
                    log(&format!("clock {h:02}:{m:02}"));
                }
                _ => log("clock: needs H M and a player"),
            },
            Some("dumpskin") => crate::scene::dump_next_skinned(),
            Some("skins") => crate::scene::list_skinned(),
            Some("dumpmesh") => crate::scene::dump_next_mesh(),
            Some("hideskinned") => crate::capture::HIDE_SKINNED.store(w.get(1) != Some(&"off"), std::sync::atomic::Ordering::Relaxed),
            Some("screenonly") => crate::capture::SCREEN_ONLY.store(w.get(1) != Some(&"off"), std::sync::atomic::Ordering::Relaxed),
            Some("names") => crate::textures::REGISTER.store(w.get(1) != Some(&"off"), std::sync::atomic::Ordering::Relaxed),
            Some("diag") => crate::stats::DIAGNOSTICS.store(w.get(1) != Some(&"off"), std::sync::atomic::Ordering::Relaxed),
            Some("camprobe") => crate::stats::CAMERA_PROBE.store(num(1).unwrap_or(8.) as u32, std::sync::atomic::Ordering::Relaxed),
            Some("wetdebug") => crate::wet::DEBUG.store(w.get(1) != Some(&"off"), std::sync::atomic::Ordering::Relaxed),
            Some("dumprts") => crate::replay::DUMP_RTS.store(true, std::sync::atomic::Ordering::Relaxed),
            Some("frameinfo") => crate::replay::DESCRIBE.store(true, std::sync::atomic::Ordering::Relaxed),
            Some("nocull") => crate::replay::NO_CULL.store(w.get(1) != Some(&"off"), std::sync::atomic::Ordering::Relaxed),
            Some("samplers") => crate::replay::SAMPLER_REPORT.store(true, std::sync::atomic::Ordering::Relaxed),
            Some("exclusive") => {
                crate::capture::EXCLUSIVE.store(w.get(1) != Some(&"off"), std::sync::atomic::Ordering::Relaxed);
                log("exclusive set");
            }
            Some("dumpdraw") => crate::replay::DUMP_DRAW.store(if w.get(1) == Some(&"portal") { u32::MAX } else { num(1).unwrap_or(1.) as u32 }, std::sync::atomic::Ordering::Relaxed),
            Some("replaylimit") => crate::replay::LIMIT.store(num(1).unwrap_or(0.) as u32, std::sync::atomic::Ordering::Relaxed),
            _ => log("unknown command"),
        }
        Action::None
    }
}

// ---- screenshots -------------------------------------------------------------

/// A PNG from 8-bit RGBA rows (stored, uncompressed deflate: no dependencies).
pub fn write_png(path: &std::path::Path, width: u32, height: u32, rgba: &[u8]) -> std::io::Result<()> {
    fn crc(data: &[u8]) -> u32 {
        let mut c = 0xFFFF_FFFFu32;
        for &b in data {
            c ^= b as u32;
            for _ in 0..8 {
                c = if c & 1 != 0 { 0xEDB8_8320 ^ (c >> 1) } else { c >> 1 };
            }
        }
        !c
    }
    fn chunk(out: &mut Vec<u8>, kind: &[u8], data: &[u8]) {
        out.extend((data.len() as u32).to_be_bytes());
        let start = out.len();
        out.extend(kind);
        out.extend(data);
        let c = crc(&out[start..]);
        out.extend(c.to_be_bytes());
    }
    let mut raw = Vec::with_capacity((width as usize * 4 + 1) * height as usize);
    for row in rgba.chunks_exact(width as usize * 4).take(height as usize) {
        raw.push(0);
        raw.extend(row);
    }
    let mut z = vec![0x78, 0x01];
    for (i, block) in raw.chunks(65535).enumerate() {
        let last = (i + 1) * 65535 >= raw.len();
        z.push(last as u8);
        z.extend((block.len() as u16).to_le_bytes());
        z.extend((!(block.len() as u16)).to_le_bytes());
        z.extend(block);
    }
    let (mut a, mut b) = (1u32, 0u32);
    for &x in &raw {
        a = (a + x as u32) % 65521;
        b = (b + a) % 65521;
    }
    z.extend(((b << 16) | a).to_be_bytes());
    let mut out = vec![0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A];
    let mut ihdr = Vec::new();
    ihdr.extend(width.to_be_bytes());
    ihdr.extend(height.to_be_bytes());
    ihdr.extend([8, 6, 0, 0, 0]);
    chunk(&mut out, b"IHDR", &ihdr);
    chunk(&mut out, b"IDAT", &z);
    chunk(&mut out, b"IEND", &[]);
    std::fs::write(path, out)
}

/// The game's own picture: the D3D9 back buffer, read back before it's presented.
/// `device` is the real IDirect3DDevice9.
pub unsafe fn shot_old(device: u32, path: &std::path::Path) -> Result<(), String> {
    unsafe fn m(obj: u32, i: usize) -> usize {
        unsafe { *(*(obj as *const *const usize)).add(i) }
    }
    type GetBackBuffer = unsafe extern "system" fn(usize, u32, u32, u32, *mut usize) -> i32;
    type GetDesc = unsafe extern "system" fn(usize, *mut u32) -> i32;
    type CreateOffscreen = unsafe extern "system" fn(usize, u32, u32, u32, u32, *mut usize, *mut c_void) -> i32;
    type GetRtData = unsafe extern "system" fn(usize, usize, usize) -> i32;
    type LockRect = unsafe extern "system" fn(usize, *mut crate::win::LockedRect, *const c_void, u32) -> i32;
    type Unlock = unsafe extern "system" fn(usize) -> i32;
    type Release = unsafe extern "system" fn(usize) -> u32;
    let mut back = 0usize;
    if unsafe { std::mem::transmute::<usize, GetBackBuffer>(m(device, 18))(device as usize, 0, 0, 0, &mut back) } < 0 {
        return Err("GetBackBuffer failed".into());
    }
    let back = crate::win::obj32(back);
    // D3DSURFACE_DESC: format, type, usage, pool, multisample, quality, width, height
    let mut desc = [0u32; 8];
    unsafe { std::mem::transmute::<usize, GetDesc>(m(back, 12))(back as usize, desc.as_mut_ptr()) };
    let (format, w, h) = (desc[0], desc[6], desc[7]);
    let mut sys = 0usize;
    let hr = unsafe { std::mem::transmute::<usize, CreateOffscreen>(m(device, 36))(device as usize, w, h, format, 2, &mut sys, std::ptr::null_mut()) };
    let sys = crate::win::obj32(sys);
    let release = |o: u32| unsafe { std::mem::transmute::<usize, Release>(m(o, 2))(o as usize) };
    if hr < 0 {
        release(back);
        return Err(format!("CreateOffscreenPlainSurface failed {hr:#x}"));
    }
    let hr = unsafe { std::mem::transmute::<usize, GetRtData>(m(device, 32))(device as usize, back as usize, sys as usize) };
    let mut result = Err(format!("GetRenderTargetData failed {hr:#x}"));
    if hr >= 0 {
        let mut locked = crate::win::LockedRect::new(); // pitch, bits
        if unsafe { std::mem::transmute::<usize, LockRect>(m(sys, 13))(sys as usize, &mut locked, std::ptr::null(), 0x10) } >= 0 {
            let (pitch, bits) = (locked.pitch as usize, locked.bits as *const u8);
            let mut rgba = Vec::with_capacity((w * h * 4) as usize);
            for y in 0..h as usize {
                let row = unsafe { std::slice::from_raw_parts(bits.add(y * pitch), w as usize * 4) };
                for px in row.chunks_exact(4) {
                    rgba.extend([px[2], px[1], px[0], 255]); // X8R8G8B8 / A8R8G8B8 are BGRA in memory
                }
            }
            unsafe { std::mem::transmute::<usize, Unlock>(m(sys, 14))(sys as usize) };
            result = write_png(path, w, h, &rgba).map_err(|e| e.to_string());
        }
    }
    release(sys);
    release(back);
    result
}
