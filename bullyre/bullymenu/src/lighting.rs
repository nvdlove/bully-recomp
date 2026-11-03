//! Lighting: the game blends its lighting every frame from the active timecycle
//! (24 hours x 21 weathers of keyframes). This keeps a copy of the game's own
//! table and writes back a version with the user's adjustments applied, so the
//! change holds at every hour and in every weather. If the game reloads the
//! table (season change, loading a save), that becomes the new base.

use crate::game::Game;
use crate::offsets as off;

#[derive(Clone, Copy, PartialEq, Debug)]
pub enum Filter {
    Game, // the game's own full-screen color filter
    Off,
    Tint, // replace it with a color multiply
}

#[derive(Clone, PartialEq, Debug)]
pub struct Lighting {
    pub enabled: bool,
    pub ambient: f32,       // multiplier on the three ambient lights
    pub warmth: f32,        // -1 (cool) .. 1 (warm), on ambient and sun
    pub sun: f32,           // multiplier on sun + back light
    pub sky: f32,           // multiplier on the sky gradient
    pub contrast: f32,      // spreads (>1) or narrows (<1) the light ranges
    pub filter: Filter,
    pub tint: [u8; 3],
    pub tint_strength: f32, // 0..1
    pub draw_distance: f32, // multiplier on the far clip plane
    pub fog: f32,           // -1 (fog closer) .. 1 (fog pushed back)
}

impl Default for Lighting {
    fn default() -> Lighting {
        Lighting {
            enabled: false,
            ambient: 1.0,
            warmth: 0.0,
            sun: 1.0,
            sky: 1.0,
            contrast: 1.0,
            filter: Filter::Game,
            tint: [255, 240, 220],
            tint_strength: 0.3,
            draw_distance: 1.0,
            fog: 0.0,
        }
    }
}

fn f(v: &str, d: f32) -> f32 {
    v.parse().ok().filter(|x: &f32| x.is_finite()).unwrap_or(d)
}

impl Lighting {
    /// "l_amb=1.2|l_sun=0.9|..." pairs (the same format the page and preset file use).
    pub fn set(&mut self, k: &str, v: &str) -> bool {
        let d = Lighting::default();
        match k {
            "l_on" => self.enabled = v == "1",
            "l_amb" => self.ambient = f(v, d.ambient).clamp(0.0, 3.0),
            "l_warm" => self.warmth = f(v, 0.0).clamp(-1.0, 1.0),
            "l_sun" => self.sun = f(v, d.sun).clamp(0.0, 3.0),
            "l_sky" => self.sky = f(v, d.sky).clamp(0.0, 3.0),
            "l_con" => self.contrast = f(v, 1.0).clamp(0.3, 2.0),
            "l_filter" => {
                self.filter = match v {
                    "off" => Filter::Off,
                    "tint" => Filter::Tint,
                    _ => Filter::Game,
                }
            }
            "l_tint" => {
                if let Ok(c) = u32::from_str_radix(v.trim_start_matches('#'), 16) {
                    self.tint = [(c >> 16) as u8, (c >> 8) as u8, c as u8];
                }
            }
            "l_tstr" => self.tint_strength = f(v, d.tint_strength).clamp(0.0, 1.0),
            "l_dist" => self.draw_distance = f(v, 1.0).clamp(0.25, 4.0),
            "l_fog" => self.fog = f(v, 0.0).clamp(-1.0, 1.0),
            _ => return false,
        }
        true
    }

    pub fn to_kv(&self) -> String {
        let filter = match self.filter {
            Filter::Game => "game",
            Filter::Off => "off",
            Filter::Tint => "tint",
        };
        let [r, g, b] = self.tint;
        format!(
            "l_on={}|l_amb={:.2}|l_warm={:.2}|l_sun={:.2}|l_sky={:.2}|l_con={:.2}|l_filter={filter}|l_tint=#{r:02x}{g:02x}{b:02x}|l_tstr={:.2}|l_dist={:.2}|l_fog={:.2}",
            self.enabled as u8, self.ambient, self.warmth, self.sun, self.sky, self.contrast, self.tint_strength, self.draw_distance, self.fog
        )
    }

