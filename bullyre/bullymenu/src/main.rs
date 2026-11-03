//! Bully menu: launches Bully windowed (without touching the desktop
//! resolution), unlocks the frame rate, draws an ESP overlay and restyles the
//! lighting, and customises the new renderer (bullygfx). The menu itself is a frameless WebView; the page sends its state
//! over `window.ipc` and Rust pushes status back with `evaluate_script`.

#![windows_subsystem = "windows"]

mod esp;
mod game;
mod gfx;
mod intercom;
mod lighting;
mod mods;
mod viewer;
mod offsets;
mod payload;
mod win;

use std::path::PathBuf;
use std::time::{Duration, Instant};

use game::{Game, LaunchOptions};
use lighting::Lighting;
use tao::dpi::LogicalSize;
use tao::event::{Event, StartCause, WindowEvent};
use tao::event_loop::{ControlFlow, EventLoopBuilder};
use tao::platform::windows::{WindowBuilderExtWindows, WindowExtWindows};
use tao::window::WindowBuilder;
use wry::WebViewBuilder;

const UNLIMITED: u32 = 1000;
/// The most the game gets until it has been played for `IN_GAME_AFTER`: launched with a
/// higher cap it hangs during start-up, set once in the game it runs fine.
const STARTUP_FPS: u32 = 60;
const IN_GAME_AFTER: Duration = Duration::from_secs(3);
const FRAME_EVERY: Duration = Duration::from_millis(16);
const TICK_EVERY: Duration = Duration::from_millis(400);
const DEFAULT_PRESET: &str = "Game default";
/// Rows the Live Data explorer shows at a time.
const VIEWER_PAGE: usize = 80;

enum UserEvent {
    Ipc(String),
}

/// Everything the page controls; sent in full on every change.
struct Ui {
    windowed: bool,
    width: u32,
    height: u32,
    background: bool,
    free_mouse: bool,
    /// launch through bullyre's own exe (game::HOST_EXE) instead of the game's
    host: bool,
    /// launch the 64-bit remaster (game::X64_EXE beside the menu)
    x64: bool,
    /// its seamless doors (off: `--no-seamless-doors`, the game's own doors with their fade)
    seamless: bool,
    /// the game's folder ("": Steam's)
    game_dir: String,
    fps: u32,
    unlimited: bool,
    esp: esp::Settings,
    light: Lighting,
    ic: intercom::Config,
    gfx: gfx::Settings,
}

impl Default for Ui {
    fn default() -> Ui {
        Ui {
            windowed: true,
            width: 1600,
            height: 900,
            background: true,
            free_mouse: true,
            host: false,
            x64: have_x64(),
            seamless: true,
            game_dir: String::new(),
            fps: 60,
            unlimited: false,
            esp: esp::Settings { enabled: false, boxes: true, health: true, text: true, snaplines: false, vehicles: true, max_dist: 150.0 },
            light: Lighting::default(),
            ic: intercom::Config::default(),
            gfx: gfx::Settings::default(),
        }
    }
}

impl Ui {
    /// "key=value|key=value|..."
    fn update(&mut self, kvs: &str) {
        for kv in kvs.split('|') {
            let Some((k, v)) = kv.split_once('=') else { continue };
            let on = v == "1";
            let n = v.parse::<u32>().ok();
            match k {
                "windowed" => self.windowed = on,
                "w" => self.width = n.unwrap_or(self.width).clamp(640, 7680),
                "h" => self.height = n.unwrap_or(self.height).clamp(480, 4320),
                "bg" => self.background = on,
                "mouse" => self.free_mouse = on,
                "host" => self.host = on,
                "x64" => self.x64 = on,
                "seamless" => self.seamless = on,
                "gamedir" => {
                    self.game_dir = v.trim().to_string();
                    game::set_game_dir(&self.game_dir);
                }
                "fps" => self.fps = n.unwrap_or(self.fps).clamp(30, 240),
                "unl" => self.unlimited = on,
                "esp" => self.esp.enabled = on,
                "boxes" => self.esp.boxes = on,
                "health" => self.esp.health = on,
                "text" => self.esp.text = on,
                "snap" => self.esp.snaplines = on,
                "veh" => self.esp.vehicles = on,
                "range" => self.esp.max_dist = n.unwrap_or(150) as f32,
                _ => {
                    if !self.gfx.set(k, v) && !self.ic.set(k, v) {
                        self.light.set(k, v);
                    }
                }
            }
        }
    }

