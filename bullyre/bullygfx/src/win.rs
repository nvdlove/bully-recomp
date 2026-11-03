//! The Win32 API this uses, declared by hand.

use std::ffi::c_void;

pub const PAGE_EXECUTE_READWRITE: u32 = 0x40;
pub const MEM_COMMIT_RESERVE: u32 = 0x3000;

#[link(name = "kernel32")]
unsafe extern "system" {
    pub fn LoadLibraryW(name: *const u16) -> *mut c_void;
    pub fn GetProcAddress(module: *mut c_void, name: *const u8) -> *mut c_void;
    pub fn GetSystemDirectoryW(buf: *mut u16, n: u32) -> u32;
    pub fn GetModuleFileNameW(module: *mut c_void, buf: *mut u16, n: u32) -> u32;
    pub fn DisableThreadLibraryCalls(module: *mut c_void) -> i32;
    pub fn VirtualAlloc(addr: *mut c_void, n: usize, ty: u32, prot: u32) -> *mut c_void;
    pub fn VirtualProtect(addr: *mut c_void, n: usize, prot: u32, old: *mut u32) -> i32;
    pub fn FlushInstructionCache(process: *mut c_void, addr: *const c_void, n: usize) -> i32;
    pub fn GetCurrentProcess() -> *mut c_void;
    pub fn ReadProcessMemory(p: *mut c_void, addr: *const c_void, buf: *mut c_void, n: usize, read: *mut usize) -> i32;
}

#[repr(C)]
pub struct WNDCLASSEXW {
    pub cbSize: u32,
    pub style: u32,
    pub lpfnWndProc: unsafe extern "system" fn(*mut c_void, u32, usize, isize) -> isize,
    pub cbClsExtra: i32,
    pub cbWndExtra: i32,
    pub hInstance: *mut c_void,
    pub hIcon: *mut c_void,
    pub hCursor: *mut c_void,
    pub hbrBackground: *mut c_void,
    pub lpszMenuName: *const u16,
    pub lpszClassName: *const u16,
    pub hIconSm: *mut c_void,
}

#[repr(C)]
#[derive(Default, Clone, Copy, PartialEq)]
pub struct RECT {
    pub left: i32,
    pub top: i32,
    pub right: i32,
    pub bottom: i32,
}

pub const WS_CHILD: u32 = 0x4000_0000;
pub const WS_DISABLED: u32 = 0x0800_0000;
pub const WS_CLIPSIBLINGS: u32 = 0x0400_0000;
pub const WM_NCHITTEST: u32 = 0x0084;
pub const HTTRANSPARENT: isize = -1;
pub const SW_HIDE: i32 = 0;
pub const SW_SHOWNA: i32 = 8;
pub const SWP_NOZORDER: u32 = 0x4;
pub const SWP_NOACTIVATE: u32 = 0x10;
pub const SWP_ASYNCWINDOWPOS: u32 = 0x4000;
pub const HWND_TOP: *mut c_void = std::ptr::null_mut();

#[link(name = "user32")]
unsafe extern "system" {
    pub fn RegisterClassExW(wc: *const WNDCLASSEXW) -> u16;
    pub fn CreateWindowExW(ex: u32, class: *const u16, title: *const u16, style: u32, x: i32, y: i32, w: i32, h: i32, parent: *mut c_void, menu: *mut c_void, inst: *mut c_void, param: *mut c_void) -> *mut c_void;
    pub fn DefWindowProcW(h: *mut c_void, m: u32, w: usize, l: isize) -> isize;
    pub fn ShowWindow(h: *mut c_void, cmd: i32) -> i32;
    pub fn ShowWindowAsync(h: *mut c_void, cmd: i32) -> i32;
    pub fn GetClientRect(h: *mut c_void, r: *mut RECT) -> i32;
    pub fn SetWindowPos(h: *mut c_void, after: *mut c_void, x: i32, y: i32, w: i32, hh: i32, flags: u32) -> i32;
    pub fn GetAsyncKeyState(vk: i32) -> i16;
    pub fn GetForegroundWindow() -> *mut c_void;
    pub fn GetWindowThreadProcessId(h: *mut c_void, pid: *mut u32) -> u32;
}

#[link(name = "kernel32")]
unsafe extern "system" {
    pub fn GetModuleHandleW(name: *const u16) -> *mut c_void;
    pub fn GetCurrentProcessId() -> u32;
}

/// Whether the window in front is one of the game's (keys pressed elsewhere aren't
/// for us).
pub fn game_in_front() -> bool {
    let mut pid = 0;
    unsafe { GetWindowThreadProcessId(GetForegroundWindow(), &mut pid) };
    pid == unsafe { GetCurrentProcessId() }
}

pub fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain([0]).collect()
}

