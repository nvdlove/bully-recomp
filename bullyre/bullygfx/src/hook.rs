//! Watching calls: each watched vtable slot is pointed at a small thunk that saves
//! every register, reports (slot id, the saved registers) to `stats::record` (which
//! finds the return address, `this` and the arguments from there), restores the
//! registers and jumps to the original method. It works for thiscall (Gamebryo,
//! `this` in ecx) and stdcall COM methods alike.
//!
//! In the 64-bit game (bullyre_x64) the thunks are x64 code: they keep rcx, rdx, r8, r9 in
//! their home slots and xmm0-3 below, and call `stats::record64`, which makes the 32-bit
//! frame `record` reads out of those and puts back what it changes. The renderer's vtable
//! is the game's (4-byte slots), so its thunks sit below 2 GB (lowmem).

use std::ffi::c_void;
use std::sync::atomic::Ordering;

use crate::stats::{self, DEVICE_BASE, RENDERER_BASE};
use crate::win::*;

/// BullyNiDX9Renderer's vtable, and its first entry (to check it's the right exe).
const RENDERER_VTABLE: usize = 0x008F_EFE4;
const RENDERER_SLOT0: usize = 0x0088_24F0;
pub const RENDERER_METHODS: usize = 105;

pub const DEVICE_METHODS: usize = 119;
const D3D9_CREATE_DEVICE: usize = 16;

#[cfg(target_arch = "x86")]
const THUNK_SIZE: usize = 32;
#[cfg(target_arch = "x86_64")]
const THUNK_SIZE: usize = 128;

/// The x64 thunks' common part: the argument registers kept (rcx, rdx, r8, r9 in their
/// home slots, [rbp+0x10..0x30]; xmm0-3 below the shadow space), `stats::record64(id, rbp)`
/// called, everything put back (what record changed included) and the frame left.
#[cfg(target_arch = "x86_64")]
fn x64_report(id: u32) -> Vec<u8> {
    let mut c = vec![
        0x55, // push rbp
        0x48, 0x89, 0xE5, // mov rbp, rsp
        0x48, 0x89, 0x4D, 0x10, // mov [rbp+0x10], rcx
        0x48, 0x89, 0x55, 0x18, // mov [rbp+0x18], rdx
        0x4C, 0x89, 0x45, 0x20, // mov [rbp+0x20], r8
        0x4C, 0x89, 0x4D, 0x28, // mov [rbp+0x28], r9
        0x48, 0x83, 0xEC, 0x60, // sub rsp, 0x60 (the shadow space, then xmm0-3)
        0xF3, 0x0F, 0x7F, 0x44, 0x24, 0x20, // movdqu [rsp+0x20], xmm0
        0xF3, 0x0F, 0x7F, 0x4C, 0x24, 0x30, // movdqu [rsp+0x30], xmm1
        0xF3, 0x0F, 0x7F, 0x54, 0x24, 0x40, // movdqu [rsp+0x40], xmm2
        0xF3, 0x0F, 0x7F, 0x5C, 0x24, 0x50, // movdqu [rsp+0x50], xmm3
        0xB9, // mov ecx, id
    ];
    c.extend(id.to_le_bytes());
    c.extend([0x48, 0x89, 0xEA]); // mov rdx, rbp
    c.extend([0x48, 0xB8]); // mov rax, record64
    c.extend((stats::record64 as *const () as u64).to_le_bytes());
    c.extend([
        0xFF, 0xD0, // call rax
        0xF3, 0x0F, 0x6F, 0x44, 0x24, 0x20, // movdqu xmm0, [rsp+0x20]
        0xF3, 0x0F, 0x6F, 0x4C, 0x24, 0x30, // movdqu xmm1, [rsp+0x30]
        0xF3, 0x0F, 0x6F, 0x54, 0x24, 0x40, // movdqu xmm2, [rsp+0x40]
        0xF3, 0x0F, 0x6F, 0x5C, 0x24, 0x50, // movdqu xmm3, [rsp+0x50]
        0x48, 0x8B, 0x4D, 0x10, // mov rcx, [rbp+0x10]
        0x48, 0x8B, 0x55, 0x18, // mov rdx, [rbp+0x18]
        0x4C, 0x8B, 0x45, 0x20, // mov r8, [rbp+0x20]
        0x4C, 0x8B, 0x4D, 0x28, // mov r9, [rbp+0x28]
        0x48, 0x89, 0xEC, // mov rsp, rbp
        0x5D, // pop rbp
    ]);
    c
}

/// Writable, executable memory for the thunks and the originals they jump to.
struct Arena {
    code: *mut u8,
    used: usize,
    cap: usize,
}

impl Arena {
    fn new(cap: usize) -> Option<Arena> {
        let code = crate::lowmem::code_pages(cap);
        (!code.is_null()).then_some(Arena { code, used: 0, cap })
    }