    /// One keyframe with the adjustments applied.
    fn apply(&self, r: &mut [u8]) {
        let scale = |v: u8, m: f32| (v as f32 * m).round().clamp(0.0, 255.0) as u8;
        // warmth: push red up and blue down (or the reverse), green barely moves
        let warm = [1.0 + 0.30 * self.warmth, 1.0 + 0.06 * self.warmth, 1.0 - 0.30 * self.warmth];
        let mut tint = |base: usize, m: f32, warmed: bool| {
            for (c, w) in warm.iter().enumerate() {
                r[base + c] = scale(r[base + c], if warmed { m * w } else { m });
            }
        };
        for light in 0..3 {
            tint(off::TC_AMBIENT + light * 3, self.ambient, true);
        }
        tint(off::TC_SUN, self.sun, true);
        tint(off::TC_BACK, self.sun, true);
        tint(off::TC_SKY_TOP, self.sky, false);
        tint(off::TC_SKY_BOTTOM, self.sky, false);
        if self.contrast != 1.0 {
            for pair in 0..3 {
                let o = off::TC_RANGES + pair * 8;
                let lo = f32::from_le_bytes(r[o..o + 4].try_into().unwrap());
                let hi = f32::from_le_bytes(r[o + 4..o + 8].try_into().unwrap());
                let mid = (lo + hi) / 2.0;
                let half = (hi - lo) / 2.0 * self.contrast;
                r[o..o + 4].copy_from_slice(&(mid - half).clamp(0.0, 2.0).to_le_bytes());
                r[o + 4..o + 8].copy_from_slice(&(mid + half).clamp(0.0, 2.0).to_le_bytes());
            }
        }
        match self.filter {
            Filter::Game => {}
            Filter::Off => r[off::TC_FILTER_MODE] = 0,
            Filter::Tint => {
                // stored blue, green, red: a warm tint written R,G,B came out cool
                r[off::TC_FILTER_MODE] = 1;
                for (i, c) in [2, 1, 0].into_iter().enumerate() {
                    let v = 255.0 + (self.tint[c] as f32 - 255.0) * self.tint_strength;
                    r[off::TC_FILTER_RGBA + i] = v.round().clamp(0.0, 255.0) as u8;
                }
                r[off::TC_FILTER_RGBA + 3] = 255;
            }
        }
        if self.draw_distance != 1.0 || self.fog != 0.0 {
            let far0 = i16::from_le_bytes([r[off::TC_FAR_CLIP], r[off::TC_FAR_CLIP + 1]]) as f32;
            let fog0 = i16::from_le_bytes([r[off::TC_FOG_START], r[off::TC_FOG_START + 1]]) as f32;
            let far = (far0 * self.draw_distance).clamp(10.0, 3000.0);
            let fog = (fog0 * self.draw_distance + self.fog * far * 0.8).clamp(-3000.0, far - 1.0);
            r[off::TC_FAR_CLIP..off::TC_FAR_CLIP + 2].copy_from_slice(&(far as i16).to_le_bytes());
            r[off::TC_FOG_START..off::TC_FOG_START + 2].copy_from_slice(&(fog as i16).to_le_bytes());
            if self.draw_distance > 1.0 {
                let nf = i32::from_le_bytes(r[off::TC_NEAR_FAR..off::TC_NEAR_FAR + 4].try_into().unwrap());
                let nf = (nf as f32 * self.draw_distance).min(20000.0) as i32;
                r[off::TC_NEAR_FAR..off::TC_NEAR_FAR + 4].copy_from_slice(&nf.to_le_bytes());
            }
        }
    }
}

