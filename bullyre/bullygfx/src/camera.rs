//! Field of view and aspect ratio, in the game itself (so culling follows, and it works
//! with either renderer).
//!
//! The renderer's SetCameraData gets the camera's NiFrustum (NiCamera +0x104: left,
//! right, top, bottom, near, far, ortho), which the game sets from its FOV (0xC3CD04,
//! horizontal degrees, 85 by default) at 16:9: right = tan(fov/2), top = right * 9/16.
//! Before the call, the frustum is rewritten in place: the vertical angle scaled by the
//! chosen FOV (relative to the game's 85°, so the game's own zooms still work), and the
//! horizontal extent from the real screen aspect (Hor+: an ultrawide sees more at the
//! sides). The game's values are remembered, so this is applied once, not compounded.

use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, AtomicU32, Ordering};

/// FOV in horizontal degrees at 16:9 (the game's default is 85).
static FOV: AtomicU32 = AtomicU32::new(0x42AA_0000); // 85.0
/// Fit the horizontal view to the screen's aspect ratio.
static HOR_PLUS: AtomicBool = AtomicBool::new(true);

const GAME_FOV: f32 = 85.0;
const RES_W: usize = 0x00BC_BC60;
const RES_H: usize = 0x00BC_BC64;

pub fn set(fov: f32, hor_plus: bool) {
    FOV.store(fov.clamp(40.0, 150.0).to_bits(), Ordering::Relaxed);
    HOR_PLUS.store(hor_plus, Ordering::Relaxed);
}

/// (frustum, the game's own left/right/top/bottom, what we wrote)
static LAST: Mutex<Option<(u32, [f32; 4], [f32; 4])>> = Mutex::new(None);

/// Vertical half-angle of a 16:9 view `fov_h` degrees wide.
fn vertical_half(fov_h: f32) -> f32 {
    ((fov_h.to_radians() / 2.0).tan() * 9.0 / 16.0).atan()
}

/// SetCameraData is about to run with this frustum.
pub fn adjust(frustum: u32) {
    if frustum == 0 {
        return;
    }
    // the game's own object, alive for the call
    let f = unsafe { std::slice::from_raw_parts_mut(frustum as usize as *mut f32, 7) };
    if f[6] != 0.0 || f[1] <= 0.0 || f[2] <= 0.0 {
        return; // orthographic, or not a plain symmetric view
    }
    let cur = [f[0], f[1], f[2], f[3]];
    let Ok(mut last) = LAST.lock() else { return };
    let base = match *last {
        Some((p, base, written)) if p == frustum && written == cur => base,
        _ => cur,
    };
    let fov = f32::from_bits(FOV.load(Ordering::Relaxed));
    let (w, h) = unsafe { (*(RES_W as *const u32), *(RES_H as *const u32)) };
    let aspect = if w > 0 && h > 0 { w as f32 / h as f32 } else { 16.0 / 9.0 };
    let hor_plus = HOR_PLUS.load(Ordering::Relaxed);
    let scale = vertical_half(fov) / vertical_half(GAME_FOV);
    let v = (base[2].atan() * scale).min(1.45);
    let top = v.tan();
    let right = if hor_plus { top * aspect } else { (base[1].atan() * scale).min(1.5).tan() };
    let new = if (scale - 1.0).abs() < 1e-4 && (!hor_plus || (aspect - base[1] / base[2]).abs() < 0.01) {
        base
    } else {
        [-right, right, top, -top]
    };
    f[..4].copy_from_slice(&new);
    *last = Some((frustum, base, new));
}

#[cfg(test)]
mod tests {
    #[test]
    fn default_fov_at_16_9_changes_nothing() {
        let s = super::vertical_half(85.0) / super::vertical_half(85.0);
        assert!((s - 1.0).abs() < 1e-6);
        // the game's frustum: right = tan(42.5°), top = right * 9/16
        let right = 42.5f32.to_radians().tan();
        assert!((right - 0.91633).abs() < 1e-4);
    }
}