    fn to_kv(&self) -> String {
        let b = |x: bool| x as u8;
        let e = &self.esp;
        format!(
            "windowed={}|w={}|h={}|bg={}|mouse={}|host={}|x64={}|seamless={}|gamedir={}|fps={}|unl={}|esp={}|boxes={}|health={}|text={}|snap={}|veh={}|range={}|{}|{}|{}",
            b(self.windowed),
            self.width,
            self.height,
            b(self.background),
            b(self.free_mouse),
            b(self.host),
            b(self.x64),
            b(self.seamless),
            self.game_dir,
            self.fps,
            b(self.unlimited),
            b(e.enabled),
            b(e.boxes),
            b(e.health),
            b(e.text),
            b(e.snaplines),
            b(e.vehicles),
            e.max_dist as u32,
            self.light.to_kv(),
            self.ic.to_kv(),
            self.gfx.to_kv()
        )
    }

    fn fps_cap(&self) -> u32 {
        if self.unlimited { UNLIMITED } else { self.fps }
    }
}

/// Settings and presets: next to the exe when they're there already (a menu kept in a folder of
/// its own), else in %LOCALAPPDATA%\BullyRemaster (the menu one file, nothing made beside it).
fn data_file(name: &str) -> PathBuf {
    let beside = beside_exe("bullymenu_settings.txt");
    if beside.exists() {
        return beside.with_file_name(name);
    }
    let home = payload::home();
    let _ = std::fs::create_dir_all(&home);
    home.join(name)
}

/// A file next to the menu's exe.
fn beside_exe(name: &str) -> PathBuf {
    std::env::current_exe().ok().and_then(|p| p.parent().map(|d| d.join(name))).unwrap_or_else(|| PathBuf::from(name))
}

/// The 64-bit game can be launched: bundled, or its exe beside the menu.
fn have_x64() -> bool {
    payload::available() || beside_exe(game::X64_EXE).exists()
}

struct Presets {
    list: Vec<(String, String)>, // name, lighting key=values
    selected: String,
}

impl Presets {
    fn load() -> Presets {
        let text = std::fs::read_to_string(data_file("bullymenu_presets.txt")).unwrap_or_default();
        let list = text
            .lines()
            .filter_map(|l| l.split_once('|'))
            .filter(|(n, _)| !n.trim().is_empty() && *n != DEFAULT_PRESET)
            .map(|(n, kv)| (n.trim().to_string(), kv.to_string()))
            .collect();
        Presets { list, selected: DEFAULT_PRESET.into() }
    }

    fn save(&self) {
        let text: String = self.list.iter().map(|(n, kv)| format!("{n}|{kv}\n")).collect();
        let _ = std::fs::write(data_file("bullymenu_presets.txt"), text);
    }

    fn names(&self) -> Vec<String> {
        std::iter::once(DEFAULT_PRESET.to_string()).chain(self.list.iter().map(|(n, _)| n.clone())).collect()
    }

    fn get(&self, name: &str) -> String {
        if name == DEFAULT_PRESET {
            return Lighting::default().to_kv();
        }
        self.list.iter().find(|(n, _)| n == name).map(|(_, kv)| kv.clone()).unwrap_or_default()
    }
}

/// Frame rate and game speed, sampled from the game's timer.
#[derive(Default)]
struct Speed {
    since: Option<(Instant, u32)>, // start of the window: time, frame counter
    last_frame: u32,
    step_sum: f32, // time steps seen on new, unpaused frames
    steps: u32,
    paused: bool,
    shown: Option<(f32, f32)>, // fps, speed (1.0 = real time)
}

impl Speed {
    fn sample(&mut self, g: &Game, now: Instant) {
        let Some((frame, step)) = g.timer() else { return };
        if frame != self.last_frame {
            self.last_frame = frame;
            self.paused = step <= 0.0;
            if !self.paused {
                self.step_sum += step;
                self.steps += 1;
            }
        }
        let (t0, f0) = *self.since.get_or_insert((now, frame));
        let secs = (now - t0).as_secs_f32();
        if secs >= 1.0 {
            let fps = frame.wrapping_sub(f0) as f32 / secs;
            self.shown = Some((fps, if self.steps > 0 { self.step_sum / self.steps as f32 * fps / 50.0 } else { 0.0 }));
            *self = Speed { since: Some((now, frame)), last_frame: frame, shown: self.shown, paused: self.paused, ..Default::default() };
        }
    }
}

/// Holds the cursor in the game window only while the game is being played.
#[derive(Default)]
struct Cursor {
    clipped: Option<win::RECT>,
}

