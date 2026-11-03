//! Bundles the 64-bit remaster into the menu when its builds are next to it
//! (../bullyre/build64/Release, ../bullygfx/target/x86_64-pc-windows-msvc/release):
//! bullyre_x64.exe, its .map (crash reports' names) and bullygfx64.dll, each deflated into
//! OUT_DIR. The menu unpacks them when it launches the 64-bit game (src/payload.rs), so the
//! menu is the one file there is. BULLYMENU_NO_PAYLOAD=1 builds a menu without them.

use std::path::PathBuf;

fn main() {
    println!("cargo:rustc-check-cfg=cfg(payload)");
    println!("cargo:rustc-check-cfg=cfg(payload_game_exe)");
    println!("cargo:rerun-if-env-changed=BULLYMENU_NO_PAYLOAD");
    let manifest = PathBuf::from(std::env::var("CARGO_MANIFEST_DIR").unwrap());
    let projects = manifest.parent().unwrap().to_path_buf();
    // Bully_unpacked.exe (the large-address-aware copy in the game folder, else bullydumper's
    // output): bundled on its own, so the menu can put it in the game folder when it's missing
    let game_exe = [
        PathBuf::from(r"C:\Program Files (x86)\Steam\steamapps\common\Bully Scholarship Edition\Bully_unpacked.exe"),
        projects.join(r"bullydumper\Bully.unpacked.exe"),
    ];
    for path in &game_exe {
        println!("cargo:rerun-if-changed={}", path.display());
    }
    if std::env::var_os("BULLYMENU_NO_PAYLOAD").is_none() {
        if let Some(path) = game_exe.iter().find(|p| p.exists()) {
            let data = std::fs::read(path).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
            let out = PathBuf::from(std::env::var("OUT_DIR").unwrap());
            std::fs::write(out.join("Bully_unpacked.exe.z"), miniz_oxide::deflate::compress_to_vec(&data, 6)).unwrap();
            println!("cargo:rustc-cfg=payload_game_exe");
        }
    }
    let files = [
        ("bullyre_x64.exe", projects.join(r"bullyre\build64\Release\bullyre_x64.exe")),
        ("bullyre_x64.map", projects.join(r"bullyre\build64\Release\bullyre_x64.map")),
        ("bullygfx64.dll", projects.join(r"bullygfx\target\x86_64-pc-windows-msvc\release\d3d9.dll")),
    ];
    for (_, path) in &files {
        println!("cargo:rerun-if-changed={}", path.display());
    }
    if std::env::var_os("BULLYMENU_NO_PAYLOAD").is_some() || !files.iter().all(|(_, p)| p.exists()) {
        return;
    }
    let out = PathBuf::from(std::env::var("OUT_DIR").unwrap());
    // (FNV-1a over what's bundled: the unpacked copy is made again when it changes)
    let mut stamp: u64 = 0xcbf2_9ce4_8422_2325;
    for (name, path) in &files {
        let data = std::fs::read(path).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
        let packed = miniz_oxide::deflate::compress_to_vec(&data, 6);
        for b in name.bytes().chain(packed.iter().copied()) {
            stamp = (stamp ^ u64::from(b)).wrapping_mul(0x0100_0000_01b3);
        }
        std::fs::write(out.join(format!("{name}.z")), &packed).unwrap();
    }
    println!("cargo:rustc-cfg=payload");
    println!("cargo:rustc-env=PAYLOAD_STAMP={stamp:016x}");
}
