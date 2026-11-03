//! bullygfx: the replacement renderer for Bully: Scholarship Edition, built as the
//! game's `d3d9.dll`.
//!
//! The game renders through Gamebryo 2.x: one renderer, `BullyNiDX9Renderer`
//! (vtable 0x8FEFE4, 105 methods), which loads `D3D9.DLL` by name at run time
//! (LoadLibraryA at 0x878D46) and keeps the IDirect3D9 at 0x20E6D04. A `d3d9.dll`
//! in the game folder is therefore loaded just as the renderer starts, which is where
//! this takes over.
//!
//! Stage 1 (this): pass everything to the system d3d9 and record, per frame, which
//! renderer methods and which Direct3D calls the game makes and from where, into
//! `bullygfx_stats.txt` next to the game. That shows what the new renderer has to
//! implement and whether anything draws around Gamebryo.

#![allow(non_snake_case, clippy::missing_safety_doc)]

mod archive;
mod buffers;
mod camera;
mod capture;
mod control;
mod crash;
mod d3d;
mod dxso;
mod replay;
mod resize;
mod hook;
mod lowmem;
mod modes;
mod modfs;
mod names;
mod nif;
mod portal;
mod post;
mod rtti;
mod scene;
mod settings;
mod shaders;
mod stats;
mod textures;
mod wet;
mod win;

/// The DLL's own heap use (in the stats file): the game is a 32-bit process.
struct Counting;
pub static HEAP: AtomicUsize = AtomicUsize::new(0);
pub static HEAP_PEAK: AtomicUsize = AtomicUsize::new(0);

unsafe impl std::alloc::GlobalAlloc for Counting {
    unsafe fn alloc(&self, l: std::alloc::Layout) -> *mut u8 {
        let p = unsafe { std::alloc::System.alloc(l) };
        if !p.is_null() {
            let now = HEAP.fetch_add(l.size(), Ordering::Relaxed) + l.size();
            HEAP_PEAK.fetch_max(now, Ordering::Relaxed);
        }
        p
    }
    unsafe fn dealloc(&self, p: *mut u8, l: std::alloc::Layout) {
        unsafe { std::alloc::System.dealloc(p, l) };
        HEAP.fetch_sub(l.size(), Ordering::Relaxed);
    }
    unsafe fn realloc(&self, p: *mut u8, l: std::alloc::Layout, new: usize) -> *mut u8 {
        let q = unsafe { std::alloc::System.realloc(p, l, new) };
        if !q.is_null() {
            HEAP.fetch_sub(l.size(), Ordering::Relaxed);
            let now = HEAP.fetch_add(new, Ordering::Relaxed) + new;
            HEAP_PEAK.fetch_max(now, Ordering::Relaxed);
        }
        q
    }
}

#[global_allocator]
static ALLOCATOR: Counting = Counting;

use std::ffi::c_void;
use std::sync::atomic::{AtomicUsize, Ordering};

use win::*;

/// The system d3d9's exports, filled in on load; the ones below jump through these.
static REAL: [AtomicUsize; FORWARDED.len()] = [const { AtomicUsize::new(0) }; FORWARDED.len()];
const FORWARDED: [&str; 11] = [
    "Direct3DCreate9",
    "Direct3DCreate9Ex",
    "D3DPERF_BeginEvent",
    "D3DPERF_EndEvent",
    "D3DPERF_GetStatus",
    "D3DPERF_QueryRepeatFrame",
    "D3DPERF_SetMarker",
    "D3DPERF_SetOptions",
    "D3DPERF_SetRegion",
    "DebugSetMute",
    "Direct3DShaderValidatorCreate9",
];

fn load_real_d3d9() -> bool {
    let mut dir = [0u16; 260];
    let n = unsafe { GetSystemDirectoryW(dir.as_mut_ptr(), dir.len() as u32) } as usize;
    if n == 0 || n >= dir.len() {
        return false;
    }
    let path: Vec<u16> = dir[..n].iter().copied().chain("\\d3d9.dll".encode_utf16()).chain([0]).collect();
    let module = unsafe { LoadLibraryW(path.as_ptr()) };
    if module.is_null() {
        return false;
    }
    for (slot, name) in REAL.iter().zip(FORWARDED) {
        let c: Vec<u8> = name.bytes().chain([0]).collect();
        slot.store(unsafe { GetProcAddress(module, c.as_ptr()) } as usize, Ordering::SeqCst);
    }
    true
}

