//! The 64-bit remaster bundled into the menu (build.rs): unpacked into
//! %LOCALAPPDATA%\BullyRemaster\bin the first time it's launched (and again when the menu
//! carries a different build), and run from there. The menu's own things (settings, the
//! WebView's data) live in %LOCALAPPDATA%\BullyRemaster too, so the menu is the one file.

use std::path::PathBuf;

#[cfg(payload)]
const FILES: &[(&str, &[u8])] = &[
    ("bullyre_x64.exe", include_bytes!(concat!(env!("OUT_DIR"), "/bullyre_x64.exe.z"))),
    ("bullyre_x64.map", include_bytes!(concat!(env!("OUT_DIR"), "/bullyre_x64.map.z"))),
    ("bullygfx64.dll", include_bytes!(concat!(env!("OUT_DIR"), "/bullygfx64.dll.z"))),
];
#[cfg(payload)]
const STAMP: &str = env!("PAYLOAD_STAMP");

#[cfg(not(payload))]
const FILES: &[(&str, &[u8])] = &[];
#[cfg(not(payload))]
const STAMP: &str = "";

#[cfg(payload_game_exe)]
const GAME_EXE: &[u8] = include_bytes!(concat!(env!("OUT_DIR"), "/Bully_unpacked.exe.z"));
#[cfg(not(payload_game_exe))]
const GAME_EXE: &[u8] = &[];

/// The bundled Bully_unpacked.exe, inflated (None: the menu was built without it).
pub fn game_exe() -> Option<Vec<u8>> {
    if GAME_EXE.is_empty() {
        return None;
    }
    miniz_oxide::inflate::decompress_to_vec(GAME_EXE).ok()
}

/// The menu carries the 64-bit game.
pub fn available() -> bool {
    !FILES.is_empty()
}

/// %LOCALAPPDATA%\BullyRemaster: the menu's settings, its WebView's data, the unpacked game.
pub fn home() -> PathBuf {
    std::env::var_os("LOCALAPPDATA").map(PathBuf::from).unwrap_or_else(std::env::temp_dir).join("BullyRemaster")
}

/// The folder the bundled game runs from: unpacked now if it isn't, or is another build's.
pub fn unpack() -> Result<PathBuf, String> {
    let dir = home().join("bin");
    let stamp = dir.join("bundle.txt");
    if std::fs::read_to_string(&stamp).ok().as_deref() == Some(STAMP) && FILES.iter().all(|(name, _)| dir.join(name).exists()) {
        return Ok(dir);
    }
    std::fs::create_dir_all(&dir).map_err(|e| format!("couldn't make {}: {e}", dir.display()))?;
    for (name, packed) in FILES {
        let data = miniz_oxide::inflate::decompress_to_vec(packed).map_err(|e| format!("the bundled {name} is damaged: {e:?}"))?;
        let path = dir.join(name);
        let fresh = dir.join(format!("{name}.new"));
        std::fs::write(&fresh, &data).map_err(|e| format!("couldn't write {}: {e}", fresh.display()))?;
        if let Err(e) = std::fs::rename(&fresh, &path) {
            let _ = std::fs::remove_file(&fresh);
            return Err(format!("couldn't put the new {name} in place (is the 64-bit game still running?): {e}"));
        }
    }
    std::fs::write(&stamp, STAMP).map_err(|e| format!("couldn't write {}: {e}", stamp.display()))?;
    Ok(dir)
}
