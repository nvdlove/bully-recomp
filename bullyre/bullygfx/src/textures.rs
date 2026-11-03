//! Re-skinning textures: which game texture is which file (learnt from Gamebryo's
//! materials as meshes are drawn), replacement PNGs by that name, exporting what the
//! game has loaded as PNGs to start from, and a checkerboard.
//!
//! Names: a mesh's NiTexturingProperty (property state +0x28) lists its maps (+0x20,
//! count u16 at +0x24); each map +8 is an NiSourceTexture, whose file name is at +0x08
//! (else +0x30) and whose Direct3D texture is +0x24 (NiDX9TextureData) → +0x5C. A
//! texture no mesh names (HUD, fonts, effects) goes by `unnamed_<content hash>`.

use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};
use std::sync::Mutex;

use crate::win::{read_bytes, read_u32};

struct Registry {
    /// Direct3D texture -> (its NiSourceTexture, file name)
    names: HashMap<u32, (u32, String)>,
    /// NiTexturingProperty objects whose maps are all known.
    done: HashSet<u32>,
}

static REGISTRY: Mutex<Option<Registry>> = Mutex::new(None);

fn c_string(p: u32) -> Option<String> {
    if p < 0x10000 {
        return None;
    }
    let mut out = Vec::new();
    let mut chunk = [0u8; 32];
    while out.len() < 512 {
        if !read_bytes(p + out.len() as u32, &mut chunk) {
            return None;
        }
        match chunk.iter().position(|&b| b == 0) {
            Some(n) => {
                out.extend_from_slice(&chunk[..n]);
                break;
            }
            None => out.extend_from_slice(&chunk),
        }
    }
    let s = String::from_utf8_lossy(&out).into_owned();
    (s.len() >= 2 && s.chars().all(|c| (' '..='~').contains(&c))).then_some(s)
}

/// A file name as a key: no folders, no extension, lower case, only safe characters.
pub fn clean(name: &str) -> String {
    let file = name.rsplit(['\\', '/']).next().unwrap_or(name).to_ascii_lowercase();
    let stem = match file.rsplit_once('.') {
        Some((s, ext)) if matches!(ext, "tga" | "dds" | "bmp" | "png" | "nif" | "jpg") => s.to_string(),
        _ => file,
    };
    stem.chars().map(|c| if c.is_ascii_alphanumeric() || matches!(c, '_' | '-' | '.') { c } else { '_' }).collect()
}

fn source_texture(source: u32) -> Option<u32> {
    read_u32(read_u32(source + 0x24).filter(|&p| p != 0)? + 0x5C).filter(|&t| t != 0)
}

/// Testing: whether names are learnt.
pub static REGISTER: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(true);

/// A mesh is being drawn: learn its textures' names.
pub fn register(geometry: u32) {
    if !REGISTER.load(std::sync::atomic::Ordering::Relaxed) {
        return;
    }
    let Some(texturing) = read_u32(geometry + 0xC4).filter(|&p| p != 0).and_then(|s| read_u32(s + 0x28)).filter(|&p| p != 0) else {
        return;
    };
    let Ok(mut reg) = REGISTRY.lock() else { return };
    let reg = reg.get_or_insert_with(|| Registry { names: HashMap::new(), done: HashSet::new() });
    if reg.done.contains(&texturing) {
        return;
    }
    let count = read_u32(texturing + 0x24).unwrap_or(0) & 0xFFFF;
    let Some(maps) = read_u32(texturing + 0x20) else { return };
    let mut complete = true;
    for i in 0..count.min(16) {
        let Some(map) = read_u32(maps + 4 * i).filter(|&p| p != 0) else { continue };
        let Some(source) = read_u32(map + 8).filter(|&p| p != 0) else { continue };
        let Some(tex) = source_texture(source) else {
            complete = false; // not loaded yet
            continue;
        };
        if reg.names.get(&tex).is_some_and(|e| e.0 == source) {
            continue;
        }
        let name = [0x08, 0x30].iter().find_map(|&o| read_u32(source + o).and_then(c_string)).map(|n| clean(&n));
        match name {
            Some(n) if !n.is_empty() => {
                reg.names.insert(tex, (source, n));
            }
            _ => complete = false,
        }
    }
    if complete {
        reg.done.insert(texturing);
    }
}

/// The file name of a game texture, if a mesh has named it (and it's still that one).
pub fn name_of(tex: u32) -> Option<String> {
    let reg = REGISTRY.lock().ok()?;
    let (source, name) = reg.as_ref()?.names.get(&tex)?;
    (source_texture(*source) == Some(tex)).then(|| name.clone())
}

pub fn named_count() -> usize {
    REGISTRY.lock().ok().and_then(|r| r.as_ref().map(|r| r.names.len())).unwrap_or(0)
}

pub fn unnamed(sig: u64) -> String {
    format!("unnamed_{sig:016x}")
}

// ---- images --------------------------------------------------------------------

/// RGBA8, with its mip chain (each level half the last, box filtered).
pub struct Image {
    pub width: u32,
    pub height: u32,
    pub levels: Vec<Vec<u8>>,
}

