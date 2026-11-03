//! Any window size: the game only accepts a resolution from the adapter's display
//! mode list, so a 21:9 window on a 16:9 monitor fell back to 800x600. The resolution
//! the game is set to (its registry RESW/RESH, which bullymenu writes) is added to the
//! modes IDirect3D9 reports.

use std::ffi::c_void;
use std::sync::atomic::{AtomicUsize, Ordering};

use crate::win::*;

const GET_ADAPTER_MODE_COUNT: usize = 6;
const ENUM_ADAPTER_MODES: usize = 7;
const REG_KEY: &str = r"Software\Rockstar Games\Bully Scholarship Edition\A0";

static REAL_COUNT: AtomicUsize = AtomicUsize::new(0);
static REAL_ENUM: AtomicUsize = AtomicUsize::new(0);

type Count = unsafe extern "system" fn(*mut c_void, u32, u32) -> u32;
type Enum = unsafe extern "system" fn(*mut c_void, u32, u32, u32, *mut [u32; 4]) -> i32;

#[link(name = "advapi32")]
unsafe extern "system" {
    fn RegGetValueW(key: isize, sub: *const u16, value: *const u16, flags: u32, ty: *mut u32, data: *mut c_void, size: *mut u32) -> i32;
}

fn reg_dword(name: &str) -> Option<u32> {
    const HKEY_CURRENT_USER: isize = 0x8000_0001u32 as i32 as isize;
    const RRF_RT_REG_DWORD: u32 = 0x10;
    let (mut v, mut n) = (0u32, 4u32);
    let r = unsafe { RegGetValueW(HKEY_CURRENT_USER, wide(REG_KEY).as_ptr(), wide(name).as_ptr(), RRF_RT_REG_DWORD, std::ptr::null_mut(), &mut v as *mut u32 as *mut c_void, &mut n) };
    (r == 0).then_some(v)
}

/// The resolution the game is set to, if it's one the adapter doesn't list.
fn extra(this: *mut c_void, adapter: u32, format: u32) -> Option<(u32, [u32; 4])> {
    let (w, h) = (reg_dword("RESW")?, reg_dword("RESH")?);
    if !(640..=7680).contains(&w) || !(480..=4320).contains(&h) {
        return None;
    }
    let count: Count = unsafe { std::mem::transmute(REAL_COUNT.load(Ordering::SeqCst)) };
    let enumerate: Enum = unsafe { std::mem::transmute(REAL_ENUM.load(Ordering::SeqCst)) };
    let n = unsafe { count(this, adapter, format) };
    let mut refresh = 60;
    for i in 0..n {
        let mut m = [0u32; 4];
        if unsafe { enumerate(this, adapter, format, i, &mut m) } >= 0 {
            if m[0] == w && m[1] == h {
                return None; // already there
            }
            refresh = refresh.max(m[2]);
        }
    }
    (n > 0).then_some((n, [w, h, refresh, format]))
}

unsafe extern "system" fn mode_count(this: *mut c_void, adapter: u32, format: u32) -> u32 {
    let count: Count = unsafe { std::mem::transmute(REAL_COUNT.load(Ordering::SeqCst)) };
    match extra(this, adapter, format) {
        Some((n, _)) => n + 1,
        None => unsafe { count(this, adapter, format) },
    }
}

unsafe extern "system" fn enum_modes(this: *mut c_void, adapter: u32, format: u32, index: u32, out: *mut [u32; 4]) -> i32 {
    let enumerate: Enum = unsafe { std::mem::transmute(REAL_ENUM.load(Ordering::SeqCst)) };
    if let Some((n, mode)) = extra(this, adapter, format)
        && index == n
        && !out.is_null()
    {
        unsafe { *out = mode };
        return 0;
    }
    unsafe { enumerate(this, adapter, format, index, out) }
}

/// Hooks the mode list in IDirect3D9's vtable.
pub unsafe fn watch(d3d: *mut c_void) {
    let vtable = unsafe { *(d3d as *const *mut usize) };
    for (slot, hook, real) in [(GET_ADAPTER_MODE_COUNT, mode_count as *const () as usize, &REAL_COUNT), (ENUM_ADAPTER_MODES, enum_modes as *const () as usize, &REAL_ENUM)] {
        let p = unsafe { vtable.add(slot) };
        if unsafe { *p } == hook {
            continue;
        }
        let mut old = 0;
        unsafe {
            let size = std::mem::size_of::<usize>();
            VirtualProtect(p as *mut c_void, size, PAGE_EXECUTE_READWRITE, &mut old);
            real.store(*p, Ordering::SeqCst);
            *p = hook;
            VirtualProtect(p as *mut c_void, size, old, &mut old);
        }
    }
}
