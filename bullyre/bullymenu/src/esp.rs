//! ESP overlay: a click-through window laid exactly over the game's client
//! area and kept directly above the game in the window order (so other
//! windows, like the menu, still cover it). Black is the transparent color
//! key; everything else drawn on it shows over the game.

use std::collections::HashMap;
use std::ptr::null_mut;

use crate::game::Game;
use crate::offsets as off;
use crate::win::*;

#[derive(Clone, Copy)]
pub struct Settings {
    pub enabled: bool,
    pub boxes: bool,
    pub health: bool,
    pub text: bool,
    pub snaplines: bool,
    pub vehicles: bool,
    pub max_dist: f32,
}

struct Camera {
    m: [f32; 16],
    tx: f32, // view window: tan of the horizontal / vertical half-angles
    ty: f32,
    w: f32,
    h: f32,
}

impl Camera {
    fn read(g: &Game, w: i32, h: i32) -> Option<Camera> {
        let mut b = [0u8; 64];
        if !g.read_bytes(off::CAMERA_VIEW, &mut b) {
            return None;
        }
        let m: [f32; 16] = std::array::from_fn(|i| f32::from_le_bytes([b[i * 4], b[i * 4 + 1], b[i * 4 + 2], b[i * 4 + 3]]));
        if m.iter().any(|v| !v.is_finite()) {
            return None;
        }
        // the RwCamera's view window, which already has the aspect ratio in it;
        // failing that, the FOV the scripts see, with the window's aspect ratio
        let rw = g.u32(off::RW_CAMERA).unwrap_or(0);
        let (tx, ty) = match (g.f32(rw + off::VIEW_WINDOW), g.f32(rw + off::VIEW_WINDOW + 4)) {
            (Some(x), Some(y)) if rw != 0 && (0.05..20.0).contains(&x) && (0.05..20.0).contains(&y) => (x, y),
            _ => {
                let fov = if g.u32(off::FOV_OVERRIDE_ON)? != 0 { g.f32(off::FOV_OVERRIDE)? } else { g.f32(off::FOV)? };
                if !(5.0..170.0).contains(&fov) {
                    return None;
                }
                let t = (fov.to_radians() / 2.0).tan();
                (t, t * h as f32 / w as f32)
            }
        };
        Some(Camera { m, tx, ty, w: w as f32, h: h as f32 })
    }

    /// World point -> overlay pixel. `view = M * p + t`: y is depth, z up, x left.
    fn project(&self, p: [f32; 3]) -> Option<(f32, f32)> {
        let m = &self.m;
        let x = m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12];
        let y = m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13];
        let z = m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14];
        if y < 0.2 {
            return None;
        }
        let sx = self.w / 2.0 - (x / y) / self.tx * (self.w / 2.0);
        let sy = self.h / 2.0 - (z / y) / self.ty * (self.h / 2.0);
        (sx > -self.w && sx < 2.0 * self.w && sy > -self.h && sy < 2.0 * self.h).then_some((sx, sy))
    }
}

struct Pool {
    objects: u32,
    map: Vec<u8>,
    stride: u32,
}

impl Pool {
    fn read(g: &Game, global: u32) -> Option<Pool> {
        let p = g.u32(global).filter(|&p| p != 0)?;
        let (objects, map, cap, stride) = (g.u32(p)?, g.u32(p + 4)?, g.u32(p + 8)?, g.u32(p + 12)?);
        if cap == 0 || cap > 4096 || stride == 0 || stride > 0x10000 {
            return None;
        }
        let mut m = vec![0u8; cap as usize];
        g.read_bytes(map, &mut m).then_some(Pool { objects, map: m, stride })
    }

    fn entries(&self) -> impl Iterator<Item = u32> + '_ {
        self.map.iter().enumerate().filter(|(_, f)| **f & 0x80 == 0).map(|(i, _)| self.objects + i as u32 * self.stride)
    }
}

fn f32_at(b: &[u8], o: u32) -> f32 {
    let o = o as usize;
    f32::from_le_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]])
}

fn u32_at(b: &[u8], o: u32) -> u32 {
    let o = o as usize;
    u32::from_le_bytes([b[o], b[o + 1], b[o + 2], b[o + 3]])
}

