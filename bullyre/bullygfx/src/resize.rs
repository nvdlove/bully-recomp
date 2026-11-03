//! Resizing the game window changes the game's resolution: when the player lets go of
//! a window edge (or maximizes), the new client size goes to the game's pending video
//! mode (0xBCBC78: width, height, AA, then three more; a non-zero AA marks it), which
//! its main loop applies (device reset) and saves, like its options menu does.
//!
//! The device reset fails while the new renderer is recording (found by elimination:
//! with capture off it always succeeds), so capture is paused a couple of frames before
//! the request and resumed once the reset went through.

use std::time::{Duration, Instant};

use crate::win::*;

const WINDOW: usize = 0x00BD_77FC;
const MODE: usize = 0x00BC_BC60; // width, height, AA, then three more
const PENDING: usize = 0x00BC_BC78;

#[link(name = "user32")]
unsafe extern "system" {
    fn IsIconic(h: *mut std::ffi::c_void) -> i32;
}

enum State {
    Idle,
    /// A new client size, and since when it has held.
    Seen((u32, u32), Instant),
    /// Capture paused; the request goes in at `frame`.
    Pausing((u32, u32), u32),
    /// Asked the game; waiting for the reset (and then a few frames).
    Asked((u32, u32), Instant),
    Settling(u32),
}

pub struct Resizer {
    state: State,
    frame: u32,
    quiet_until: Option<Instant>,
}

fn read(a: usize) -> u32 {
    unsafe { *(a as *const u32) }
}

fn mode() -> (u32, u32) {
    (read(MODE), read(MODE + 4))
}

impl Resizer {
    pub const fn new() -> Resizer {
        Resizer { state: State::Idle, frame: 0, quiet_until: None }
    }

    /// Once a frame, at Present. Returns whether capture must be paused this frame.
    pub fn tick(&mut self) -> bool {
        self.frame = self.frame.wrapping_add(1);
        let window = read(WINDOW) as usize as *mut std::ffi::c_void;
        if window.is_null() || unsafe { IsIconic(window) } != 0 || crate::win::read_u32(0x00C1_AEA8).is_none_or(|p| p == 0) {
            self.state = State::Idle;
            return false;
        }
        let mut r = RECT::default();
        unsafe { GetClientRect(window, &mut r) };
        let (cw, ch) = (r.right.max(0) as u32, r.bottom.max(0) as u32);
        let (rw, rh) = mode();
        let differs = cw.abs_diff(rw) > 8 || ch.abs_diff(rh) > 8;
        let dragging = unsafe { GetAsyncKeyState(0x01) } as u16 & 0x8000 != 0;
        let want = ((cw & !1).max(640), (ch & !1).max(360));
        match self.state {
            State::Idle => {
                if differs && !dragging && self.quiet_until.is_none_or(|t| Instant::now() >= t) {
                    self.state = State::Seen(want, Instant::now());
                }
                false
            }
            State::Seen(size, since) => {
                if !differs {
                    self.state = State::Idle;
                } else if size != want || dragging {
                    self.state = State::Seen(want, Instant::now());
                } else if since.elapsed() >= Duration::from_millis(400) {
                    self.state = State::Pausing(size, self.frame + 20);
                    return true;
                }
                false
            }
            State::Pausing(size, at) => {
                if self.frame >= at {
                    // the request: AA last (non-zero AA marks it)
                    let cur: Vec<u32> = (0..6).map(|i| read(MODE + 4 * i)).collect();
                    unsafe {
                        for (i, v) in [(0, size.0), (1, size.1), (3, cur[3]), (4, cur[4]), (5, cur[5])] {
                            *((PENDING + 4 * i) as *mut u32) = v;
                        }
                        *((PENDING + 8) as *mut u32) = cur[2].max(1);
                    }
                    crate::control::log(&format!("resize: asking the game for {}x{}", size.0, size.1));
                    self.state = State::Asked(size, Instant::now());
                }
                true
            }
            State::Asked(size, since) => {
                if mode() == size || since.elapsed() >= Duration::from_secs(4) {
                    self.state = State::Settling(self.frame + 3);
                }
                true
            }
            State::Settling(until) => {
                if self.frame >= until {
                    self.state = State::Idle;
                    self.quiet_until = Some(Instant::now() + Duration::from_secs(2));
                    return false;
                }
                true
            }
        }
    }
}
