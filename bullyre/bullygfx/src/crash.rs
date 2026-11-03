//! Native crashes (access violations and the like) leave a note next to the game:
//! the code, where it happened (module + offset) and what on the stack looks like
//! return addresses, so a crash that isn't a Rust panic can still be placed.

use std::ffi::c_void;
use std::fmt::Write;
use std::sync::atomic::{AtomicU32, Ordering};

#[repr(C)]
struct ExceptionRecord {
    code: u32,
    flags: u32,
    record: *mut ExceptionRecord,
    address: *mut c_void,
    params: u32,
    info: [usize; 15],
}

#[repr(C)]
struct Pointers {
    record: *mut ExceptionRecord,
    context: *mut u8, // CONTEXT (x86): Eip at 0xB8, Esp at 0xC4
}

#[link(name = "kernel32")]
unsafe extern "system" {
    fn AddVectoredExceptionHandler(first: u32, handler: unsafe extern "system" fn(*mut Pointers) -> i32) -> *mut c_void;
    fn GetModuleHandleExW(flags: u32, addr: *const c_void, out: *mut *mut c_void) -> i32;
    fn GetModuleFileNameW(module: *mut c_void, name: *mut u16, size: u32) -> u32;
}

const FROM_ADDRESS: u32 = 4;
const UNCHANGED_REFCOUNT: u32 = 2;
const CONTINUE_SEARCH: i32 = 0;

static NOTED: AtomicU32 = AtomicU32::new(0);

/// "module+offset" for a code address, or None outside any module.
fn place(addr: usize) -> Option<String> {
    let mut module = std::ptr::null_mut();
    if unsafe { GetModuleHandleExW(FROM_ADDRESS | UNCHANGED_REFCOUNT, addr as *const c_void, &mut module) } == 0 {
        return None;
    }
    let mut name = [0u16; 260];
    let n = unsafe { GetModuleFileNameW(module, name.as_mut_ptr(), 260) } as usize;
    let path = String::from_utf16_lossy(&name[..n]);
    let file = path.rsplit(['\\', '/']).next().unwrap_or(&path).to_string();
    Some(format!("{file}+{:#x}", addr - module as usize))
}

unsafe extern "system" fn handler(p: *mut Pointers) -> i32 {
    let rec = unsafe { &*(*p).record };
    // access violation, illegal instruction, stack overflow, integer divide by zero
    let serious = matches!(rec.code, 0xC000_0005 | 0xC000_001D | 0xC000_00FD | 0xC000_0094);
    if !serious || NOTED.fetch_add(1, Ordering::Relaxed) >= 5 {
        return CONTINUE_SEARCH;
    }
    let ctx = unsafe { (*p).context };
    // CONTEXT: x86 Eip 0xB8, Esp 0xC4; x64 Rip 0xF8, Rsp 0x98
    #[cfg(target_arch = "x86")]
    let (eip, esp) = unsafe { (*(ctx.add(0xB8) as *const u32) as usize, *(ctx.add(0xC4) as *const u32) as usize) };
    #[cfg(target_arch = "x86_64")]
    let (eip, esp) = unsafe { (*(ctx.add(0xF8) as *const u64) as usize, *(ctx.add(0x98) as *const u64) as usize) };
    let mut out = String::new();
    let _ = writeln!(out, "exception {:#010x} at {:#x} ({})", rec.code, eip, place(eip).unwrap_or_default());
    if rec.code == 0xC000_0005 && rec.params >= 2 {
        let _ = writeln!(out, "  {} {:#x}", if rec.info[0] == 1 { "writing" } else { "reading" }, rec.info[1]);
    }
    let _ = writeln!(out, "  stack (what looks like return addresses):");
    let word = std::mem::size_of::<usize>();
    for i in 0..256 {
        let Some(v) = crate::win::read_word(esp + i * word) else { break };
        if let Some(at) = place(v)
            && !at.starts_with("ntdll")
        {
            let _ = writeln!(out, "    [esp+{:#x}] {:#x} {at}", i * word, v);
        }
    }
    let path = crate::win::game_file("bullygfx_crash.txt");
    let old = std::fs::read_to_string(&path).unwrap_or_default();
    let _ = std::fs::write(&path, old + &out + "\n");
    CONTINUE_SEARCH
}

pub fn install() {
    unsafe { AddVectoredExceptionHandler(0, handler) };
}