/// A keyframe the way the game's tables look (checked before anything is written).
fn plausible(r: &[u8]) -> bool {
    let far = i16::from_le_bytes([r[off::TC_FAR_CLIP], r[off::TC_FAR_CLIP + 1]]);
    let nf = i32::from_le_bytes(r[off::TC_NEAR_FAR..off::TC_NEAR_FAR + 4].try_into().unwrap());
    r[off::TC_FILTER_MODE] <= 2 && (10..=5000).contains(&far) && (100..=20000).contains(&nf)
}

/// The game's original table, saved with the id of the game process it came from,
/// so a menu restarted while its lighting is still applied doesn't take that as
/// the original.
fn saved_base_path() -> std::path::PathBuf {
    std::env::temp_dir().join("bullymenu_lighting_base.bin")
}

fn load_saved_base(pid: u32) -> Option<Vec<u8>> {
    let b = std::fs::read(saved_base_path()).ok()?;
    (b.len() == 4 + off::TC_RECORDS * off::TC_SIZE && b[..4] == pid.to_le_bytes()).then(|| b[4..].to_vec())
}

fn save_base(pid: u32, base: &[u8]) {
    let _ = std::fs::write(saved_base_path(), [&pid.to_le_bytes()[..], base].concat());
}

#[derive(Default)]
pub struct Engine {
    base: Option<Vec<u8>>,    // the game's own table
    written: Option<Vec<u8>>, // what we last wrote over it
}

impl Engine {
    pub fn forget(&mut self) {
        *self = Engine::default();
    }

    /// Keeps the game's table in line with `want`. Call a few times a second.
    pub fn tick(&mut self, g: &Game, want: &Lighting) -> Result<(), String> {
        let mut cur = vec![0u8; off::TC_RECORDS * off::TC_SIZE];
        if !g.read_bytes(off::TIMECYCLE, &mut cur) {
            return Err("can't read the lighting table".into());
        }
        if !want.enabled {
            // put the game's own table back, unless the game already replaced ours
            if let (Some(w), Some(base)) = (&self.written, &self.base)
                && *w == cur
                && !g.write_bytes(off::TIMECYCLE, base)
            {
                return Err("can't write the lighting table".into());
            }
            self.forget();
            return Ok(());
        }
        let reloaded = self.written.as_ref().is_some_and(|w| *w != cur);
        if reloaded {
            // the game loaded a new table over ours: that is the new base
            self.base = None;
        }
        if self.base.is_none() {
            // most slots are real keyframes; the rest (unused weathers) are left alone
            let good = cur.chunks(off::TC_SIZE).filter(|r| plausible(r)).count();
            if good < off::TC_RECORDS / 2 {
                return Err("lighting table not recognized (different game version?)".into());
            }
            let base = match (reloaded, load_saved_base(g.pid)) {
                (false, Some(saved)) => saved,
                _ => cur.clone(),
            };
            save_base(g.pid, &base);
            self.base = Some(base);
        }
        let mut out = self.base.clone().unwrap();
        for r in out.chunks_mut(off::TC_SIZE).filter(|r| plausible(r)) {
            want.apply(r);
        }
        if out != cur && !g.write_bytes(off::TIMECYCLE, &out) {
            return Err("can't write the lighting table".into());
        }
        self.written = Some(out);
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trip() {
        let mut a = Lighting { enabled: true, ambient: 1.3, warmth: -0.4, filter: Filter::Tint, tint: [10, 200, 30], ..Default::default() };
        a.fog = 0.25;
        let mut b = Lighting::default();
        for kv in a.to_kv().split('|') {
            let (k, v) = kv.split_once('=').unwrap();
            assert!(b.set(k, v));
        }
        assert_eq!(a, b);
    }

    #[test]
    fn neutral_changes_nothing() {
        let mut r = vec![0u8; off::TC_SIZE];
        for (i, v) in r.iter_mut().enumerate() {
            *v = (i * 7) as u8;
        }
        r[off::TC_FILTER_MODE] = 2;
        let before = r.clone();
        Lighting { enabled: true, ..Default::default() }.apply(&mut r);
        assert_eq!(r, before);
    }
}
