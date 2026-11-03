//! The new renderer (bullygfx, the d3d9.dll next to the game) and its customiser.
//! The DLL watches `bullygfx_settings.txt` in the game folder and writes how it's
//! doing to `bullygfx_status.txt`; this side writes one and reads the other.

use std::path::{Path, PathBuf};

use crate::game::game_dir;

#[derive(Clone, Debug, PartialEq)]
pub struct Settings {
    pub enabled: bool,
    pub exposure: f32,
    pub contrast: f32,
    pub saturation: f32,
    pub gamma: f32,
    pub tint: String, // #rrggbb
    pub tint_strength: f32,
    pub vignette: f32,
    pub sharpen: f32,
    pub textures: String, // game | replaced | checker | white
    /// Counters: raising one asks the renderer to do it once.
    pub export: u32,
    pub reload: u32,
    /// Horizontal FOV at 16:9, degrees (the game's is 85).
    pub fov: f32,
    /// Fit the view to the window's aspect (ultrawide sees more at the sides).
    pub hor_plus: bool,
    /// Wet streets: off | auto (when it rains) | always
    pub wet: String,
    pub wet_amount: f32,
    /// Weather to set (0 clear, 1 overcast, 2 rain); counters: raising one asks for it.
    pub weather: i32,
    pub weather_seq: u32,
    pub demo_seq: u32,
    pub dry_seq: u32,
}

impl Default for Settings {
    fn default() -> Settings {
        Settings {
            enabled: false,
            exposure: 1.0,
            contrast: 1.0,
            saturation: 1.0,
            gamma: 1.0,
            tint: "#ffe0b0".into(),
            tint_strength: 0.0,
            vignette: 0.0,
            sharpen: 0.0,
            textures: "game".into(),
            export: 0,
            reload: 0,
            fov: 85.0,
            hor_plus: true,
            wet: "auto".into(),
            wet_amount: 0.85,
            weather: 0,
            weather_seq: 0,
            demo_seq: 0,
            dry_seq: 0,
        }
    }
}

fn f(v: &str, default: f32) -> f32 {
    v.parse::<f32>().ok().filter(|x| x.is_finite()).unwrap_or(default)
}

impl Settings {
    /// One key=value from the page; false if it isn't ours.
    pub fn set(&mut self, k: &str, v: &str) -> bool {
        let d = Settings::default();
        match k {
            "g_on" => self.enabled = v == "1",
            "g_exp" => self.exposure = f(v, d.exposure).clamp(0.2, 3.0),
            "g_con" => self.contrast = f(v, d.contrast).clamp(0.3, 2.0),
            "g_sat" => self.saturation = f(v, d.saturation).clamp(0.0, 2.5),
            "g_gam" => self.gamma = f(v, d.gamma).clamp(0.4, 2.5),
            "g_tint" if v.len() == 7 && v.starts_with('#') => self.tint = v.to_string(),
            "g_tstr" => self.tint_strength = f(v, 0.0).clamp(0.0, 1.0),
            "g_vig" => self.vignette = f(v, 0.0).clamp(0.0, 1.0),
            "g_sharp" => self.sharpen = f(v, 0.0).clamp(0.0, 2.0),
            "g_tex" if matches!(v, "game" | "replaced" | "checker" | "white") => self.textures = v.to_string(),
            "g_fov" => self.fov = f(v, d.fov).clamp(50.0, 130.0),
            "g_horp" => self.hor_plus = v == "1",
            "g_wet" if matches!(v, "off" | "auto" | "always") => self.wet = v.to_string(),
            "g_wetamt" => self.wet_amount = f(v, d.wet_amount).clamp(0.0, 1.0),
            "g_weather" => self.weather = v.parse().unwrap_or(self.weather),
            "g_wseq" => self.weather_seq = v.parse().unwrap_or(self.weather_seq),
            "g_dseq" => self.demo_seq = v.parse().unwrap_or(self.demo_seq),
            "g_dryseq" => self.dry_seq = v.parse().unwrap_or(self.dry_seq),
            "g_export" => self.export = v.parse().unwrap_or(self.export),
            "g_reload" => self.reload = v.parse().unwrap_or(self.reload),
            _ => return false,
        }
        true
    }