/// Entity position: the matrix's translation, or the inline position when there's no matrix.
fn position(g: &Game, ent: u32, head: &[u8]) -> Option<[f32; 3]> {
    let mat = u32_at(head, off::ENT_MATRIX);
    let mut b = [0u8; 12];
    if mat != 0 {
        if !g.read_bytes(mat + 0x30, &mut b) {
            return None;
        }
    } else {
        b.copy_from_slice(&head[off::ENT_POS as usize..off::ENT_POS as usize + 12]);
    }
    let _ = ent;
    let p = [f32_at(&b, 0), f32_at(&b, 4), f32_at(&b, 8)];
    p.iter().all(|v| v.is_finite() && v.abs() < 100_000.0).then_some(p)
}

fn dist(a: [f32; 3], b: [f32; 3]) -> f32 {
    ((a[0] - b[0]).powi(2) + (a[1] - b[1]).powi(2) + (a[2] - b[2]).powi(2)).sqrt()
}

pub struct Overlay {
    hwnd: HWND,
    dc: HDC,
    bmp: HGDIOBJ,
    old_bmp: HGDIOBJ,
    w: i32,
    h: i32,
    font: HGDIOBJ,
    pens: HashMap<(u32, i32), HGDIOBJ>,
    brushes: HashMap<u32, HGDIOBJ>,
    shown: bool,
}

unsafe extern "system" fn overlay_proc(h: HWND, m: u32, w: WPARAM, l: LPARAM) -> LRESULT {
    const HTTRANSPARENT: LRESULT = -1;
    if m == WM_NCHITTEST {
        return HTTRANSPARENT;
    }
    unsafe { DefWindowProcW(h, m, w, l) }
}

const SHADOW: u32 = rgb(1, 1, 1); // near-black: visible, unlike the 0,0,0 color key

impl Overlay {
    pub fn new(inst: HINSTANCE) -> Overlay {
        let class = wide("BullyMenuEspOverlay");
        unsafe {
            let wc = WNDCLASSEXW {
                cbSize: std::mem::size_of::<WNDCLASSEXW>() as u32,
                style: 0,
                lpfnWndProc: overlay_proc,
                cbClsExtra: 0,
                cbWndExtra: 0,
                hInstance: inst,
                hIcon: null_mut(),
                hCursor: null_mut(),
                hbrBackground: GetStockObject(4), // black brush
                lpszMenuName: std::ptr::null(),
                lpszClassName: class.as_ptr(),
                hIconSm: null_mut(),
            };
            RegisterClassExW(&wc);
            let hwnd = CreateWindowExW(
                WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                class.as_ptr(),
                wide("Bully ESP").as_ptr(),
                WS_POPUP,
                0,
                0,
                1,
                1,
                null_mut(),
                null_mut(),
                inst,
                null_mut(),
            );
            SetLayeredWindowAttributes(hwnd, 0, 0, LWA_COLORKEY);
            let screen = GetDC(null_mut());
            let dc = CreateCompatibleDC(screen);
            ReleaseDC(null_mut(), screen);
            let font = CreateFontW(-15, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, NONANTIALIASED_QUALITY, 0, wide("Consolas").as_ptr());
            SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            Overlay { hwnd, dc, bmp: null_mut(), old_bmp: null_mut(), w: 0, h: 0, font, pens: HashMap::new(), brushes: HashMap::new(), shown: false }
        }
    }

    pub fn hide(&mut self) {
        if self.shown {
            unsafe { ShowWindow(self.hwnd, SW_HIDE) };
            self.shown = false;
        }
    }

    fn pen(&mut self, color: u32, width: i32) -> HGDIOBJ {
        *self.pens.entry((color, width)).or_insert_with(|| unsafe { CreatePen(PS_SOLID, width, color) })
    }

    fn brush(&mut self, color: u32) -> HGDIOBJ {
        *self.brushes.entry(color).or_insert_with(|| unsafe { CreateSolidBrush(color) })
    }