impl Cursor {
    fn update(&mut self, g: Option<&Game>, enabled: bool) {
        let want = g.filter(|g| enabled && g.mouse_is_shared()).and_then(|g| {
            let w = g.window()?;
            unsafe {
                if win::GetForegroundWindow() != w || win::IsIconic(w) != 0 || g.paused() != Some(false) {
                    return None;
                }
                let mut c = win::RECT::default();
                win::GetClientRect(w, &mut c);
                let mut o = win::POINT::default();
                win::ClientToScreen(w, &mut o);
                let r = win::RECT { left: o.x, top: o.y, right: o.x + c.right, bottom: o.y + c.bottom };
                // Only take the cursor once it's in the picture: clipping it while it's on
                // the title bar yanks it down, and a window being dragged with it.
                let mut p = win::POINT::default();
                win::GetCursorPos(&mut p);
                let inside = (r.left..r.right).contains(&p.x) && (r.top..r.bottom).contains(&p.y);
                (inside || self.clipped.is_some()).then_some(r)
            }
        });
        if want != self.clipped {
            unsafe { win::ClipCursor(want.as_ref().map_or(std::ptr::null(), |r| r as *const win::RECT)) };
            self.clipped = want;
        }
    }

    fn release(&mut self) {
        if self.clipped.take().is_some() {
            unsafe { win::ClipCursor(std::ptr::null()) };
        }
    }
}

struct App {
    ui: Ui,
    presets: Presets,
    speed: Speed,
    cursor: Cursor,
    light: lighting::Engine,
    light_error: String,
    game: Option<Game>,
    framed_once: bool,
    frame_wanted: bool, // this game was started (or attached) windowed
    last_error: String,
    input_tried: bool, // the live input fix ran on this game
    intercom: intercom::Intercom,
    in_dorm: bool,
    playing_since: Option<Instant>, // the player has been in the running game since
    in_game: bool,                  // ... long enough for the full frame cap
    last_cap: Option<u32>,          // the cap this menu last wrote
    gfx_link: gfx::Link,
    gfx_status: gfx::Status,
    /// The Live Data page: the dumper's tables (loaded when first opened), the rows
    /// the page shows, whether it's open.
    viewer_db: Option<Result<viewer::Db, String>>,
    viewer_rows: Vec<usize>,
    viewer_on: bool,
    /// The game's resolution when the window was last fitted to it.
    last_res: Option<(u32, u32)>,
    /// The Mods page: why the last folder couldn't be added.
    mods_error: String,

}

impl App {
    fn launch(&mut self) {
        if self.game.as_ref().is_some_and(|g| g.alive()) {
            return;
        }
        let x64 = self.ui.x64;
        let installed = if x64 {
            if !payload::available() {
                gfx::install_x64();
            }
            Ok(())
        } else {
            gfx::install_bullyre();
            gfx::install()
        };
        self.gfx_link.write(&self.ui.gfx);
        let u = &self.ui;
        // ("--host" on the command line: this run only, for the tests; never saved)
        let host = u.host || std::env::args().any(|a| a == "--host");
        let opts = LaunchOptions { windowed: u.windowed, width: u.width, height: u.height, background: u.background, free_mouse: u.free_mouse, host, x64, seamless: u.seamless };
        match Game::launch(&opts) {
            Ok(g) => {
                self.game = Some(g);
                self.last_error = installed.err().unwrap_or_default();
            }
            Err(e) => {
                // "started, but ..." still leaves a running game to attach to
                self.game = Game::find();
                self.last_error = e;
            }
        }
        self.light.forget();
        self.input_tried = false;
        self.framed_once = false;
        self.frame_wanted = opts.windowed && !opts.x64; // (the 64-bit game's window is its own)
        self.forget_fps();
        self.apply_fps();
    }

    /// The Launch page's game folder: where it is, and what's wrong with it ("" if nothing).
    fn gamedir_script(&self) -> String {
        let dir = game::game_dir();
        let mut problem = game::check_game_dir(&dir).unwrap_or_default();
        if problem.is_empty() && self.ui.x64 && !have_x64() {
            problem = format!("this menu doesn't carry the 64-bit game ({} isn't beside it either)", game::X64_EXE);
        }
        format!("setGameDir({}, {}, {})", js(&dir.to_string_lossy()), js(&problem), have_x64())
    }

    fn forget_fps(&mut self) {
        self.playing_since = None;
        self.in_game = false;
        self.last_cap = None;
    }

    /// The cap the game should run at now.
    fn fps_target(&self) -> u32 {
        if self.in_game { self.ui.fps_cap() } else { self.ui.fps_cap().min(STARTUP_FPS) }
    }