    pub fn to_kv(&self) -> String {
        format!(
            "g_on={}|g_exp={}|g_con={}|g_sat={}|g_gam={}|g_tint={}|g_tstr={}|g_vig={}|g_sharp={}|g_tex={}|g_export={}|g_reload={}|g_fov={}|g_horp={}|g_wet={}|g_wetamt={}|g_weather={}|g_wseq={}|g_dseq={}|g_dryseq={}",
            self.enabled as u8,
            self.exposure,
            self.contrast,
            self.saturation,
            self.gamma,
            self.tint,
            self.tint_strength,
            self.vignette,
            self.sharpen,
            self.textures,
            self.export,
            self.reload,
            self.fov,
            self.hor_plus as u8,
            self.wet,
            self.wet_amount,
            self.weather,
            self.weather_seq,
            self.demo_seq,
            self.dry_seq
        )
    }

    /// What the game folder's bullygfx_settings.txt already says (a menu with nothing saved
    /// starts from it, so launching doesn't put someone's tuning back to the defaults).
    pub fn read_file(&mut self) -> bool {
        let Ok(text) = std::fs::read_to_string(game_file("bullygfx_settings.txt")) else { return false };
        for line in text.lines() {
            let Some((k, v)) = line.split_once('=') else { continue };
            let key = match k.trim() {
                "renderer" => "g_on",
                "exposure" => "g_exp",
                "contrast" => "g_con",
                "saturation" => "g_sat",
                "gamma" => "g_gam",
                "tint" => "g_tint",
                "tint_strength" => "g_tstr",
                "vignette" => "g_vig",
                "sharpen" => "g_sharp",
                "textures" => "g_tex",
                "fov" => "g_fov",
                "hor_plus" => "g_horp",
                "wet" => "g_wet",
                "wet_amount" => "g_wetamt",
                _ => continue,
            };
            self.set(key, v.trim());
        }
        true
    }

    /// What the renderer reads.
    fn file_text(&self) -> String {
        format!(
            "renderer={}\nexposure={}\ncontrast={}\nsaturation={}\ngamma={}\ntint={}\ntint_strength={}\nvignette={}\nsharpen={}\ntextures={}\nexport={}\nreload={}\nfov={}\nhor_plus={}\nwet={}\nwet_amount={}\nweather={}\nweather_seq={}\ndemo_seq={}\ndry_seq={}\n",
            self.enabled as u8,
            self.exposure,
            self.contrast,
            self.saturation,
            self.gamma,
            self.tint,
            self.tint_strength,
            self.vignette,
            self.sharpen,
            self.textures,
            self.export,
            self.reload,
            self.fov,
            self.hor_plus as u8,
            self.wet,
            self.wet_amount,
            self.weather,
            self.weather_seq,
            self.demo_seq,
            self.dry_seq
        )
    }
}

pub fn game_file(name: &str) -> PathBuf {
    game_dir().join(name)
}

/// Puts the newest builds of the 64-bit remaster beside the menu, when they're around
/// (../bullyre/build64/Release and ../bullygfx's x86_64 build next to the menu's folder):
/// bullyre_x64.exe (and its .map, for crash reports) and bullygfx64.dll. Without them the
/// copies already beside the menu are used.
pub fn install_x64() {
    let Some(menu_dir) = crate::beside_exe("x").parent().map(Path::to_path_buf) else { return };
    let builds: [(&[&str], &str); 3] = [
        (&["bullyre", "build64", "Release", "bullyre_x64.exe"], "bullyre_x64.exe"),
        (&["bullyre", "build64", "Release", "bullyre_x64.map"], "bullyre_x64.map"),
        (&["bullygfx", "target", "x86_64-pc-windows-msvc", "release", "d3d9.dll"], "bullygfx64.dll"),
    ];
    for (parts, name) in builds {
        let Some(built) = menu_dir.ancestors().map(|d| parts.iter().fold(d.to_path_buf(), |p, s| p.join(s))).find(|p| p.exists()) else {
            continue;
        };
        let installed = menu_dir.join(name);
        let newer = match (std::fs::metadata(&built).and_then(|m| m.modified()), std::fs::metadata(&installed).and_then(|m| m.modified())) {
            (Ok(x), Ok(y)) => x > y,
            (Ok(_), Err(_)) => true,
            _ => false,
        };
        if newer {
            let _ = std::fs::copy(&built, &installed);
        }
    }
}