    fn resize(&mut self, w: i32, h: i32) {
        if (w, h) == (self.w, self.h) {
            return;
        }
        unsafe {
            if !self.bmp.is_null() {
                SelectObject(self.dc, self.old_bmp);
                DeleteObject(self.bmp);
            }
            let screen = GetDC(null_mut());
            self.bmp = CreateCompatibleBitmap(screen, w, h);
            ReleaseDC(null_mut(), screen);
            self.old_bmp = SelectObject(self.dc, self.bmp);
        }
        (self.w, self.h) = (w, h);
    }

    fn rect(&mut self, l: f32, t: f32, r: f32, b: f32, color: u32) {
        let (l, t, r, b) = (l as i32, t as i32, r as i32, b as i32);
        unsafe {
            SelectObject(self.dc, GetStockObject(NULL_BRUSH));
            SelectObject(self.dc, self.pen(SHADOW, 4));
            Rectangle(self.dc, l, t, r, b);
            SelectObject(self.dc, self.pen(color, 2));
            Rectangle(self.dc, l, t, r, b);
        }
    }

    fn fill(&mut self, l: i32, t: i32, r: i32, b: i32, color: u32) {
        let br = self.brush(color);
        unsafe { FillRect(self.dc, &RECT { left: l, top: t, right: r, bottom: b }, br) };
    }

    fn line(&mut self, x0: f32, y0: f32, x1: f32, y1: f32, color: u32) {
        unsafe {
            SelectObject(self.dc, self.pen(color, 1));
            MoveToEx(self.dc, x0 as i32, y0 as i32, null_mut());
            LineTo(self.dc, x1 as i32, y1 as i32);
        }
    }

    fn text(&mut self, x: f32, y: f32, s: &str, color: u32) {
        let w: Vec<u16> = s.encode_utf16().collect();
        let (x, y) = (x as i32 - (w.len() as i32 * 8) / 2, y as i32);
        unsafe {
            SetTextColor(self.dc, SHADOW);
            for (dx, dy) in [(-1, 0), (1, 0), (0, -1), (0, 1)] {
                TextOutW(self.dc, x + dx, y + dy, w.as_ptr(), w.len() as i32);
            }
            SetTextColor(self.dc, color);
            TextOutW(self.dc, x, y, w.as_ptr(), w.len() as i32);
        }
    }

    fn health_color(frac: f32) -> u32 {
        let f = frac.clamp(0.0, 1.0);
        if f > 0.5 { rgb(((1.0 - f) * 2.0 * 255.0) as u8, 255, 40) } else { rgb(255, (f * 2.0 * 255.0) as u8, 40) }
    }

    /// Redraws the overlay over the game, or hides it when there's nothing to show.
    pub fn update(&mut self, game: Option<&Game>, s: &Settings, menu: HWND) {
        let Some(g) = game.filter(|_| s.enabled) else { return self.hide() };
        let Some(gw) = g.window() else { return self.hide() };
        unsafe {
            let fg = GetForegroundWindow();
            if IsIconic(gw) != 0 || (fg != gw && fg != menu && fg != self.hwnd) {
                return self.hide();
            }
            let mut c = RECT::default();
            GetClientRect(gw, &mut c);
            let mut o = POINT::default();
            ClientToScreen(gw, &mut o);
            let (w, h) = (c.right, c.bottom);
            if w < 50 || h < 50 {
                return self.hide();
            }
            // just above the game: below whatever window is above it
            let above = GetWindow(gw, GW_HWNDPREV);
            let (after, z) = if above == self.hwnd { (null_mut(), SWP_NOZORDER) } else if above.is_null() { (HWND_TOP, 0) } else { (above, 0) };
            SetWindowPos(self.hwnd, after, o.x, o.y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW | z);
            self.shown = true;
            self.resize(w, h);
            self.fill(0, 0, w, h, 0);
            self.draw(g, s, w, h);
            let dst = GetDC(self.hwnd);
            BitBlt(dst, 0, 0, w, h, self.dc, 0, 0, SRCCOPY);
            ReleaseDC(self.hwnd, dst);
        }
    }