    /// Writes the frame cap, except over the music class's own one (its rhythm game
    /// is timed to it).
    fn apply_fps(&mut self) {
        let Some(g) = &self.game else { return };
        let want = self.fps_target();
        let cur = g.fps_cap();
        if cur == Some(offsets::MUSIC_CLASS_FPS) && self.last_cap != Some(offsets::MUSIC_CLASS_FPS) {
            return;
        }
        if cur != Some(want) && g.set_fps_cap(want) {
            self.last_cap = Some(want);
        }
    }

    /// Unlocks the full frame cap once the player has been in the game for a moment.
    fn track_in_game(&mut self, now: Instant) {
        let Some(g) = &self.game else { return };
        if self.in_game {
            return;
        }
        if !g.has_player() || g.paused() != Some(false) {
            self.playing_since = None;
        } else if now - *self.playing_since.get_or_insert(now) >= IN_GAME_AFTER {
            self.in_game = true;
        }
    }

    /// Follows the game, keeps its window framed, the fps cap and the lighting set.
    fn tick(&mut self) {
        if self.game.as_ref().is_some_and(|g| !g.alive()) {
            self.game = None;
            self.light.forget();
        }
        if self.game.is_none()
            && let Some(g) = Game::find()
        {
            // a game we didn't start: apply what can be applied live (to the 32-bit one: the
            // 64-bit one's code isn't the game's, its window its own)
            self.frame_wanted = !g.x64 && self.ui.windowed && game::reg_read("WIN") == Some(1);
            let errs = if g.x64 { Vec::new() } else { g.runtime_patches(self.frame_wanted, self.ui.background) };
            self.last_error = errs.join("; ");
            self.framed_once = false;
            self.light.forget();
            self.input_tried = false;
            self.forget_fps();
            self.game = Some(g);
        }
        self.in_dorm = self.game.as_ref().and_then(|g| g.area()) == Some(offsets::AREA_BOYS_DORM);
        self.intercom.set_allowed(!self.ui.ic.dorm_only || self.in_dorm);
        self.track_in_game(Instant::now());
        self.apply_fps();
        let Some(g) = &self.game else { return };
        // free the mouse and the Windows key in a running game (once; the patch marks it done)
        if !g.x64 && self.ui.free_mouse && !g.mouse_is_shared() && g.window().is_some() && !self.input_tried {
            self.input_tried = true;
            if let Err(e) = g.free_input() {
                self.last_error = e;
            }
        }
        if let (Some(w), Some((rw, rh))) = (g.window(), g.resolution())
            && self.frame_wanted
            && unsafe { win::IsIconic(w) } == 0
            && unsafe { win::IsHungAppWindow(w) } == 0 // restyling a hung window would hang the menu too
        {
            let maximized = unsafe { win::IsZoomed(w) } != 0;
            if !game::is_styled(w) || !self.framed_once {
                game::frame_window(w, rw, rh, !self.framed_once);
                self.framed_once = true;
                self.last_res = Some((rw, rh));
            } else if self.last_res != Some((rw, rh)) {
                // the game switched resolution (bullygfx asks it when the window is
                // resized): the frame fits it exactly, and the size is kept for next time
                if !maximized {
                    game::frame_window(w, rw, rh, false);
                }
                self.last_res = Some((rw, rh));
                self.ui.width = rw;
                self.ui.height = rh;
                self.save_settings();
            }
        }
        self.light_error = self.light.tick(g, &self.ui.light).err().unwrap_or_default();
    }

    fn status_script(&self) -> String {
        let (running, info, mouse_note) = match &self.game {
            None => (false, "Launch it from here, or start it and it'll be picked up".to_string(), String::new()),
            Some(g) => {
                let mut info = format!("pid {}", g.pid);
                if let Some((w, h)) = g.resolution() {
                    info += &format!(" · {w}×{h}");
                }
                info += if g.launched { " · launched here" } else { " · attached" };
                if g.x64 {
                    info += " · 64-bit";
                }
                let note = if !g.x64 && self.ui.free_mouse && !g.mouse_is_shared() && self.input_tried {
                    "Couldn't free the mouse in this game: close it and launch it from here.".to_string()
                } else {
                    String::new()
                };
                (true, info, note)
            }
        };
        format!("setStatus({running}, {}, {}, {}, {})", js(&info), js(&self.last_error), js(&self.light_error), js(&mouse_note))
    }

    fn stats_script(&self) -> String {
        match (&self.game, self.speed.shown) {
            (Some(_), Some((fps, speed))) => format!("setStats({fps:.1}, {speed:.3}, {}, {})", self.speed.paused, self.fps_held()),
            _ => format!("setStats(null, null, false, {})", self.fps_held()),
        }
    }