    /// A thunk for `id` that ends in `jmp [original]`, where the original is stored
    /// right after the code.
    #[cfg(target_arch = "x86")]
    fn thunk(&mut self, id: u32, original: usize) -> Option<usize> {
        if self.used + THUNK_SIZE > self.cap {
            return None;
        }
        let at = self.code as usize + self.used;
        let orig_slot = (at + 28) as u32;
        let record = stats::record as *const () as usize as u32;
        let mut c = vec![0x60, 0x54]; // pushad; push esp: the saved registers, then the stack
        c.push(0x68); // push id
        c.extend(id.to_le_bytes());
        c.push(0xB8); // mov eax, record
        c.extend(record.to_le_bytes());
        c.extend([0xFF, 0xD0]); // call eax
        c.extend([0x83, 0xC4, 0x08]); // add esp, 8
        c.push(0x61); // popad
        c.extend([0xFF, 0x25]); // jmp [orig_slot]
        c.extend(orig_slot.to_le_bytes());
        debug_assert!(c.len() <= 28);
        c.resize(28, 0xCC);
        c.extend((original as u32).to_le_bytes());
        unsafe { std::ptr::copy_nonoverlapping(c.as_ptr(), at as *mut u8, THUNK_SIZE) };
        self.used += THUNK_SIZE;
        Some(at)
    }

    /// The x64 thunk: the report, then jmp [rip+0] to the original (kept after it)
    #[cfg(target_arch = "x86_64")]
    fn thunk(&mut self, id: u32, original: usize) -> Option<usize> {
        if self.used + THUNK_SIZE > self.cap {
            return None;
        }
        let at = self.code as usize + self.used;
        let mut c = x64_report(id);
        c.extend([0xFF, 0x25, 0, 0, 0, 0]); // jmp [rip+0]
        c.extend((original as u64).to_le_bytes());
        assert!(c.len() <= THUNK_SIZE);
        c.resize(THUNK_SIZE, 0xCC);
        unsafe { std::ptr::copy_nonoverlapping(c.as_ptr(), at as *mut u8, THUNK_SIZE) };
        self.used += THUNK_SIZE;
        Some(at)
    }
}

/// Points `count` slots of one of the game's vtables (4-byte slots, in either game) from
/// `vtable` at thunks with ids `base + i`.
unsafe fn watch_vtable(vtable: *mut u32, count: usize, base: u32) -> bool {
    let Some(mut arena) = Arena::new(count * THUNK_SIZE) else { return false };
    let mut old = 0;
    if unsafe { VirtualProtect(vtable as *mut c_void, count * 4, PAGE_EXECUTE_READWRITE, &mut old) } == 0 {
        return false;
    }
    for i in 0..count {
        let slot = unsafe { vtable.add(i) };
        if let Some(t) = arena.thunk(base + i as u32, unsafe { *slot } as usize) {
            unsafe { *slot = t as u32 };
        }
    }
    unsafe {
        VirtualProtect(vtable as *mut c_void, count * 4, old, &mut old);
        FlushInstructionCache(GetCurrentProcess(), arena.code as *const c_void, arena.used);
    }
    true
}

/// The game's renderer: every one of its methods.
pub unsafe fn watch_renderer() {
    let vtable = RENDERER_VTABLE as *mut u32;
    if unsafe { *vtable } as usize != RENDERER_SLOT0 {
        stats::note("renderer vtable not where expected (different game exe?), not watched");
        return;
    }
    if !unsafe { watch_vtable(vtable, RENDERER_METHODS, RENDERER_BASE) } {
        stats::note("couldn't patch the renderer vtable");
    }
}

static REAL_CREATE_DEVICE: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);

/// IDirect3D9: catch CreateDevice so the device's methods can be watched too.
pub unsafe fn watch_d3d(d3d: *mut c_void) {
    unsafe { crate::modes::watch(d3d) };
    let vtable = unsafe { *(d3d as *const *mut usize) };
    let slot = unsafe { vtable.add(D3D9_CREATE_DEVICE) };
    if unsafe { *slot } == create_device as *const () as usize {
        return;
    }
    let mut old = 0;
    let size = std::mem::size_of::<usize>();
    unsafe {
        VirtualProtect(slot as *mut c_void, size, PAGE_EXECUTE_READWRITE, &mut old);
        REAL_CREATE_DEVICE.store(*slot, Ordering::SeqCst);
        *slot = create_device as *const () as usize;
        VirtualProtect(slot as *mut c_void, size, old, &mut old);
    }
}

type CreateDevice = unsafe extern "system" fn(*mut c_void, u32, u32, *mut c_void, u32, *mut c_void, *mut *mut c_void) -> i32;

unsafe extern "system" fn create_device(
    this: *mut c_void,
    adapter: u32,
    ty: u32,
    window: *mut c_void,
    flags: u32,
    params: *mut c_void,
    out: *mut *mut c_void,
) -> i32 {
    let real: CreateDevice = unsafe { std::mem::transmute(REAL_CREATE_DEVICE.load(Ordering::SeqCst)) };
    // not a pure device: the replay reads the device state back with Get* calls
    const D3DCREATE_PUREDEVICE: u32 = 0x10;
    // thread safe: the replay's render thread reads the game's resources
    const D3DCREATE_MULTITHREADED: u32 = 0x4;
    let flags = (flags & !D3DCREATE_PUREDEVICE) | D3DCREATE_MULTITHREADED;
    let hr = unsafe { real(this, adapter, ty, window, flags, params, out) };
    stats::note(&format!("CreateDevice(adapter {adapter}, type {ty}, flags {flags:#x}) = {hr:#010x}"));
    if hr >= 0 && !out.is_null() && !unsafe { *out }.is_null() {
        match unsafe { wrap_device(*out) } {
            Some(w) => unsafe { *out = w },
            None => stats::note("couldn't wrap the device"),
        }
    }
    hr
}

