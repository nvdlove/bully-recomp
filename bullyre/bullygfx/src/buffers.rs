//! Which of the game's vertex and index buffers were written since the replay read
//! them: Lock is hooked in the buffers' vtables (one per class, shared by every
//! buffer), so a buffer is only read again once the game has locked it for writing.

use std::collections::HashSet;
use std::ffi::c_void;
use std::sync::Mutex;

use crate::win::*;

const LOCK: usize = 11;
const D3DLOCK_READONLY: u32 = 0x10;
const E_FAIL: i32 = 0x80004005u32 as i32;

type LockFn = unsafe extern "system" fn(usize, u32, u32, *mut *mut c_void, u32) -> i32;

/// (vtable, its original Lock)
static ORIGINALS: Mutex<Vec<(usize, usize)>> = Mutex::new(Vec::new());
static DIRTY: Mutex<Option<HashSet<u32>>> = Mutex::new(None);

unsafe extern "system" fn lock(this: u32, offset: u32, size: u32, out: *mut *mut c_void, flags: u32) -> i32 {
    let vtable = unsafe { *(this as *const usize) };
    let original = ORIGINALS.lock().ok().and_then(|o| o.iter().find(|e| e.0 == vtable).map(|e| e.1));
    if flags & D3DLOCK_READONLY == 0
        && let Ok(mut d) = DIRTY.lock()
    {
        d.get_or_insert_with(HashSet::new).insert(this);
    }
    match original {
        Some(f) => unsafe { std::mem::transmute::<usize, LockFn>(f)(this as usize, offset, size, out, flags) },
        None => E_FAIL,
    }
}

/// Starts watching the buffer's class (once per vtable).
pub fn watch(buf: u32) {
    let vtable = unsafe { *(buf as *const usize) };
    let Ok(mut originals) = ORIGINALS.lock() else { return };
    if originals.iter().any(|e| e.0 == vtable) {
        return;
    }
    let slot = (vtable as *mut usize).wrapping_add(LOCK);
    let current = unsafe { *slot };
    if current == lock as *const () as usize {
        return;
    }
    originals.push((vtable, current));
    let mut old = 0;
    let size = std::mem::size_of::<usize>();
    unsafe {
        VirtualProtect(slot as *mut c_void, size, PAGE_EXECUTE_READWRITE, &mut old);
        *slot = lock as *const () as usize;
        VirtualProtect(slot as *mut c_void, size, old, &mut old);
    }
}

/// Whether the game locked the buffer for writing since `clean`.
pub fn dirty(buf: u32) -> bool {
    DIRTY.lock().ok().is_some_and(|d| d.as_ref().is_some_and(|d| d.contains(&buf)))
}

pub fn clean(buf: u32) {
    if let Ok(mut d) = DIRTY.lock()
        && let Some(d) = d.as_mut()
    {
        d.remove(&buf);
    }
}
