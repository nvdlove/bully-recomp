//! The Mods page: the mod folders the renderer's dll loads over the game (nothing is
//! copied into the game), in bullygfx_mods\mods.txt: "+ folder" on, "- folder" off,
//! later ones win. The dll watches the file, so switching applies while the game runs;
//! it reports back in bullygfx_mods\status.txt.

use std::path::{Path, PathBuf};

use crate::win::HWND;

const HEADER: &str = "# Mod folders, one per line: \"+ folder\" is on, \"- folder\" is off. Later ones win. Changes apply while the game runs.\n";

fn dir() -> PathBuf {
    crate::gfx::game_file("bullygfx_mods")
}

pub struct Mod {
    pub path: String,
    pub on: bool,
}

pub fn load() -> Vec<Mod> {
    let text = std::fs::read_to_string(dir().join("mods.txt")).unwrap_or_default();
    text.lines()
        .map(str::trim)
        .filter(|l| !l.is_empty() && !l.starts_with('#') && !l.contains('='))
        .map(|l| match l.split_at(1) {
            ("+", rest) => Mod { path: rest.trim().into(), on: true },
            ("-", rest) => Mod { path: rest.trim().into(), on: false },
            _ => Mod { path: l.into(), on: true },
        })
        .collect()
}

pub fn save(mods: &[Mod]) {
    let _ = std::fs::create_dir_all(dir());
    // settings lines (max_texture=...) stay
    let old = std::fs::read_to_string(dir().join("mods.txt")).unwrap_or_default();
    let mut text = HEADER.to_string();
    for l in old.lines().filter(|l| l.contains('=') && !l.starts_with('#')) {
        text += l;
        text += "\n";
    }
    for m in mods {
        text += &format!("{} {}\n", if m.on { "+" } else { "-" }, m.path);
    }
    let _ = std::fs::write(dir().join("mods.txt"), text);
}

/// Whether a folder looks like a mod: an archive (World.img + World.dir, there or in
/// its Stream folder) or textures somewhere in it.
pub fn looks_like_mod(folder: &Path) -> bool {
    let archive = [folder.to_path_buf(), folder.join("Stream")].iter().any(|d| d.join("World.img").is_file() && d.join("World.dir").is_file());
    archive || has_textures(folder, 4)
}

fn has_textures(d: &Path, depth: u32) -> bool {
    let Ok(rd) = std::fs::read_dir(d) else { return false };
    rd.flatten().any(|e| {
        let p = e.path();
        if p.is_dir() {
            depth > 0 && has_textures(&p, depth - 1)
        } else {
            p.extension().and_then(|x| x.to_str()).is_some_and(|x| matches!(x.to_ascii_lowercase().as_str(), "png" | "dds" | "tga" | "bmp"))
        }
    })
}

/// The dll's report: per mod folder "changed|deleted|textures|error", and what the
/// last live change did.
pub fn status() -> (Vec<(String, String)>, String) {
    let text = std::fs::read_to_string(dir().join("status.txt")).unwrap_or_default();
    let mut per_mod = Vec::new();
    let mut applied = String::new();
    for line in text.lines() {
        if let Some(rest) = line.strip_prefix("mod|") {
            if let Some((path, info)) = rest.split_once('|') {
                per_mod.push((path.to_string(), info.to_string()));
            }
        } else if let Some(rest) = line.strip_prefix("applied|") {
            applied = rest.to_string();
        }
    }
    (per_mod, applied)
}

/// Windows' folder picker.
pub fn pick_folder(owner: HWND) -> Option<PathBuf> {
    crate::win::pick_folder(owner, "Pick the mod's folder (the one with its World.img, or its textures)", "").map(PathBuf::from)
}