    /// The cap is held at `STARTUP_FPS` for now.
    fn fps_held(&self) -> bool {
        self.game.is_some() && self.fps_target() < self.ui.fps_cap()
    }

    /// The Live Data page's totals and groups (or why it has none).
    fn viewer_meta_script(&self) -> String {
        match &self.viewer_db {
            Some(Ok(db)) => {
                let groups: Vec<String> = db.groups.iter().map(|g| js(g)).collect();
                format!(
                    "setViewerMeta({{offsets:{},natives:{},functions:{},classes:{},shaders:613,groups:[{}]}})",
                    db.rows.len(),
                    db.natives,
                    db.functions,
                    db.classes,
                    groups.join(",")
                )
            }
            Some(Err(e)) => format!("setViewerMeta({{error:{}}})", js(e)),
            None => "setViewerMeta({error:\"not loaded\"})".into(),
        }
    }

    /// The live cards and the shown rows' values.
    fn viewer_live_scripts(&self) -> Vec<String> {
        let Some(g) = &self.game else { return vec!["setLive(null)".into()] };
        let mut out = vec![format!("setLive({})", viewer::dashboard(g))];
        if let Some(Ok(db)) = &self.viewer_db {
            let bases = viewer::Bases::of(g);
            let vals: Vec<String> = self.viewer_rows.iter().map(|&i| js(&viewer::value(g, &bases, &db.rows[i]))).collect();
            out.push(format!("setViewerValues([{}])", vals.join(",")));
        }
        out
    }

    /// The Renderer page: whether it's on and how it's doing.
    fn gfx_script(&self) -> String {
        let s = &self.gfx_status;
        let running = self.game.is_some();
        format!(
            "setGfx({running}, {}, {}, {}, {}, {}, {}, {}, {})",
            js(if running { &s.state } else { "" }),
            js(&s.gpu),
            js(&s.ms),
            js(&s.named),
            js(&s.replaced),
            js(&s.exported),
            js(&s.weather),
            js(&s.wetness)
        )
    }

    /// The Mods page: [folder, on, what it changes or its error], the last live change.
    fn mods_script(&self) -> String {
        let (report, applied) = mods::status();
        let list: Vec<String> = mods::load()
            .iter()
            .map(|m| {
                let info = report.iter().find(|r| r.0.eq_ignore_ascii_case(&m.path)).map_or("", |r| r.1.as_str());
                format!("[{},{},{}]", js(&m.path), m.on, js(info))
            })
            .collect();
        format!("setMods([{}], {}, {}, {})", list.join(","), self.game.is_some(), js(&applied), js(&self.mods_error))
    }

    fn intercom_script(&self) -> String {
        format!("setIntercom({}, {}, {})", self.intercom.talking(), self.in_dorm, js(&self.intercom.error()))
    }

    fn mics_script(&self) -> String {
        let names: Vec<String> = intercom::mics().iter().map(|n| js(n)).collect();
        format!("setMics([{}], {})", names.join(","), self.ui.ic.mic)
    }

    fn presets_script(&self) -> String {
        let names: Vec<String> = self.presets.names().iter().map(|n| js(n)).collect();
        format!("setPresets([{}], {})", names.join(","), js(&self.presets.selected))
    }

    fn save_settings(&self) {
        let _ = std::fs::write(data_file("bullymenu_settings.txt"), format!("{}|preset={}\n", self.ui.to_kv(), self.presets.selected));
    }

    /// Leaves the game as it was: original lighting, free cursor.
    fn detach(&mut self) {
        if let Some(g) = &self.game {
            let mut off = self.ui.light.clone();
            off.enabled = false;
            let _ = self.light.tick(g, &off);
        }
        self.cursor.release();
    }
}