fn mips(width: u32, height: u32, top: Vec<u8>) -> Image {
    let mut levels = vec![top];
    let (mut w, mut h) = (width, height);
    while w > 1 || h > 1 {
        let (nw, nh) = ((w / 2).max(1), (h / 2).max(1));
        let src = levels.last().unwrap();
        let mut dst = vec![0u8; (nw * nh * 4) as usize];
        for y in 0..nh {
            for x in 0..nw {
                for c in 0..4 {
                    let mut sum = 0u32;
                    for (dx, dy) in [(0, 0), (1, 0), (0, 1), (1, 1)] {
                        let sx = (x * 2 + dx).min(w - 1);
                        let sy = (y * 2 + dy).min(h - 1);
                        sum += src[((sy * w + sx) * 4 + c) as usize] as u32;
                    }
                    dst[((y * nw + x) * 4 + c) as usize] = ((sum + 2) / 4) as u8;
                }
            }
        }
        levels.push(dst);
        (w, h) = (nw, nh);
    }
    Image { width, height, levels }
}

pub fn load_png(path: &Path) -> Result<Image, String> {
    let file = std::fs::File::open(path).map_err(|e| e.to_string())?;
    let mut decoder = png::Decoder::new(std::io::BufReader::new(file));
    decoder.set_transformations(png::Transformations::EXPAND | png::Transformations::STRIP_16);
    let mut reader = decoder.read_info().map_err(|e| e.to_string())?;
    let mut buf = vec![0u8; reader.output_buffer_size()];
    let info = reader.next_frame(&mut buf).map_err(|e| e.to_string())?;
    let (w, h) = (info.width, info.height);
    if w == 0 || h == 0 || w > 8192 || h > 8192 {
        return Err(format!("{w}x{h} is too big"));
    }
    let px = &buf[..info.buffer_size()];
    let rgba: Vec<u8> = match info.color_type {
        png::ColorType::Rgba => px.to_vec(),
        png::ColorType::Rgb => px.chunks_exact(3).flat_map(|p| [p[0], p[1], p[2], 255]).collect(),
        png::ColorType::GrayscaleAlpha => px.chunks_exact(2).flat_map(|p| [p[0], p[0], p[0], p[1]]).collect(),
        png::ColorType::Grayscale => px.iter().flat_map(|&g| [g, g, g, 255]).collect(),
        png::ColorType::Indexed => return Err("indexed colour after expanding".into()),
    };
    Ok(mips(w, h, rgba))
}

pub fn save_png(path: &Path, width: u32, height: u32, rgba: &[u8]) -> Result<(), String> {
    let file = std::fs::File::create(path).map_err(|e| e.to_string())?;
    let mut enc = png::Encoder::new(std::io::BufWriter::new(file), width, height);
    enc.set_color(png::ColorType::Rgba);
    enc.set_depth(png::BitDepth::Eight);
    enc.write_header().and_then(|mut w| w.write_image_data(rgba)).map_err(|e| e.to_string())
}

/// A magenta/grey checkerboard, 256 px, 32 px squares.
pub fn checker() -> Image {
    let n = 256u32;
    let mut px = Vec::with_capacity((n * n * 4) as usize);
    for y in 0..n {
        for x in 0..n {
            let on = ((x / 32) + (y / 32)) % 2 == 0;
            px.extend(if on { [230, 40, 200, 255] } else { [40, 40, 44, 255] });
        }
    }
    mips(n, n, px)
}

// ---- DXT (BC1/2/3) to RGBA, for export --------------------------------------------

fn rgb565(c: u16) -> [i32; 3] {
    let e = |v: u16, bits: u32| ((v as u32 * 255 + ((1 << bits) - 1) / 2) / ((1 << bits) - 1)) as i32;
    [e(c >> 11, 5), e((c >> 5) & 63, 6), e(c & 31, 5)]
}

fn color_block(b: &[u8], four_colour: bool, out: &mut [[u8; 4]; 16]) {
    let c0 = u16::from_le_bytes([b[0], b[1]]);
    let c1 = u16::from_le_bytes([b[2], b[3]]);
    let (a, bb) = (rgb565(c0), rgb565(c1));
    let mix = |p: i32, q: i32, pw: i32, qw: i32| ((p * pw + q * qw) / (pw + qw)) as u8;
    let mut pal = [[0u8; 4]; 4];
    pal[0] = [a[0] as u8, a[1] as u8, a[2] as u8, 255];
    pal[1] = [bb[0] as u8, bb[1] as u8, bb[2] as u8, 255];
    if four_colour || c0 > c1 {
        pal[2] = [mix(a[0], bb[0], 2, 1), mix(a[1], bb[1], 2, 1), mix(a[2], bb[2], 2, 1), 255];
        pal[3] = [mix(a[0], bb[0], 1, 2), mix(a[1], bb[1], 1, 2), mix(a[2], bb[2], 1, 2), 255];
    } else {
        pal[2] = [mix(a[0], bb[0], 1, 1), mix(a[1], bb[1], 1, 1), mix(a[2], bb[2], 1, 1), 255];
        pal[3] = [0, 0, 0, 0];
    }
    let idx = u32::from_le_bytes([b[4], b[5], b[6], b[7]]);
    for (i, px) in out.iter_mut().enumerate() {
        *px = pal[((idx >> (2 * i)) & 3) as usize];
    }
}