/// The device the game gets: a stand-in whose methods report the call and then
/// run the real device's. (d3d9 switches the real device's vtable around after
/// start-up, so patching that vtable only sees the first few frames.)
#[repr(C)]
struct Wrapper {
    vtable: *const usize,
    real: *mut c_void,
}

#[cfg(target_arch = "x86")]
const WRAP_THUNK: usize = 48;
#[cfg(target_arch = "x86_64")]
const WRAP_THUNK: usize = 128;
static DEVICE_VTABLE: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);

/// One thunk of the wrapper's vtable: report, then swap `this` for the real device
/// and jump to its method i.
#[cfg(target_arch = "x86")]
fn wrap_thunk(i: usize) -> Vec<u8> {
    // (eax is free: COM methods are stdcall and take nothing in registers)
    let record = stats::record as *const () as usize as u32;
    let mut c = vec![0x60, 0x54, 0x68]; // pushad; push esp; push id
    c.extend((DEVICE_BASE + i as u32).to_le_bytes());
    c.push(0xB8); // mov eax, record
    c.extend(record.to_le_bytes());
    c.extend([0xFF, 0xD0, 0x83, 0xC4, 0x08, 0x61]); // call eax; add esp, 8; popad
    c.extend([0x8B, 0x44, 0x24, 0x04]); // mov eax, [esp+4]    wrapper
    c.extend([0x8B, 0x40, 0x04]); // mov eax, [eax+4]          real device
    c.extend([0x89, 0x44, 0x24, 0x04]); // mov [esp+4], eax
    c.extend([0x8B, 0x00]); // mov eax, [eax]                 its vtable
    c.extend([0xFF, 0xA0]); // jmp [eax + i*4]
    c.extend(((i * 4) as u32).to_le_bytes());
    c
}
#[cfg(target_arch = "x86_64")]
fn wrap_thunk(i: usize) -> Vec<u8> {
    let mut c = x64_report(DEVICE_BASE + i as u32);
    c.extend([0x48, 0x8B, 0x49, 0x08]); // mov rcx, [rcx+8]   the real device
    c.extend([0x48, 0x8B, 0x01]); // mov rax, [rcx]          its vtable
    c.extend([0xFF, 0xA0]); // jmp [rax + i*8]
    c.extend(((i * 8) as u32).to_le_bytes());
    c
}

/// One vtable of thunks, shared by every wrapped device.
unsafe fn device_vtable() -> Option<*const usize> {
    let have = DEVICE_VTABLE.load(Ordering::SeqCst);
    if have != 0 {
        return Some(have as *const usize);
    }
    let word = std::mem::size_of::<usize>();
    let size = DEVICE_METHODS * (WRAP_THUNK + word);
    let mem = unsafe { VirtualAlloc(std::ptr::null_mut(), size, MEM_COMMIT_RESERVE, PAGE_EXECUTE_READWRITE) } as *mut u8;
    if mem.is_null() {
        return None;
    }
    let table = mem as *mut usize;
    let code = unsafe { mem.add(DEVICE_METHODS * word) };
    for i in 0..DEVICE_METHODS {
        let mut c = wrap_thunk(i);
        assert!(c.len() <= WRAP_THUNK);
        c.resize(WRAP_THUNK, 0xCC);
        unsafe {
            let at = code.add(i * WRAP_THUNK);
            std::ptr::copy_nonoverlapping(c.as_ptr(), at, WRAP_THUNK);
            *table.add(i) = at as usize;
        }
    }
    unsafe { FlushInstructionCache(GetCurrentProcess(), mem as *const c_void, size) };
    DEVICE_VTABLE.store(table as usize, Ordering::SeqCst);
    Some(table)
}

/// The wrapper (kept as long as the game might use it: never freed). The game keeps it
/// in a 4-byte field, so in the 64-bit game it's from the low heap.
unsafe fn wrap_device(real: *mut c_void) -> Option<*mut c_void> {
    let vtable = unsafe { device_vtable() }?;
    let w = crate::lowmem::object(std::mem::size_of::<Wrapper>()) as *mut Wrapper;
    if w.is_null() {
        return None;
    }
    unsafe { w.write(Wrapper { vtable, real }) };
    Some(w as *mut c_void)
}

/// The real device behind a wrapper the game passed (a 32-bit value: the game's).
pub fn real_device_of(wrapper: u32) -> usize {
    if wrapper == 0 {
        return 0;
    }
    unsafe { (*(wrapper as usize as *const Wrapper)).real as usize }
}