/// A JS string literal.
fn js(s: &str) -> String {
    let mut o = String::from("\"");
    for c in s.chars() {
        match c {
            '"' => o += "\\\"",
            '\\' => o += "\\\\",
            '\n' => o += "\\n",
            '\r' => {}
            '<' => o += "\\u003c",
            c if (c as u32) < 0x20 => o += &format!("\\u{:04x}", c as u32),
            c => o.push(c),
        }
    }
    o + "\""
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if let Some(i) = args.iter().position(|a| a == "--render-intercom")
        && let (Some(input), Some(output)) = (args.get(i + 1), args.get(i + 2))
    {
        std::process::exit(match intercom::render_file(input, output) {
            Ok(()) => 0,
            Err(_) => 1,
        });
    }
    if args.iter().any(|a| a == "--unpack") {
        // the bundled 64-bit game unpacked, nothing else (checking the bundle): the result in
        // %LOCALAPPDATA%\BullyRemaster\unpack.txt
        let result = match payload::unpack() {
            Ok(dir) => format!("ok {}", dir.display()),
            Err(e) => format!("failed: {e}"),
        };
        let _ = std::fs::create_dir_all(payload::home());
        let _ = std::fs::write(payload::home().join("unpack.txt"), result);
        return;
    }
    if args.iter().any(|a| a == "--live-dump") {
        // what the Live Data page shows, from the running game (for checking it)
        let query = args.iter().skip_while(|a| *a != "--live-dump").nth(1).cloned().unwrap_or_default();
        let mut out = String::new();
        match (viewer::Db::load(), Game::find()) {
            (Ok(db), Some(g)) => {
                out += &format!("meta offsets {} natives {} functions {} classes {} groups {}
", db.rows.len(), db.natives, db.functions, db.classes, db.groups.len());
                out += &format!("live {}
", viewer::dashboard(&g));
                let bases = viewer::Bases::of(&g);
                let rows = db.query(&query, "");
                out += &format!("query {query:?}: {} rows
", rows.len());
                for &i in rows.iter().take(25) {
                    let r = &db.rows[i];
                    out += &format!("  {:28} {:32} {:10} = {}
", r.name, r.addr, r.ty, viewer::value(&g, &bases, r));
                }
            }
            (Err(e), _) => out += &format!("no data: {e}
"),
            (_, None) => out += "game not running
",
        }
        let _ = std::fs::write(data_file("live_dump.txt"), out);
        return;
    }
    let event_loop = EventLoopBuilder::<UserEvent>::with_user_event().build();
    // --hidden: no window on screen (automated test launches; the menu still runs)
    let hidden = std::env::args().any(|a| a == "--hidden");
    let window = WindowBuilder::new()
        .with_title("Bully Menu")
        .with_inner_size(LogicalSize::new(820.0, 600.0))
        .with_resizable(false)
        .with_decorations(false)
        .with_undecorated_shadow(true)
        .with_visible(!hidden)
        .with_skip_taskbar(hidden)
        .build(&event_loop)
        .expect("create window");

    let proxy = event_loop.create_proxy();
    // (the WebView's data in %LOCALAPPDATA%\BullyRemaster, not a folder beside the exe)
    let mut web_context = wry::WebContext::new(Some(payload::home().join("WebView2")));
    let webview = WebViewBuilder::new_with_web_context(&mut web_context)
        .with_html(include_str!("ui.html"))
        .with_background_color((49, 51, 56, 255))
        .with_ipc_handler(move |req| {
            let _ = proxy.send_event(UserEvent::Ipc(req.into_body()));
        })
        .build(&window)
        .expect("create webview (is the WebView2 runtime installed?)");

    let inst = unsafe { win::GetModuleHandleW(std::ptr::null()) };
    let mut overlay = esp::Overlay::new(inst);
    let menu_hwnd = window.hwnd() as win::HWND;

    let mut app = App {
        ui: Ui::default(),
        presets: Presets::load(),
        speed: Speed::default(),
        cursor: Cursor::default(),
        light: lighting::Engine::default(),
        light_error: String::new(),
        game: None,
        framed_once: false,
        frame_wanted: false,
        last_error: String::new(),
        input_tried: false,
        intercom: intercom::Intercom::default(),
        in_dorm: false,
        playing_since: None,
        in_game: false,
        last_cap: None,
        gfx_link: gfx::Link::default(),
        gfx_status: gfx::Status::default(),
        viewer_db: None,
        viewer_rows: Vec::new(),
        viewer_on: false,
        last_res: None,
        mods_error: String::new(),
    };
    // last session's settings, else the game's own saved window size
    let saved = std::fs::read_to_string(data_file("bullymenu_settings.txt")).unwrap_or_default();
    let saved = saved.trim();
    if !saved.is_empty() {
        app.ui.update(saved);
        if let Some(p) = saved.split('|').find_map(|kv| kv.strip_prefix("preset=")) {
            app.presets.selected = p.to_string();
        }
    } else {
        // nothing saved: the renderer's settings as the game folder has them, the window's size as
        // the game saved it
        app.ui.gfx.read_file();
        if game::reg_read("WIN") == Some(1)
            && let (Some(w), Some(h)) = (game::reg_read("RESW"), game::reg_read("RESH"))
            && w >= 640
            && h >= 480
        {
            app.ui.width = w;
            app.ui.height = h;
        }
    }
    if std::env::args().any(|a| a == "--esp") {
        app.ui.esp.enabled = true;
    }
    app.intercom.configure(&app.ui.ic);
    if std::env::args().any(|a| a == "--launch") {
        app.launch(); // with the saved settings, as if Launch was clicked
    }

    let mut next_tick = Instant::now();
    let mut last_status = String::new();
    let mut last_stats = String::new();
    let mut last_ic = String::new();
    let mut last_gfx = String::new();
    let mut last_mods = String::new();
    event_loop.run(move |event, _, flow| {
        let now = Instant::now();
        match event {
            Event::UserEvent(UserEvent::Ipc(msg)) => {
                let (cmd, arg) = msg.split_once('|').unwrap_or((msg.as_str(), ""));
                match cmd {
                    "ready" => {
                        let _ = webview.evaluate_script(&app.mics_script());
                        let _ = webview.evaluate_script(&format!("loadState({})", js(&app.ui.to_kv())));
                        let _ = webview.evaluate_script(&app.gamedir_script());
                        let _ = webview.evaluate_script(&app.presets_script());
                        last_status.clear();
                        last_stats.clear();
                        last_ic.clear();
                        last_gfx.clear();
                        last_mods.clear();
                        next_tick = now;
                    }
                    "state" => {
                        let x64 = app.ui.x64;
                        app.ui.update(arg);
                        if app.ui.x64 != x64 {
                            let _ = webview.evaluate_script(&app.gamedir_script());
                        }
                        app.gfx_link.write(&app.ui.gfx);
                        app.intercom.configure(&app.ui.ic);
                        app.intercom.set_allowed(!app.ui.ic.dorm_only || app.in_dorm);
                        app.apply_fps();
                        app.save_settings();
                        next_tick = next_tick.min(now + Duration::from_millis(60)); // lighting follows the sliders quickly
                    }
                    "weather" | "weather_demo" | "weather_dry" => {
                        let gfx = &mut app.ui.gfx;
                        match cmd {
                            "weather" => {
                                gfx.weather = arg.parse().unwrap_or(0);
                                gfx.weather_seq += 1;
                            }
                            "weather_demo" => gfx.demo_seq += 1,
                            _ => gfx.dry_seq += 1,
                        }
                        app.gfx_link.write(&app.ui.gfx);
                        app.save_settings();
                    }
                    "gfx_export" | "gfx_reload" => {
                        if cmd == "gfx_export" {
                            app.ui.gfx.export += 1;
                        } else {
                            app.ui.gfx.reload += 1;
                        }
                        app.gfx_link.write(&app.ui.gfx);
                        app.save_settings();
                    }
                    "viewer" => {
                        app.viewer_on = arg == "1";
                        if app.viewer_on {
                            if app.viewer_db.is_none() {
                                app.viewer_db = Some(viewer::Db::load());
                            }
                            let _ = webview.evaluate_script(&app.viewer_meta_script());
                            next_tick = now;
                        }
                    }
                    "viewer_query" => {
                        // filter \t group \t first row
                        let mut p = arg.split('\t');
                        let (filter, group, first) = (p.next().unwrap_or(""), p.next().unwrap_or(""), p.next().and_then(|x| x.parse::<usize>().ok()).unwrap_or(0));
                        if let Some(Ok(db)) = &app.viewer_db {
                            let all = db.query(filter, group);
                            let total = all.len();
                            app.viewer_rows = all.into_iter().skip(first).take(VIEWER_PAGE).collect();
                            let rows: Vec<String> = app
                                .viewer_rows
                                .iter()
                                .map(|&i| {
                                    let r = &db.rows[i];
                                    format!("[{},{},{},{},{}]", js(&r.name), js(&r.group), js(&r.addr), js(&r.ty), js(&r.notes))
                                })
                                .collect();
                            let _ = webview.evaluate_script(&format!("setViewerRows({total}, {first}, [{}])", rows.join(",")));
                            next_tick = now;
                        }
                    }
                    "mods_add" => {
                        app.mods_error.clear();
                        if let Some(folder) = mods::pick_folder(menu_hwnd) {
                            let path = folder.display().to_string();
                            let mut list = mods::load();
                            if !mods::looks_like_mod(&folder) {
                                app.mods_error = format!("No mod files in {path}: pick the folder with the mod's World.img (or its textures)");
                            } else if list.iter().any(|m| m.path.eq_ignore_ascii_case(&path)) {
                                app.mods_error = "That mod is already in the list".into();
                            } else {
                                list.push(mods::Mod { path, on: true });
                                mods::save(&list);
                            }
                        }
                        next_tick = now;
                    }
                    "mods_toggle" | "mods_remove" | "mods_open" | "mods_up" => {
                        // index | on
                        let (i, v) = arg.split_once('|').unwrap_or((arg, ""));
                        let mut list = mods::load();
                        if let Ok(i) = i.parse::<usize>()
                            && i < list.len()
                        {
                            match cmd {
                                "mods_toggle" => list[i].on = v == "1",
                                "mods_remove" => {
                                    list.remove(i);
                                }
                                "mods_up" => {
                                    if i > 0 {
                                        list.swap(i, i - 1);
                                    }
                                }
                                _ => {
                                    let _ = std::process::Command::new("explorer").arg(&list[i].path).spawn();
                                }
                            }
                            if cmd != "mods_open" {
                                mods::save(&list);
                            }
                        }
                        app.mods_error.clear();
                        next_tick = now;
                    }
                    "gfx_open" => {
                        let _ = std::process::Command::new("explorer").arg(gfx::textures_dir()).spawn();
                    }
                    "launch" => {
                        app.launch();
                        next_tick = now;
                    }
                    "browse" => {
                        let start = game::game_dir().to_string_lossy().to_string();
                        if let Some(dir) = win::pick_folder(menu_hwnd, "Pick the Bully: Scholarship Edition folder (the one with Bully.exe)", &start) {
                            app.ui.game_dir = dir;
                            game::set_game_dir(&app.ui.game_dir);
                            app.save_settings();
                        }
                        let _ = webview.evaluate_script(&app.gamedir_script());
                    }
                    "preset_load" => {
                        app.presets.selected = arg.to_string();
                        let kv = app.presets.get(arg);
                        app.ui.update(&kv);
                        let _ = webview.evaluate_script(&format!("loadState({})", js(&kv)));
                        let _ = webview.evaluate_script(&app.presets_script());
                        app.save_settings();
                    }
                    "preset_save" => {
                        let name = arg.trim().to_string();
                        if !name.is_empty() && name != DEFAULT_PRESET {
                            let mut l = app.ui.light.clone();
                            l.enabled = true;
                            let kv = l.to_kv();
                            match app.presets.list.iter_mut().find(|(n, _)| *n == name) {
                                Some(p) => p.1 = kv,
                                None => app.presets.list.push((name.clone(), kv)),
                            }
                            app.presets.selected = name;
                            app.presets.save();
                            app.save_settings();
                            let _ = webview.evaluate_script(&app.presets_script());
                        }
                    }
                    "preset_delete" => {
                        app.presets.list.retain(|(n, _)| n != arg);
                        app.presets.selected = DEFAULT_PRESET.into();
                        app.presets.save();
                        app.save_settings();
                        let _ = webview.evaluate_script(&app.presets_script());
                    }
                    "drag" => {
                        let _ = window.drag_window();
                    }
                    "min" => window.set_minimized(true),
                    "close" => {
                        app.intercom.stop();
                        app.detach();
                        overlay.hide();
                        *flow = ControlFlow::Exit;
                        return;
                    }
                    _ => {}
                }
            }
            Event::NewEvents(StartCause::ResumeTimeReached { .. }) | Event::NewEvents(StartCause::Init) => {
                overlay.update(app.game.as_ref(), &app.ui.esp, menu_hwnd);
                app.cursor.update(app.game.as_ref(), app.ui.free_mouse);
                if let Some(g) = &app.game {
                    app.speed.sample(g, now);
                }
                let ic = app.intercom_script();
                if ic != last_ic {
                    let _ = webview.evaluate_script(&ic);
                    last_ic = ic;
                }
                if now >= next_tick {
                    next_tick = now + TICK_EVERY;
                    app.tick();
                    app.gfx_status = gfx::status();
                    if app.viewer_on {
                        for script in app.viewer_live_scripts() {
                            let _ = webview.evaluate_script(&script);
                        }
                    }
                    for (script, last) in [(app.status_script(), &mut last_status), (app.stats_script(), &mut last_stats), (app.gfx_script(), &mut last_gfx), (app.mods_script(), &mut last_mods)] {
                        if script != *last {
                            let _ = webview.evaluate_script(&script);
                            *last = script;
                        }
                    }
                }
            }
            Event::WindowEvent { event: WindowEvent::CloseRequested, .. } => {
                app.intercom.stop();
                app.detach();
                overlay.hide();
                *flow = ControlFlow::Exit;
                return;
            }
            _ => {}
        }
        *flow = ControlFlow::WaitUntil(Instant::now() + FRAME_EVERY);
    });
}