fn decode_dxt(format: u32, width: u32, height: u32, data: &[u8]) -> Vec<u8> {
    let block = if format == crate::d3d::FMT_DXT1 { 8 } else { 16 };
    let (bw, bh) = (width.div_ceil(4), height.div_ceil(4));
    let mut out = vec![0u8; (width * height * 4) as usize];
    for by in 0..bh {
        for bx in 0..bw {
            let at = ((by * bw + bx) * block) as usize;
            let Some(b) = data.get(at..at + block as usize) else { continue };
            let mut px = [[0u8; 4]; 16];
            match format {
                crate::d3d::FMT_DXT1 => color_block(b, false, &mut px),
                crate::d3d::FMT_DXT3 => {
                    color_block(&b[8..], true, &mut px);
                    for (i, p) in px.iter_mut().enumerate() {
                        let nib = (b[i / 2] >> (4 * (i % 2))) & 15;
                        p[3] = nib * 17;
                    }
                }
                _ => {
                    color_block(&b[8..], true, &mut px);
                    let (a0, a1) = (b[0] as u32, b[1] as u32);
                    let mut alpha = [0u32; 8];
                    alpha[0] = a0;
                    alpha[1] = a1;
                    if a0 > a1 {
                        for k in 1..7 {
                            alpha[k + 1] = ((7 - k as u32) * a0 + k as u32 * a1) / 7;
                        }
                    } else {
                        for k in 1..5 {
                            alpha[k + 1] = ((5 - k as u32) * a0 + k as u32 * a1) / 5;
                        }
                        alpha[6] = 0;
                        alpha[7] = 255;
                    }
                    let bits = b[2..8].iter().rev().fold(0u64, |acc, &x| (acc << 8) | x as u64);
                    for (i, p) in px.iter_mut().enumerate() {
                        p[3] = alpha[((bits >> (3 * i)) & 7) as usize] as u8;
                    }
                }
            }
            for (i, p) in px.iter().enumerate() {
                let (x, y) = (bx * 4 + (i as u32 % 4), by * 4 + (i as u32 / 4));
                if x < width && y < height {
                    let o = ((y * width + x) * 4) as usize;
                    out[o..o + 4].copy_from_slice(p);
                }
            }
        }
    }
    out
}

// ---- export ----------------------------------------------------------------------

/// What the last export did, for the menu.
pub static EXPORT_STATUS: Mutex<String> = Mutex::new(String::new());

/// Writes the textures (name, the game's data) as PNGs into `dir`, on a thread of its
/// own. Existing files are left alone (they may be edited copies).
pub fn export(list: Vec<(String, crate::d3d::Texture)>, dir: PathBuf) {
    if let Ok(mut s) = EXPORT_STATUS.lock() {
        *s = format!("exporting {} textures…", list.len());
    }
    std::thread::spawn(move || {
        let _ = std::fs::create_dir_all(&dir);
        let (mut written, mut kept, mut failed) = (0, 0, 0);
        for (name, t) in list {
            let path = dir.join(format!("{name}.png"));
            if path.exists() {
                kept += 1;
                continue;
            }
            let Some(top) = t.levels.first() else { continue };
            let rgba = if t.format == 0 { top.clone() } else { decode_dxt(t.format, t.width, t.height, top) };
            match save_png(&path, t.width, t.height, &rgba) {
                Ok(()) => written += 1,
                Err(_) => failed += 1,
            }
        }
        if let Ok(mut s) = EXPORT_STATUS.lock() {
            *s = format!("{written} new PNGs in {} ({kept} already there{})", dir.display(), if failed > 0 { format!(", {failed} failed") } else { String::new() });
        }
    });
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn names_are_cleaned() {
        assert_eq!(clean(r"Z:\Bully\Temp\Export\Textures\Objects\Classic_wheel\wheel_dozer_d.tga"), "wheel_dozer_d");
        assert_eq!(clean("wloads02.tga"), "wloads02");
        assert_eq!(clean("Some Name.DDS"), "some_name");
    }

    #[test]
    fn dxt1_block_decodes() {
        // c0 = white, c1 = black, all pixels index 0 then index 1
        let mut b = [0xFF, 0xFF, 0x00, 0x00, 0, 0, 0, 0];
        let out = decode_dxt(crate::d3d::FMT_DXT1, 4, 4, &b);
        assert_eq!(&out[..4], &[255, 255, 255, 255]);
        b[4..8].copy_from_slice(&[0x55; 4]);
        let out = decode_dxt(crate::d3d::FMT_DXT1, 4, 4, &b);
        assert_eq!(&out[..4], &[0, 0, 0, 255]);
    }

    #[test]
    fn mips_go_down_to_one() {
        let img = mips(8, 2, vec![128; 8 * 2 * 4]);
        assert_eq!(img.levels.len(), 4);
        assert_eq!(img.levels.last().unwrap(), &vec![128; 4]);
    }
}