#[repr(C)]
#[derive(Default)]
struct ProcessMemoryCounters {
    cb: u32,
    page_faults: u32,
    peak_working_set: usize,
    working_set: usize,
    quota_peak_paged: usize,
    quota_paged: usize,
    quota_peak_nonpaged: usize,
    quota_nonpaged: usize,
    pagefile: usize,
    peak_pagefile: usize,
    private: usize,
}

#[link(name = "psapi")]
unsafe extern "system" {
    fn GetProcessMemoryInfo(p: *mut c_void, c: *mut ProcessMemoryCounters, cb: u32) -> i32;
}

/// The process's private bytes.
pub fn private_bytes() -> usize {
    let mut c = ProcessMemoryCounters { cb: std::mem::size_of::<ProcessMemoryCounters>() as u32, ..Default::default() };
    unsafe { GetProcessMemoryInfo(GetCurrentProcess(), &mut c, c.cb) };
    c.private
}

/// D3DLOCKED_RECT: the row pitch, then the pointer to the bits (+4 in the 32-bit game, +8
/// in the 64-bit one).
#[repr(C)]
#[derive(Clone, Copy)]
pub struct LockedRect {
    pub pitch: i32,
    pub bits: *mut c_void,
}
impl LockedRect {
    pub fn new() -> LockedRect {
        LockedRect { pitch: 0, bits: std::ptr::null_mut() }
    }
}

/// A COM object's pointer as the u32 the capture keeps. In the 64-bit game (bullyre_x64)
/// d3d9's objects come from its low heap, below 2 GB, as the game's 4-byte fields need; one
/// that isn't (the address cut) is reported once in the log.
pub fn obj32(p: usize) -> u32 {
    if (p as u64) >> 32 != 0 {
        static REPORTED: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
        if !REPORTED.swap(true, std::sync::atomic::Ordering::Relaxed) {
            crate::control::log(&format!("a D3D object above 4 GB ({p:#x}): kept as its low 32 bits, which is wrong"));
        }
    }
    p as u32
}

/// `n` bytes of our own memory at `addr` into `out`, without faulting on a bad address. The
/// 64-bit game's exe exports a read that does it under SEH (bullyre_read: a plain copy unless
/// it faults); else ReadProcessMemory, a system call each time (in the 64-bit game ~7% of its
/// main thread: the capture looks through the game's objects all frame).
fn read_into(addr: usize, out: *mut c_void, n: usize) -> bool {
    #[cfg(target_arch = "x86_64")]
    {
        static READ: std::sync::OnceLock<usize> = std::sync::OnceLock::new();
        let f = *READ.get_or_init(|| unsafe { GetProcAddress(GetModuleHandleW(std::ptr::null()), c"bullyre_read".as_ptr() as *const u8) as usize });
        if f != 0 {
            let read: unsafe extern "C" fn(usize, *mut c_void, usize) -> i32 = unsafe { std::mem::transmute(f) };
            return unsafe { read(addr, out, n) } != 0;
        }
    }
    let mut got = 0;
    let ok = unsafe { ReadProcessMemory(GetCurrentProcess(), addr as *const c_void, out, n, &mut got) };
    ok != 0 && got == n
}

/// Reads our own memory without faulting on a bad address.
pub fn read_u32(addr: u32) -> Option<u32> {
    let mut v = 0u32;
    read_into(addr as usize, &mut v as *mut u32 as *mut c_void, 4).then_some(v)
}

/// A native word of our own memory (anywhere) without faulting on a bad address.
pub fn read_word(addr: usize) -> Option<usize> {
    let mut v = 0usize;
    read_into(addr, &mut v as *mut usize as *mut c_void, std::mem::size_of::<usize>()).then_some(v)
}

pub fn read_bytes(addr: u32, buf: &mut [u8]) -> bool {
    read_into(addr as usize, buf.as_mut_ptr() as *mut c_void, buf.len())
}

/// The game's exe path.
pub fn exe_path() -> std::path::PathBuf {
    let mut buf = [0u16; 520];
    let n = unsafe { GetModuleFileNameW(std::ptr::null_mut(), buf.as_mut_ptr(), buf.len() as u32) } as usize;
    String::from_utf16_lossy(&buf[..n]).into()
}

/// A file in the game's folder: where the settings, mods, caches and logs live, and the game's
/// own data (Stream). The exe's folder, unless the 64-bit game runs from elsewhere (bullyre_x64
/// started by bullymenu from its own folder): then BULLYGFX_GAME_DIR says where the game is.
pub fn game_file(name: impl AsRef<std::path::Path>) -> std::path::PathBuf {
    match std::env::var_os("BULLYGFX_GAME_DIR") {
        Some(dir) if !dir.is_empty() => std::path::PathBuf::from(dir).join(name),
        _ => exe_path().with_file_name(name.as_ref()),
    }
}
