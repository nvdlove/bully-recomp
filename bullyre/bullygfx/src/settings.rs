//! The customiser's settings, from bullymenu: `bullygfx_settings.txt` next to the game
//! (key=value per line), read again whenever it changes. What the renderer is doing
//! goes back in `bullygfx_status.txt`.

use std::path::PathBuf;
use std::time::SystemTime;

/// How game textures are shown.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
pub enum TextureMode {
    /// The game's own.
    #[default]
    Game,
    /// A PNG from bullygfx_textures/replace wherever one has the texture's name.
    Replaced,
    /// A checkerboard everywhere (shows the UVs and that replacing works).
    Checker,
    /// White: lighting and vertex colours only.
    White,
}

#[derive(Clone, Debug, PartialEq)]
pub struct Settings {
    /// The new renderer, alone (D3D9 draws nothing).
    pub renderer: bool,
    // the picture, on the finished frame
    pub exposure: f32,
    pub contrast: f32,
    pub saturation: f32,
    pub gamma: f32,
    pub tint: [f32; 3],
    pub tint_strength: f32,
    pub vignette: f32,
    pub sharpen: f32,
    /// FXAA on the finished frame (the game has no anti-aliasing).
    pub aa: bool,
    /// Anisotropic filtering for the game's mipmapped, linearly filtered textures (1: as the
    /// game asks: most of the world doesn't).
    pub aniso: u16,
    /// Bloom: how much of the bright parts' glow is added (0 off), and from what brightness.
    pub bloom: f32,
    pub bloom_threshold: f32,
    pub textures: TextureMode,
    /// Counters: a change asks for the action once.
    pub export: u32,
    pub reload: u32,
    /// Horizontal FOV in degrees at 16:9 (the game's is 85).
    pub fov: f32,
    /// Fit the view to the screen's aspect (ultrawide sees more at the sides).
    pub hor_plus: bool,
    /// Wet streets: 0 off, 1 when it rains, 2 always.
    pub wet: u8,
    pub wet_amount: f32,
    /// Weather to set (0 clear, 1 overcast, 2 rain) when `weather_seq` changes; the rain
    /// demo (overcast, then rain) when `demo_seq` does; dry streets on `dry_seq`.
    pub weather: i32,
    pub weather_seq: u32,
    pub demo_seq: u32,
    pub dry_seq: u32,
}

impl Default for Settings {
    fn default() -> Settings {
        Settings {
            renderer: false,
            exposure: 1.0,
            contrast: 1.0,
            saturation: 1.0,
            gamma: 1.0,
            tint: [1.0; 3],
            tint_strength: 0.0,
            vignette: 0.0,
            sharpen: 0.0,
            aa: true,
            aniso: 16,
            bloom: 0.3,
            bloom_threshold: 0.8,
            textures: TextureMode::Game,
            export: 0,
            reload: 0,
            fov: 85.0,
            hor_plus: true,
            wet: 1,
            wet_amount: 0.8,
            weather: 0,
            weather_seq: 0,
            demo_seq: 0,
            dry_seq: 0,
        }
    }
}

