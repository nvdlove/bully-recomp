//! Memory the game's 4-byte fields can hold. The 64-bit game (bullyre_x64.exe) keeps 4-byte
//! pointers in its data, so what it's handed (the wrapped device, the thunks put in its
//! vtable and import slots) has to sit below 2 GB: there it comes from the exe's low heap
//! (its exports bullyre_low_alloc / bullyre_low_pages). In the 32-bit game any memory is.

#[cfg(target_arch = "x86_64")]
fn export(name: &std::ffi::CStr) -> usize {
    use crate::win::{GetModuleHandleW, GetProcAddress};
    unsafe { GetProcAddress(GetModuleHandleW(std::ptr::null()), name.as_ptr() as *const u8) as usize }
}

/// Writable, executable pages for code the game's slots point at.
pub fn code_pages(size: usize) -> *mut u8 {
    #[cfg(target_arch = "x86_64")]
    {
        let f = export(c"bullyre_low_pages");
        if f == 0 {
            return std::ptr::null_mut();
        }
        let f: unsafe extern "C" fn(usize, u32) -> *mut u8 = unsafe { std::mem::transmute(f) };
        unsafe { f(size, crate::win::PAGE_EXECUTE_READWRITE) }
    }
    #[cfg(not(target_arch = "x86_64"))]
    {
        use crate::win::*;
        unsafe { VirtualAlloc(std::ptr::null_mut(), size, MEM_COMMIT_RESERVE, PAGE_EXECUTE_READWRITE) as *mut u8 }
    }
}

/// A block for an object the game keeps a pointer to (kept for the process's life).
pub fn object(size: usize) -> *mut u8 {
    #[cfg(target_arch = "x86_64")]
    {
        let f = export(c"bullyre_low_alloc");
        if f == 0 {
            return std::ptr::null_mut();
        }
        let f: unsafe extern "C" fn(usize) -> *mut u8 = unsafe { std::mem::transmute(f) };
        unsafe { f(size) }
    }
    #[cfg(not(target_arch = "x86_64"))]
    {
        let layout = std::alloc::Layout::from_size_align(size.max(1), 16).unwrap();
        unsafe { std::alloc::alloc(layout) }
    }
}

/// An address the game can keep in a 4-byte slot that calls `target`: itself in the 32-bit
/// game, a jump below 2 GB (jmp [rip+0]; dq target) in the 64-bit one.
pub fn slot_target(target: usize) -> Option<u32> {
    #[cfg(target_arch = "x86_64")]
    {
        let p = code_pages(16);
        if p.is_null() {
            return None;
        }
        let mut c = vec![0xFF, 0x25, 0, 0, 0, 0];
        c.extend((target as u64).to_le_bytes());
        unsafe {
            std::ptr::copy_nonoverlapping(c.as_ptr(), p, c.len());
            crate::win::FlushInstructionCache(crate::win::GetCurrentProcess(), p as *const std::ffi::c_void, c.len());
        }
        Some(p as usize as u32)
    }
    #[cfg(not(target_arch = "x86_64"))]
    {
        Some(target as u32)
    }
}