/// Exports that go straight to the system d3d9 (whatever their arguments).
macro_rules! forward {
    ($($name:ident = $i:expr),* $(,)?) => {$(
        #[unsafe(no_mangle)]
        #[unsafe(naked)]
        pub extern "C" fn $name() {
            #[cfg(target_arch = "x86")]
            core::arch::naked_asm!("jmp dword ptr [{0} + {1}]", sym REAL, const $i * 4);
            #[cfg(target_arch = "x86_64")]
            core::arch::naked_asm!("jmp qword ptr [rip + {0} + {1}]", sym REAL, const $i * 8);
        }
    )*};
}
forward!(
    Direct3DCreate9Ex = 1,
    D3DPERF_BeginEvent = 2,
    D3DPERF_EndEvent = 3,
    D3DPERF_GetStatus = 4,
    D3DPERF_QueryRepeatFrame = 5,
    D3DPERF_SetMarker = 6,
    D3DPERF_SetOptions = 7,
    D3DPERF_SetRegion = 8,
    DebugSetMute = 9,
    Direct3DShaderValidatorCreate9 = 10,
);

/// (bullyre_x64's seamless doors) a doorway another area is drawn through: its corners (4 x
/// x, y, z) and the camera it's seen from (eye, dir, up, right: 4 x x, y, z; then the frustum's
/// left, right, top, bottom, near, far), in the game's world. 0 when the replay renderer isn't
/// recording (the game draws nothing through it then).
#[unsafe(no_mangle)]
pub extern "C" fn bullygfx_portal(corners: *const f32, camera: *const f32) -> i32 {
    if corners.is_null() || camera.is_null() {
        return 0;
    }
    let c = unsafe { std::slice::from_raw_parts(corners, 12) };
    let k = unsafe { std::slice::from_raw_parts(camera, 18) };
    let v3 = |s: &[f32], i: usize| [s[i], s[i + 1], s[i + 2]];
    let cam = capture::Camera { eye: v3(k, 0), dir: v3(k, 3), up: v3(k, 6), right: v3(k, 9), frustum: [k[12], k[13], k[14], k[15], k[16], k[17]] };
    capture::portal([v3(c, 0), v3(c, 3), v3(c, 6), v3(c, 9)], cam, portal::PORTAL_IN) as i32
}

/// bullygfx_portal with a mode: 0 the other area drawn in the doorway (portal.rs's PORTAL_IN), 1
/// the area the camera is still in drawn around it (PORTAL_AROUND: the player has gone through,
/// the camera hasn't yet; the camera is the one the next draws are made from), 2 only whether a
/// doorway would be marked (nothing sent).
#[unsafe(no_mangle)]
pub extern "C" fn bullygfx_portal2(corners: *const f32, camera: *const f32, mode: u32) -> i32 {
    if mode == 2 {
        return capture::portal_ready() as i32;
    }
    if corners.is_null() || camera.is_null() || mode > portal::PORTAL_AROUND {
        return 0;
    }
    let c = unsafe { std::slice::from_raw_parts(corners, 12) };
    let k = unsafe { std::slice::from_raw_parts(camera, 18) };
    let v3 = |s: &[f32], i: usize| [s[i], s[i + 1], s[i + 2]];
    let cam = capture::Camera { eye: v3(k, 0), dir: v3(k, 3), up: v3(k, 6), right: v3(k, 9), frustum: [k[12], k[13], k[14], k[15], k[16], k[17]] };
    capture::portal([v3(c, 0), v3(c, 3), v3(c, 6), v3(c, 9)], cam, mode) as i32
}

/// Exported undecorated (a stdcall export would be `Direct3DCreate9@4`).
#[unsafe(no_mangle)]
#[unsafe(naked)]
pub extern "C" fn Direct3DCreate9() {
    core::arch::naked_asm!("jmp {0}", sym create9)
}

unsafe extern "system" fn create9(sdk: u32) -> *mut c_void {
    let real: unsafe extern "system" fn(u32) -> *mut c_void = unsafe { std::mem::transmute(REAL[0].load(Ordering::SeqCst)) };
    let d3d = unsafe { real(sdk) };
    if !d3d.is_null() && stats::enabled() {
        unsafe { hook::watch_d3d(d3d) };
    }
    d3d
}

#[unsafe(no_mangle)]
pub extern "system" fn DllMain(module: *mut c_void, reason: u32, _: *mut c_void) -> i32 {
    const PROCESS_ATTACH: u32 = 1;
    if reason == PROCESS_ATTACH {
        unsafe { DisableThreadLibraryCalls(module) };
        if !load_real_d3d9() {
            return 0;
        }
        if stats::enabled() {
            // panics abort the game: leave the reason next to it first
            std::panic::set_hook(Box::new(|info| {
                let _ = std::fs::write(crate::win::game_file("bullygfx_panic.txt"), format!("{info}
{}", std::backtrace::Backtrace::force_capture()));
            }));
            crash::install();
            modfs::install();
            control::clear_stale_commands();
            control::install_logger();
            unsafe { hook::watch_renderer() };
            stats::start();
        }
    }
    1
}