impl Settings {
    fn parse(text: &str) -> Settings {
        let mut s = Settings::default();
        for line in text.lines() {
            let Some((k, v)) = line.split_once('=') else { continue };
            let (k, v) = (k.trim(), v.trim());
            let f = v.parse::<f32>().ok().filter(|x| x.is_finite());
            match k {
                "renderer" => s.renderer = v == "1",
                "exposure" => s.exposure = f.unwrap_or(1.0).clamp(0.0, 4.0),
                "contrast" => s.contrast = f.unwrap_or(1.0).clamp(0.0, 3.0),
                "saturation" => s.saturation = f.unwrap_or(1.0).clamp(0.0, 3.0),
                "gamma" => s.gamma = f.unwrap_or(1.0).clamp(0.2, 3.0),
                "tint" => {
                    // #rrggbb
                    if let Ok(c) = u32::from_str_radix(v.trim_start_matches('#'), 16) {
                        s.tint = [(c >> 16) as u8, (c >> 8) as u8, c as u8].map(|b| b as f32 / 255.0);
                    }
                }
                "tint_strength" => s.tint_strength = f.unwrap_or(0.0).clamp(0.0, 1.0),
                "vignette" => s.vignette = f.unwrap_or(0.0).clamp(0.0, 1.0),
                "sharpen" => s.sharpen = f.unwrap_or(0.0).clamp(0.0, 2.0),
                "aa" => s.aa = !matches!(v, "0" | "off" | "none"),
                "aniso" => s.aniso = v.parse::<u16>().unwrap_or(16).clamp(1, 16),
                "bloom" => s.bloom = f.unwrap_or(0.3).clamp(0.0, 2.0),
                "bloom_threshold" => s.bloom_threshold = f.unwrap_or(0.8).clamp(0.0, 1.0),
                "textures" => {
                    s.textures = match v {
                        "replaced" => TextureMode::Replaced,
                        "checker" => TextureMode::Checker,
                        "white" => TextureMode::White,
                        _ => TextureMode::Game,
                    }
                }
                "fov" => s.fov = f.unwrap_or(85.0).clamp(40.0, 150.0),
                "hor_plus" => s.hor_plus = v != "0",
                "wet" => {
                    s.wet = match v {
                        "off" => 0,
                        "always" => 2,
                        _ => 1,
                    }
                }
                "wet_amount" => s.wet_amount = f.unwrap_or(0.8).clamp(0.0, 1.0),
                "weather" => s.weather = v.parse().unwrap_or(0),
                "weather_seq" => s.weather_seq = v.parse().unwrap_or(0),
                "demo_seq" => s.demo_seq = v.parse().unwrap_or(0),
                "dry_seq" => s.dry_seq = v.parse().unwrap_or(0),
                "export" => s.export = v.parse().unwrap_or(0),
                "reload" => s.reload = v.parse().unwrap_or(0),
                _ => {}
            }
        }
        s
    }
}

fn file(name: &str) -> PathBuf {
    crate::win::game_file(name)
}

/// Where the texture folders are: bullygfx_textures/{export,replace}.
pub fn textures_dir() -> PathBuf {
    file("bullygfx_textures")
}

/// Watches the settings file.
#[derive(Default)]
pub struct Watcher {
    seen: Option<SystemTime>,
    frames: u32,
}

impl Watcher {
    /// Every 30th call: the settings, if the file changed.
    pub fn poll(&mut self) -> Option<Settings> {
        self.frames += 1;
        if self.frames % 30 != 1 {
            return None;
        }
        let path = file("bullygfx_settings.txt");
        let modified = std::fs::metadata(&path).and_then(|m| m.modified()).ok()?;
        if self.seen == Some(modified) {
            return None;
        }
        self.seen = Some(modified);
        std::fs::read_to_string(&path).ok().map(|t| Settings::parse(&t))
    }
}

/// What the menu shows about the renderer.
#[derive(Default, Clone, PartialEq)]
pub struct Status {
    pub state: String, // "off", "on", "failed: ..."
    pub gpu: String,
    pub ms: f32,
    pub named: usize,
    pub replaced: usize,
    pub exported: String,
    pub weather: i32,
    /// Wet streets, 0..1.
    pub wetness: f32,
}

pub fn write_status(s: &Status) {
    let text = format!(
        "state={}\ngpu={}\nms={:.1}\nnamed={}\nreplaced={}\nexported={}\nweather={}\nwetness={:.2}\n",
        s.state.replace('\n', " "),
        s.gpu,
        s.ms,
        s.named,
        s.replaced,
        s.exported.replace('\n', " "),
        s.weather,
        s.wetness
    );
    let _ = std::fs::write(file("bullygfx_status.txt"), text);
}