/// Writes the settings for the renderer when they changed.
#[derive(Default)]
pub struct Link {
    written: Option<String>,
}

impl Link {
    pub fn write(&mut self, s: &Settings) {
        let text = s.file_text();
        if self.written.as_deref() == Some(text.as_str()) {
            return;
        }
        if std::fs::write(game_file("bullygfx_settings.txt"), &text).is_ok() {
            self.written = Some(text);
        }
    }
}

/// What the renderer says about itself.
#[derive(Default, Clone, PartialEq)]
pub struct Status {
    pub state: String,
    pub gpu: String,
    pub ms: String,
    pub named: String,
    pub replaced: String,
    pub exported: String,
    pub weather: String,
    pub wetness: String,
}

pub fn status() -> Status {
    let text = std::fs::read_to_string(game_file("bullygfx_status.txt")).unwrap_or_default();
    let mut s = Status::default();
    for line in text.lines() {
        let Some((k, v)) = line.split_once('=') else { continue };
        let v = v.trim().to_string();
        match k {
            "state" => s.state = v,
            "gpu" => s.gpu = v,
            "ms" => s.ms = v,
            "named" => s.named = v,
            "replaced" => s.replaced = v,
            "exported" => s.exported = v,
            "weather" => s.weather = v,
            "wetness" => s.wetness = v,
            _ => {}
        }
    }
    s
}

/// The renderer's texture folders (made if missing).
pub fn textures_dir() -> PathBuf {
    let dir = game_file("bullygfx_textures");
    let _ = std::fs::create_dir_all(dir.join("export"));
    let _ = std::fs::create_dir_all(dir.join("replace"));
    dir
}

/// Puts the newest build of bullyre (the reversed game code) next to the game, when
/// there is one (../bullyre/build/Release next to the menu): bullyre.dll, and the host
/// exe that runs the game with it built in (bullyre_host.exe).
pub fn install_bullyre() {
    let mut starts: Vec<PathBuf> = Vec::new();
    if let Ok(p) = std::env::current_exe() {
        starts.push(p);
    }
    if let Ok(p) = std::env::current_dir() {
        starts.push(p);
    }
    for file in ["bullyre.dll", "bullyre_host.exe"] {
        let Some(built) = starts.iter().flat_map(|s| s.ancestors().map(|d| d.join("bullyre").join("build").join("Release").join(file))).find(|p| p.exists()) else {
            continue;
        };
        let installed = game_file(file);
        let newer = match (std::fs::metadata(&built).and_then(|m| m.modified()), std::fs::metadata(&installed).and_then(|m| m.modified())) {
            (Ok(x), Ok(y)) => x > y,
            (Ok(_), Err(_)) => true,
            _ => false,
        };
        if newer {
            let _ = std::fs::copy(&built, &installed);
        }
    }
}

/// Puts the newest build of the renderer next to the game (before it starts: the
/// running game has the dll loaded). Looks for ../bullygfx next to the menu.
pub fn install() -> Result<(), String> {
    let mut starts: Vec<PathBuf> = Vec::new();
    if let Ok(p) = std::env::current_exe() {
        starts.push(p);
    }
    if let Ok(p) = std::env::current_dir() {
        starts.push(p);
    }
    let built = starts
        .iter()
        .flat_map(|s| s.ancestors().map(|d| d.join("bullygfx").join("target").join("i686-pc-windows-msvc").join("release").join("d3d9.dll")))
        .find(|p| p.exists());
    let Some(built) = built else {
        return if game_file("d3d9.dll").exists() { Ok(()) } else { Err("the new renderer (bullygfx's d3d9.dll) isn't built".into()) };
    };
    let installed = game_file("d3d9.dll");
    let newer = |a: &Path, b: &Path| match (std::fs::metadata(a).and_then(|m| m.modified()), std::fs::metadata(b).and_then(|m| m.modified())) {
        (Ok(x), Ok(y)) => x > y,
        (Ok(_), Err(_)) => true,
        _ => false,
    };
    if newer(&built, &installed) {
        std::fs::copy(&built, &installed).map_err(|e| format!("couldn't install the new renderer: {e}"))?;
        // symbols, for the crash reports
        let pdb = built.with_extension("pdb");
        if pdb.exists() {
            let _ = std::fs::copy(&pdb, game_file("d3d9.pdb"));
        }
    }
    Ok(())
}