    fn draw(&mut self, g: &Game, s: &Settings, w: i32, h: i32) {
        let Some(cam) = Camera::read(g, w, h) else { return };
        let player = g.u32(off::PLAYER).unwrap_or(0);
        let mut head = vec![0u8; 0x1D00];
        let me = if player != 0 && g.read_bytes(player, &mut head) { position(g, player, &head) } else { None };
        let my_vehicle = if player != 0 { u32_at(&head, off::PED_VEHICLE) } else { 0 };
        let mut count = 0;

        if let Some(pool) = Pool::read(g, off::PED_POOL) {
            let mut buf = vec![0u8; 0x1D00];
            for ped in pool.entries() {
                if ped == player || !g.read_bytes(ped, &mut buf) {
                    continue;
                }
                let Some(p) = position(g, ped, &buf) else { continue };
                let d = me.map_or(0.0, |m| dist(m, p));
                if d > s.max_dist {
                    continue;
                }
                // the position is at the feet; peds are about 1.75 m tall
                let (Some(top), Some(bot)) = (cam.project([p[0], p[1], p[2] + 1.75]), cam.project(p)) else { continue };
                count += 1;
                let hp = f32_at(&buf, off::PED_HEALTH);
                let max = f32_at(&buf, off::PED_MAX_HEALTH).max(1.0);
                let dead = buf[off::PED_DEAD as usize] != 0 || hp <= 0.0;
                let frac = if dead { 0.0 } else { hp / max };
                let color = if dead { rgb(150, 150, 150) } else { Self::health_color(frac) };
                let bh = (bot.1 - top.1).max(4.0);
                let bw = bh * 0.4;
                let cx = (top.0 + bot.0) / 2.0;
                let (l, r) = (cx - bw / 2.0, cx + bw / 2.0);
                if s.snaplines {
                    self.line(w as f32 / 2.0, h as f32, cx, bot.1, color);
                }
                if s.boxes {
                    self.rect(l, top.1, r, bot.1, color);
                }
                if s.health && !dead {
                    let (bx, t, b) = (l as i32 - 7, top.1 as i32, bot.1 as i32);
                    self.fill(bx - 1, t - 1, bx + 4, b + 1, SHADOW);
                    let filled = b - ((b - t) as f32 * frac.clamp(0.0, 1.0)) as i32;
                    self.fill(bx, filled, bx + 3, b, color);
                }
                if s.text {
                    let label = if dead { format!("dead {d:.0}m") } else { format!("{hp:.0}/{max:.0} {d:.0}m") };
                    self.text(cx, bot.1 + 3.0, &label, rgb(235, 235, 235));
                }
            }
        }

        if s.vehicles
            && let Some(pool) = Pool::read(g, off::VEHICLE_POOL)
        {
            let mut buf = vec![0u8; 0x400];
            for veh in pool.entries() {
                if veh == my_vehicle || !g.read_bytes(veh, &mut buf) {
                    continue;
                }
                let Some(p) = position(g, veh, &buf) else { continue };
                let d = me.map_or(0.0, |m| dist(m, p));
                if d > s.max_dist {
                    continue;
                }
                let (Some(top), Some(bot)) = (cam.project([p[0], p[1], p[2] + 1.5]), cam.project(p)) else { continue };
                let bh = (bot.1 - top.1).max(4.0);
                let bw = bh * 1.6;
                let cx = (top.0 + bot.0) / 2.0;
                let color = rgb(80, 170, 255);
                if s.boxes {
                    self.rect(cx - bw / 2.0, top.1, cx + bw / 2.0, bot.1, color);
                }
                if s.snaplines {
                    self.line(w as f32 / 2.0, h as f32, cx, bot.1, color);
                }
                if s.text {
                    let hp = f32_at(&buf, off::VEH_HEALTH);
                    let model = u16::from_le_bytes([buf[off::ENT_MODEL as usize], buf[off::ENT_MODEL as usize + 1]]);
                    self.text(cx, bot.1 + 3.0, &format!("vehicle #{model} {hp:.0}hp {d:.0}m"), color);
                }
            }
        }

        self.text(90.0, 8.0, &format!("ESP: {count} peds"), rgb(255, 220, 90));
    }
}

impl Drop for Overlay {
    fn drop(&mut self) {
        unsafe {
            for p in self.pens.values().chain(self.brushes.values()) {
                DeleteObject(*p);
            }
            if !self.bmp.is_null() {
                SelectObject(self.dc, self.old_bmp);
                DeleteObject(self.bmp);
            }
            DeleteObject(self.font);
            DeleteDC(self.dc);
        }
    }
}
