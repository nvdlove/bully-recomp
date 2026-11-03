//! Mods without installing them, switched on and off while the game runs.
//!
//! bullygfx_mods\mods.txt lists mod folders, one per line: "+ folder" (on), "- folder"
//! (off); later ones win. It's watched: changes apply live.
//!
//! - A mod's World.img/World.dir (in the folder or its Stream folder): the entries that
//!   differ from the game's are found once (cached in bullygfx_mods) and its archive
//!   is placed past the end of the game's World.img, in a region only we know about;
//!   reads there come from the mod's archive. At start-up the game gets a World.dir
//!   with the enabled mods' entries in it (new names too). Later changes go straight
//!   into the game's streaming table (CStreamingInfo, 0xCA39B8): an entry's archive
//!   position and size are rewritten, and a loaded model is dropped (its buildings'
//!   and objects' geometry deleted, then CStreaming::RemoveModel) so the game streams
//!   the new one back in. Peds and vehicles keep what they have until they respawn;
//!   names the game didn't know at start-up (or bigger than its streaming buffers)
//!   need a restart.
//!   The game reads up to four models in one go when they follow each other in the
//!   directory (nextOnCd, +6), assuming they sit end to end in the archive: every
//!   entry that points into our regions is cut out of those chains.
//!   Only the changed entries go into a region, packed: the game keeps archive
//!   positions as 32-bit byte offsets.
//! - A mod whose archive is a whole copy of the game's (most of its names) can also
//!   delete entries: the game's models it left out (old buildings its new ones
//!   replace). Installed, those have no entry at all, and the game still asks for
//!   them (their buildings are still placed): with no size, RequestModelStream reads
//!   with a stale size and corrupts memory. Here they point at one of the game's own
//!   empty models (decals01.nif: a bare scene root) instead.
//! - Textures a model asks for by file name (mods' models often point at a folder
//!   like C:\[author]\textures\x.dds) and the game can't find are looked up by name in
//!   the enabled mods. PNGs win and are compressed to DXT once (cached), so a mod's
//!   textures can be edited as plain PNGs. Textures bigger than max_texture (a line
//!   in mods.txt, default 1024, 0 for no cap) lose their top mip levels: the game is a
//!   32-bit process and keeps a copy of every texture, and big mods fill its 4 GB. The
//!   64-bit game (bullyre_x64) reads max_texture_64 instead, no cap by default; there a
//!   PNG over 1024 that has a DDS of its own is served as the DDS (no conversion).
//!
//! The game's imports of CreateFileA / ReadFile / SetFilePointer / GetFileSize /
//! CloseHandle are patched to go through here.

use std::collections::{HashMap, HashSet};
use std::ffi::{CStr, c_void};
use std::os::windows::fs::FileExt;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, AtomicU32, AtomicU64, AtomicUsize, Ordering};
use std::sync::{Arc, Condvar, Mutex, RwLock};
use std::time::Duration;

use crate::win::*;

// the exe's import address table entries (kernel32)
const IAT_CLOSE_HANDLE: usize = 0x008F_C050;
const IAT_CREATE_FILE_A: usize = 0x008F_C064;
const IAT_SET_FILE_POINTER: usize = 0x008F_C068;
const IAT_READ_FILE: usize = 0x008F_C06C;
const IAT_GET_FILE_SIZE: usize = 0x008F_C098;

// the game's streaming
const STREAMING_INFO: usize = 0x00CA_39B8; // 16 bytes each: +4 load state, +5 flags, +8 sector, +0xC sectors
const SLOT_OF: usize = 0x0052_DC90; // cdecl (id) -> index into STREAMING_INFO, or -1
const REMOVE_MODEL: usize = 0x0052_F640; // cdecl (id)
const DELETE_RW_OBJECT: usize = 0x0046_67A0; // thiscall (entity)
const TXD_REFS: usize = 0x005F_11E0; // cdecl (slot) -> refs
const MODEL_INFOS: usize = 0x00C6_7738; // CBaseModelInfo*[]: +0x14 refs, +0x16 txd slot
const ENTITY_POOLS: [usize; 3] = [0x00C0_F5F8, 0x00C0_F5FC, 0x00C0_F600]; // buildings, objects, dummies
const LOAD_CD_DIRECTORY: usize = 0x0052_E1B0; // cdecl (image): reads its directory into STREAMING_INFO
const LOAD_CD_DIRECTORY_CALL: usize = 0x0052_ED10; // the one call of it
const MAX_ENTRY: usize = 0x00CA_3978; // sectors of the largest entry (the streaming buffers' size)
const MODEL_BY_NAME: usize = 0x0051_C190; // cdecl (name, &id)
const MODEL_BY_NAME_HASHED: usize = 0x0051_C390; // cdecl (name) -> id, when HASHED_NAMES
const HASHED_NAMES: usize = 0x00BF_380C;
const TXD_BASE: u32 = 0x2FA8; // ids: models, then texture dictionaries, then collision ...
const COL_BASE: u32 = 0x56B8;
const LAST_ID: u32 = 0x8000;

const SECTOR: u64 = 2048;
const FILE_FLAG_OVERLAPPED: u32 = 0x4000_0000;
/// Log every file the game opens (bullygfx_log.txt).
const LOG_OPENS: bool = false;

#[link(name = "kernel32")]
unsafe extern "system" {
    fn CreateFileW(name: *const u16, access: u32, share: u32, sa: *mut c_void, disposition: u32, flags: u32, template: *mut c_void) -> *mut c_void;
    fn SetEvent(h: *mut c_void) -> i32;
}

#[repr(C)]
struct Overlapped {
    internal: usize,
    internal_high: usize,
    offset: u32,
    offset_high: u32,
    event: *mut c_void,
}

type CreateFileA = unsafe extern "system" fn(*const i8, u32, u32, *mut c_void, u32, u32, *mut c_void) -> *mut c_void;
type ReadFile = unsafe extern "system" fn(*mut c_void, *mut u8, u32, *mut u32, *mut Overlapped) -> i32;
type SetFilePointer = unsafe extern "system" fn(*mut c_void, i32, *mut i32, u32) -> u32;
type GetFileSize = unsafe extern "system" fn(*mut c_void, *mut u32) -> u32;
type CloseHandle = unsafe extern "system" fn(*mut c_void) -> i32;

static REAL_CREATE_FILE_A: AtomicUsize = AtomicUsize::new(0);
static REAL_READ_FILE: AtomicUsize = AtomicUsize::new(0);
static REAL_SET_FILE_POINTER: AtomicUsize = AtomicUsize::new(0);
static REAL_GET_FILE_SIZE: AtomicUsize = AtomicUsize::new(0);
static REAL_CLOSE_HANDLE: AtomicUsize = AtomicUsize::new(0);

fn real<T: Copy>(slot: &AtomicUsize) -> T {
    let p = slot.load(Ordering::SeqCst);
    unsafe { std::mem::transmute_copy(&p) }
}

// ---- the archive past the game's --------------------------------------------------

/// A mod's changed entries, packed one after another past the game's World.img (and
/// earlier mods').
struct Region {
    start: u64,
    len: u64,
    file: std::fs::File,
    /// (offset in the region, offset in the mod's World.img, bytes), in order
    pieces: Vec<(u64, u64, u64)>,
}

/// The biggest texture side a mod texture keeps (0: no cap): max_texture= in mods.txt
/// (1024 when it's not there) for the 32-bit game, which keeps a copy of every texture in
/// its 4 GB; the 64-bit game has the memory: max_texture_64= (no cap when it's not there).
static TEXTURE_CAP: AtomicU32 = AtomicU32::new(if cfg!(target_pointer_width = "64") { 0 } else { 1024 });

fn read_texture_cap() -> u32 {
    let (key, default) = if cfg!(target_pointer_width = "64") { ("max_texture_64=", 0) } else { ("max_texture=", 1024) };
    std::fs::read_to_string(list_path())
        .ok()
        .and_then(|t| t.lines().find_map(|l| l.trim().strip_prefix(key).and_then(|v| v.trim().parse().ok())))
        .unwrap_or(default)
}

/// A PNG's longer side (from its header; 0 when it can't be read).
fn png_side(p: &Path) -> u32 {
    let mut h = [0u8; 24];
    match std::fs::File::open(p) {
        Ok(f) if read_exact(&f, &mut h, 0) && &h[1..4] == b"PNG" => {
            u32::from_be_bytes(h[16..20].try_into().unwrap()).max(u32::from_be_bytes(h[20..24].try_into().unwrap()))
        }
        _ => 0,
    }
}

/// Where deleted models point: an empty model of the game's (sector << 32 | sectors;
/// 0 when there's none).
static EMPTY_MODEL: AtomicU64 = AtomicU64::new(0);
const EMPTY_MODEL_NAMES: [&str; 3] = ["decals01.nif", "dcl_bbathsd05.nif", "dl_rc_yumopen.nif"];

fn empty_model() -> Option<(u32, u32)> {
    let v = EMPTY_MODEL.load(Ordering::SeqCst);
    (v != 0).then_some(((v >> 32) as u32, v as u32))
}

/// Size of the game's World.img, whole sectors (0 until known).
static BASE: AtomicU64 = AtomicU64::new(0);
static REGIONS: RwLock<Vec<Arc<Region>>> = RwLock::new(Vec::new());

fn total() -> u64 {
    REGIONS.read().unwrap().last().map_or(BASE.load(Ordering::SeqCst), |r| r.start + r.len)
}

/// Reads bytes of our regions.
fn read_regions(at: u64, buf: &mut [u8]) -> usize {
    let regions = REGIONS.read().unwrap();
    let mut got = 0;
    while got < buf.len() {
        let pos = at + got as u64;
        let Some(r) = regions.iter().find(|r| pos >= r.start && pos < r.start + r.len) else { break };
        let rel = pos - r.start;
        let (from, file_at, n) = r.pieces[r.pieces.partition_point(|p| p.0 <= rel) - 1];
        let k = ((from + n - rel) as usize).min(buf.len() - got);
        if !read_exact(&r.file, &mut buf[got..got + k], file_at + rel - from) {
            break;
        }
        got += k;
    }
    got
}

// ---- mods ------------------------------------------------------------------------

/// Where mods are configured and their cache lives.
pub fn mods_dir() -> PathBuf {
    crate::win::game_file("bullygfx_mods")
}

fn list_path() -> PathBuf {
    mods_dir().join("mods.txt")
}

const LIST_HEADER: &str = "# Mod folders, one per line: \"+ folder\" is on, \"- folder\" is off. Later ones win. Changes apply while the game runs.\n";

/// The mod list: (folder, on).
fn configured() -> Vec<(PathBuf, bool)> {
    let Ok(text) = std::fs::read_to_string(list_path()) else {
        let _ = std::fs::create_dir_all(mods_dir());
        let _ = std::fs::write(list_path(), LIST_HEADER);
        return Vec::new();
    };
    text.lines()
        .map(str::trim)
        .filter(|l| !l.is_empty() && !l.starts_with('#') && !l.contains('='))
        .map(|l| match l.split_at(1) {
            ("+", rest) => (PathBuf::from(rest.trim()), true),
            ("-", rest) => (PathBuf::from(rest.trim()), false),
            _ => (PathBuf::from(l), true),
        })
        .collect()
}

#[derive(Clone)]
struct Entry {
    offset: u32,
    size: u32,
    name: [u8; 24],
}

fn parse_dir(b: &[u8]) -> Vec<Entry> {
    b.chunks_exact(32)
        .map(|e| Entry {
            offset: u32::from_le_bytes(e[0..4].try_into().unwrap()),
            size: u32::from_le_bytes(e[4..8].try_into().unwrap()),
            name: e[8..32].try_into().unwrap(),
        })
        .collect()
}

fn entry_name(name: &[u8; 24]) -> String {
    let end = name.iter().position(|&c| c == 0).unwrap_or(24);
    String::from_utf8_lossy(&name[..end]).to_ascii_lowercase()
}

fn stamp(p: &Path, h: &mut impl std::hash::Hasher) {
    use std::hash::Hash;
    p.hash(h);
    if let Ok(m) = std::fs::metadata(p) {
        m.len().hash(h);
        m.modified().ok().hash(h);
    }
}

/// A mod's archive: World.dir + World.img in the folder or its Stream folder.
fn archive_of(root: &Path) -> Option<(PathBuf, PathBuf)> {
    [root.to_path_buf(), root.join("Stream")].into_iter().find_map(|d| {
        let (dir, img) = (d.join("World.dir"), d.join("World.img"));
        (dir.is_file() && img.is_file()).then_some((dir, img))
    })
}

fn index_textures(root: &Path) -> HashMap<String, Vec<PathBuf>> {
    let mut out: HashMap<String, Vec<PathBuf>> = HashMap::new();
    let mut stack = vec![root.to_path_buf()];
    while let Some(d) = stack.pop() {
        let Ok(rd) = std::fs::read_dir(&d) else { continue };
        for e in rd.flatten() {
            let p = e.path();
            if e.file_type().is_ok_and(|t| t.is_dir()) {
                stack.push(p);
            } else if let Some(ext) = p.extension().and_then(|x| x.to_str()) {
                if matches!(ext.to_ascii_lowercase().as_str(), "png" | "dds" | "tga" | "bmp") {
                    let stem = p.file_stem().unwrap().to_string_lossy().to_ascii_lowercase();
                    out.entry(stem).or_default().push(p);
                }
            }
        }
    }
    out
}

/// The game's own directory.
struct Game {
    entries: Vec<Entry>,
    by_name: HashMap<String, (u32, u32, [u8; 24])>,
}

/// A mod, ready to switch on.
struct Mod {
    _region: Option<Arc<Region>>,
    /// Entries that differ from the game's (or are new): name -> (sector in the
    /// merged archive, sectors, the entry's name bytes).
    changes: Vec<(String, u32, u32, [u8; 24])>,
    /// The game's models it deletes (a whole-archive mod leaving them out).
    removed: Vec<String>,
    textures: HashMap<String, Vec<PathBuf>>,
}

fn load_game() -> Result<Game, String> {
    let stream = crate::win::game_file("Stream");
    let img_len = std::fs::metadata(stream.join("World.img")).map_err(|e| format!("World.img: {e}"))?.len();
    BASE.store(img_len.div_ceil(SECTOR) * SECTOR, Ordering::SeqCst);
    let entries = parse_dir(&std::fs::read(stream.join("World.dir")).map_err(|e| format!("World.dir: {e}"))?);
    let by_name: HashMap<String, (u32, u32, [u8; 24])> = entries.iter().map(|e| (entry_name(&e.name), (e.offset, e.size, e.name))).collect();
    if let Some(&(off, size, _)) = EMPTY_MODEL_NAMES.iter().find_map(|n| by_name.get(*n)) {
        EMPTY_MODEL.store((off as u64) << 32 | size as u64, Ordering::SeqCst);
    }
    Ok(Game { entries, by_name })
}

/// Opens a mod: its archive gets the next region, its changes are found (or read
/// from the cache).
fn prepare(root: &Path, game: &Game) -> Result<Mod, String> {
    if !root.is_dir() {
        return Err("not a folder".into());
    }
    let textures = index_textures(root);
    let Some((dir, img)) = archive_of(root) else {
        crate::control::log(&format!("mods: {}: {} textures, no archive", root.display(), textures.len()));
        return Ok(Mod { _region: None, changes: Vec::new(), removed: Vec::new(), textures });
    };
    let file = std::fs::File::open(&img).map_err(|e| format!("{}: {e}", img.display()))?;
    let img_len = file.metadata().map_err(|e| e.to_string())?.len();

    // which entries differ from the game's: cached
    let stream = crate::win::game_file("Stream");
    let mut h = std::hash::DefaultHasher::new();
    for p in [&stream.join("World.dir"), &stream.join("World.img"), &dir, &img] {
        stamp(p, &mut h);
    }
    let cache = mods_dir().join(format!("changes-{:016x}.txt", std::hash::Hasher::finish(&h)));
    let mod_entries = parse_dir(&std::fs::read(&dir).map_err(|e| e.to_string())?);
    let changed: HashSet<String> = match std::fs::read_to_string(&cache) {
        Ok(text) => text.lines().map(str::to_owned).collect(),
        Err(_) => {
            let t = std::time::Instant::now();
            let game_img = std::fs::File::open(stream.join("World.img")).map_err(|e| e.to_string())?;
            let set = diff(&mod_entries, game, &game_img, &file, img_len);
            let mut text: Vec<&str> = set.iter().map(String::as_str).collect();
            text.sort();
            let _ = std::fs::write(&cache, text.join("\n"));
            crate::control::log(&format!("mods: compared {} with the game in {:.1} s", root.display(), t.elapsed().as_secs_f32()));
            set
        }
    };
    // the changed entries, packed into the next region
    let mut regions = REGIONS.write().unwrap();
    let start = regions.last().map_or(BASE.load(Ordering::SeqCst), |r| r.start + r.len);
    let (mut pieces, mut changes, mut len, mut seen) = (Vec::new(), Vec::new(), 0u64, HashSet::new());
    for e in &mod_entries {
        let name = entry_name(&e.name);
        if !changed.contains(&name) || !seen.insert(name.clone()) {
            continue;
        }
        pieces.push((len, e.offset as u64 * SECTOR, e.size as u64 * SECTOR));
        changes.push((name, ((start + len) / SECTOR) as u32, e.size, e.name));
        len += e.size as u64 * SECTOR;
    }
    if start + len > u32::MAX as u64 {
        return Err("the mods' changes don't fit in the game's 4 GB archive space".into());
    }
    let region = Arc::new(Region { start, len, file, pieces });
    regions.push(region.clone());
    drop(regions);
    // a whole copy of the game's archive: the models it leaves out are deleted
    let names: HashSet<String> = mod_entries.iter().map(|e| entry_name(&e.name)).collect();
    let kept = game.by_name.keys().filter(|n| names.contains(*n)).count();
    let removed: Vec<String> = if kept * 10 >= game.by_name.len() * 9 {
        game.by_name.keys().filter(|n| !names.contains(*n) && n.ends_with(".nif")).cloned().collect()
    } else {
        Vec::new()
    };
    crate::control::log(&format!("mods: {}: {} changed entries, {} deleted models, {} textures", root.display(), changes.len(), removed.len(), textures.len()));
    Ok(Mod { _region: Some(region), changes, removed, textures })
}

/// The names of a mod's entries that differ from the game's or are new (sane ones).
fn diff(mod_entries: &[Entry], game: &Game, game_img: &std::fs::File, mod_img: &std::fs::File, mod_len: u64) -> HashSet<String> {
    let mut out = HashSet::new();
    let (mut a, mut b) = (Vec::new(), Vec::new());
    for e in mod_entries {
        if e.size == 0 || (e.offset as u64 + e.size as u64) * SECTOR > mod_len.div_ceil(SECTOR) * SECTOR {
            continue;
        }
        let name = entry_name(&e.name);
        let same = game.by_name.get(&name).is_some_and(|&(off, size, _)| {
            let n = (size as u64 * SECTOR) as usize;
            size == e.size && {
                a.resize(n, 0);
                b.resize(n, 0);
                read_exact(game_img, &mut a, off as u64 * SECTOR) && read_exact(mod_img, &mut b, e.offset as u64 * SECTOR) && a == b
            }
        });
        if !same {
            out.insert(name);
        }
    }
    out
}

fn read_exact(f: &std::fs::File, buf: &mut [u8], at: u64) -> bool {
    let mut got = 0;
    while got < buf.len() {
        match f.seek_read(&mut buf[got..], at + got as u64) {
            Ok(0) | Err(_) => return false,
            Ok(k) => got += k,
        }
    }
    true
}

/// Where every entry any known mod touches should point, for the mods that are on:
/// name -> (sector, sectors, name bytes).
type Wanted = HashMap<String, (u32, u32, [u8; 24])>;

/// (A deleted model: the empty model's position.)
fn wanted(game: &Game, all: &[&Mod], on: &[&Mod]) -> Wanted {
    let mut w = Wanted::new();
    for m in all {
        for name in m.changes.iter().map(|c| &c.0).chain(&m.removed) {
            if let Some(&(off, size, bytes)) = game.by_name.get(name) {
                w.insert(name.clone(), (off, size, bytes));
            }
        }
    }
    for m in on {
        for name in &m.removed {
            if let Some(&(_, _, bytes)) = game.by_name.get(name) {
                let (off, size) = empty_model().unwrap_or((0, 0));
                w.insert(name.clone(), (off, size, bytes));
            }
        }
        for (name, off, size, bytes) in &m.changes {
            w.insert(name.clone(), (*off, *size, *bytes));
        }
    }
    w
}

// ---- state shared with the hooks and the game thread ------------------------------

/// The start-up World.dir (the game's with the enabled mods in it), once ready.
enum Startup {
    Building,
    Ready(Option<Vec<u8>>),
}

static STARTUP: Mutex<Startup> = Mutex::new(Startup::Ready(None));
static STARTUP_READY: Condvar = Condvar::new();

/// The World.dir the game gets, waiting for it if it's still being built; None:
/// the game's own.
fn startup_dir() -> Option<Vec<u8>> {
    let mut s = STARTUP.lock().unwrap();
    loop {
        match &*s {
            Startup::Ready(d) => return d.clone(),
            Startup::Building => s = STARTUP_READY.wait(s).unwrap(),
        }
    }
}

/// For the menu (bullygfx_mods\status.txt): what each mod changes, what the last
/// live change did.
static MOD_REPORT: Mutex<Vec<String>> = Mutex::new(Vec::new());
static LAST_APPLY: Mutex<String> = Mutex::new(String::new());

fn write_status() {
    let mut text = MOD_REPORT.lock().unwrap().join("\n");
    let applied = LAST_APPLY.lock().unwrap().clone();
    if !applied.is_empty() {
        text += &format!("\napplied|{applied}");
    }
    let _ = std::fs::write(mods_dir().join("status.txt"), text + "\n");
}

/// The textures of the mods that are on (later mods first).
static TEXTURES: RwLock<Vec<Arc<HashMap<String, Vec<PathBuf>>>>> = RwLock::new(Vec::new());
/// What the streaming table should say, for the game thread to apply.
static PENDING: Mutex<Option<Wanted>> = Mutex::new(None);
/// The directory the game loaded (to find each name's id), and whether it's loaded.
static LOADED_DIR: Mutex<Option<Vec<u8>>> = Mutex::new(None);
static DIR_LOADED: AtomicBool = AtomicBool::new(false);

// ---- textures --------------------------------------------------------------------

/// The best file for a texture the game couldn't open: same name, most matching
/// folders, PNG first (DDS only when there is no PNG); from the latest mod that has it.
fn texture_for(wanted: &str) -> Option<PathBuf> {
    let wanted_parts: Vec<&str> = wanted.split(['\\', '/']).filter(|s| !s.is_empty()).collect();
    let file = *wanted_parts.last()?;
    let (stem, ext) = file.rsplit_once('.')?;
    let sets = TEXTURES.read().unwrap().clone();
    let candidates = sets.iter().find_map(|t| t.get(stem))?;
    let score = |p: &PathBuf| {
        let parts: Vec<String> = p.components().map(|c| c.as_os_str().to_string_lossy().to_ascii_lowercase()).collect();
        let folders = parts.iter().rev().skip(1).zip(wanted_parts.iter().rev().skip(1)).take_while(|(a, b)| a.as_str() == **b).count();
        // a PNG copy usually sits in a renamed top folder ("x PNG"): one folder's grace
        let png = p.extension().is_some_and(|x| x.eq_ignore_ascii_case("png"));
        (folders + png as usize, png)
    };
    let best = candidates.iter().max_by_key(|p| score(p))?;
    // a cube map's PNG copy is flat: a cube is served as the mod made it
    let is_cube = |p: &PathBuf| {
        p.extension().is_some_and(|x| x.eq_ignore_ascii_case("dds"))
            && std::fs::File::open(p).ok().and_then(|f| {
                let mut h = [0u8; 116];
                read_exact(&f, &mut h, 0).then(|| u32::from_le_bytes(h[112..116].try_into().unwrap()) & 0x200 != 0)
            }) == Some(true)
    };
    let best = match candidates.iter().filter(|p| is_cube(p)).max_by_key(|p| score(p)) {
        Some(cube) => cube,
        None => best,
    };
    // (no cap (the 64-bit game's default): a PNG over 1024 that has a DDS of its own is
    // served as that DDS, its full size without a conversion; each would be a cache file of
    // several MB)
    let is = |p: &PathBuf, x: &str| p.extension().is_some_and(|e| e.eq_ignore_ascii_case(x));
    let best = if TEXTURE_CAP.load(Ordering::Relaxed) == 0 && ext.eq_ignore_ascii_case("dds") && is(best, "png") && png_side(best) > 1024 {
        let dds = candidates.iter().filter(|p| is(p, "dds")).max_by_key(|p| score(p)).unwrap_or(best);
        static SAID: AtomicU32 = AtomicU32::new(0);
        if SAID.fetch_add(1, Ordering::Relaxed) < 10 {
            crate::control::log(&format!("mods: full size: {} (its PNG is {} px)", dds.display(), png_side(best)));
        }
        dds
    } else {
        best
    };
    let best_ext = best.extension()?.to_string_lossy().to_ascii_lowercase();
    if best_ext == "png" {
        match png_as_dds(best).and_then(|p| capped(&p)) {
            Ok(p) => Some(p),
            Err(e) => {
                crate::control::log(&format!("mods: {}: {e}", best.display()));
                None
            }
        }
    } else if best_ext == ext && ext == "dds" {
        Some(capped(best).unwrap_or_else(|_| best.clone()))
    } else if best_ext == ext {
        Some(best.clone())
    } else {
        None
    }
}

static CONVERTED: AtomicU32 = AtomicU32::new(0);

/// The DDS without the mip levels above the cap (the file itself when it's small
/// enough, or not a plain 2D DXT/RGB texture with its mips), cached.
fn capped(dds: &Path) -> Result<PathBuf, String> {
    let cap = TEXTURE_CAP.load(Ordering::Relaxed);
    let f = std::fs::File::open(dds).map_err(|e| e.to_string())?;
    let mut h = [0u8; 128];
    if cap == 0 || !read_exact(&f, &mut h, 0) || &h[..4] != b"DDS " {
        return Ok(dds.to_path_buf());
    }
    let u = |at: usize| u32::from_le_bytes(h[at..at + 4].try_into().unwrap());
    let (height, width, mips, pf_flags, fourcc, bits, caps2) = (u(12), u(16), u(28).max(1), u(80), &h[84..88], u(88), u(112));
    let block = match fourcc {
        b"DXT1" => 8,
        b"DXT3" | b"DXT5" => 16,
        _ => 0,
    };
    let compressed = pf_flags & 0x4 != 0;
    if caps2 & 0x200 != 0 || (compressed && block == 0) || (!compressed && bits % 8 != 0) {
        return Ok(dds.to_path_buf());
    }
    let mut drop = 0;
    while (width >> drop).max(height >> drop) > cap && drop + 1 < mips {
        drop += 1;
    }
    if drop == 0 {
        return Ok(dds.to_path_buf());
    }
    let level_size = |k: u32| -> u64 {
        let (w, h) = ((width >> k).max(1) as u64, (height >> k).max(1) as u64);
        if compressed { w.div_ceil(4) * h.div_ceil(4) * block } else { w * h * (bits / 8) as u64 }
    };
    let mut hs = std::hash::DefaultHasher::new();
    stamp(dds, &mut hs);
    let stem = dds.file_stem().unwrap().to_string_lossy().to_ascii_lowercase();
    let out = mods_dir().join("cache").join(format!("{stem}-{:016x}-max{cap}.dds", std::hash::Hasher::finish(&hs)));
    if out.is_file() {
        return Ok(out);
    }
    let skip: u64 = (0..drop).map(level_size).sum();
    let keep: u64 = (drop..mips).map(level_size).sum();
    let mut data = vec![0u8; keep as usize];
    if !read_exact(&f, &mut data, 128 + skip) {
        return Ok(dds.to_path_buf());
    }
    let mut header = h;
    let put = |hd: &mut [u8; 128], at: usize, v: u32| hd[at..at + 4].copy_from_slice(&v.to_le_bytes());
    put(&mut header, 12, (height >> drop).max(1));
    put(&mut header, 16, (width >> drop).max(1));
    put(&mut header, 28, mips - drop);
    put(&mut header, 20, if compressed { level_size(drop) as u32 } else { (width >> drop).max(1) * bits / 8 });
    let mut bytes = header.to_vec();
    bytes.extend_from_slice(&data);
    std::fs::create_dir_all(out.parent().unwrap()).map_err(|e| e.to_string())?;
    let tmp = out.with_extension(format!("{}.tmp", std::process::id()));
    std::fs::write(&tmp, &bytes).map_err(|e| e.to_string())?;
    if std::fs::rename(&tmp, &out).is_err() {
        let _ = std::fs::remove_file(&tmp);
    }
    Ok(out)
}

/// A PNG compressed to a DDS (DXT1, or DXT5 with alpha; full mip chain), cached.
fn png_as_dds(png: &Path) -> Result<PathBuf, String> {
    let mut h = std::hash::DefaultHasher::new();
    stamp(png, &mut h);
    let stem = png.file_stem().unwrap().to_string_lossy().to_ascii_lowercase();
    let out = mods_dir().join("cache").join(format!("{stem}-{:016x}.dds", std::hash::Hasher::finish(&h)));
    if out.is_file() {
        return Ok(out);
    }
    let img = crate::textures::load_png(png)?;
    // DXT1 keeps 1-bit alpha (cut-outs, most of these): DXT5 only for real blending
    let alpha = img.levels[0].chunks_exact(4).any(|p| p[3] > 8 && p[3] < 247);
    let format = if alpha { texpresso::Format::Bc3 } else { texpresso::Format::Bc1 };
    let params = texpresso::Params { algorithm: texpresso::Algorithm::RangeFit, ..Default::default() };
    let (w, h) = (img.width as usize, img.height as usize);
    let fourcc = if alpha { *b"DXT5" } else { *b"DXT1" };
    let mut header = [0u32; 31];
    header[0] = 124;
    header[1] = 0x1 | 0x2 | 0x4 | 0x1000 | 0x20000 | 0x80000; // caps, height, width, pixel format, mip count, linear size
    header[2] = h as u32;
    header[3] = w as u32;
    header[4] = format.compressed_size(w, h) as u32;
    header[6] = img.levels.len() as u32;
    header[18] = 32; // pixel format: size, flags (four cc), four cc
    header[19] = 0x4;
    header[20] = u32::from_le_bytes(fourcc);
    header[26] = 0x1000 | 0x400000 | 0x8; // texture, mipmap, complex
    let mut dds = b"DDS ".to_vec();
    dds.extend(header.iter().flat_map(|v| v.to_le_bytes()));
    let (mut lw, mut lh) = (w, h);
    for level in &img.levels {
        let start = dds.len();
        dds.resize(start + format.compressed_size(lw, lh), 0);
        format.compress(level, lw, lh, params, &mut dds[start..]);
        (lw, lh) = ((lw / 2).max(1), (lh / 2).max(1));
    }
    std::fs::create_dir_all(out.parent().unwrap()).map_err(|e| e.to_string())?;
    let tmp = out.with_extension(format!("{}.tmp", std::process::id()));
    std::fs::write(&tmp, &dds).map_err(|e| e.to_string())?;
    if std::fs::rename(&tmp, &out).is_err() {
        let _ = std::fs::remove_file(&tmp); // another thread got there first
    }
    CONVERTED.fetch_add(1, Ordering::Relaxed);
    Ok(out)
}

// ---- the manager thread ----------------------------------------------------------

fn manager() {
    let game = match load_game() {
        Ok(g) => g,
        Err(e) => {
            crate::control::log(&format!("mods: off: {e}"));
            *STARTUP.lock().unwrap() = Startup::Ready(None);
            STARTUP_READY.notify_all();
            return;
        }
    };
    let mut mods: HashMap<PathBuf, Result<Mod, String>> = HashMap::new();
    let mut last = None;
    let mut first = true;
    loop {
        let list = std::fs::read_to_string(list_path()).ok();
        if list != last || first {
            last = list;
            let config = configured();
            TEXTURE_CAP.store(read_texture_cap(), Ordering::Relaxed);
            for (root, on) in &config {
                if *on && !mods.contains_key(root) {
                    let m = prepare(root, &game).inspect_err(|e| crate::control::log(&format!("mods: {}: {e}", root.display())));
                    mods.insert(root.clone(), m);
                }
            }
            let all: Vec<&Mod> = mods.values().flatten().collect();
            let on: Vec<&Mod> = config.iter().filter(|(_, on)| *on).filter_map(|(r, _)| mods.get(r).and_then(|m| m.as_ref().ok())).collect();
            *MOD_REPORT.lock().unwrap() = config
                .iter()
                .filter_map(|(root, _)| {
                    let line = match mods.get(root)? {
                        Ok(m) => format!("{}|{}|{}|", m.changes.len(), m.removed.len(), m.textures.values().map(Vec::len).sum::<usize>()),
                        Err(e) => format!("0|0|0|{}", e.replace('|', "/")),
                    };
                    Some(format!("mod|{}|{line}", root.display()))
                })
                .collect();
            write_status();
            *TEXTURES.write().unwrap() = on.iter().rev().map(|m| Arc::new(m.textures.clone())).collect();
            let w = wanted(&game, &all, &on);
            if first {
                // the directory the game will load: its own, with the enabled mods'
                // entries (and new names) in it
                let dir = (!on.is_empty()).then(|| {
                    let mut entries = game.entries.clone();
                    let mut index: HashMap<String, usize> = entries.iter().enumerate().map(|(i, e)| (entry_name(&e.name), i)).collect();
                    for m in &on {
                        for (name, off, size, bytes) in &m.changes {
                            let e = Entry { offset: *off, size: *size, name: *bytes };
                            match index.get(name) {
                                Some(&i) => entries[i] = e,
                                None => {
                                    index.insert(name.clone(), entries.len());
                                    entries.push(e);
                                }
                            }
                        }
                    }
                    // deleted models: the empty model (or, without one, left out as in
                    // the mod's own directory)
                    for e in entries.iter_mut() {
                        if let Some(&(off, size, _)) = w.get(&entry_name(&e.name))
                            && Some((off, size)) == empty_model()
                        {
                            (e.offset, e.size) = (off, size);
                        }
                    }
                    entries.retain(|e| w.get(&entry_name(&e.name)).is_none_or(|v| v.1 != 0));
                    let mut out = Vec::with_capacity(entries.len() * 32);
                    for e in &entries {
                        out.extend_from_slice(&e.offset.to_le_bytes());
                        out.extend_from_slice(&e.size.to_le_bytes());
                        out.extend_from_slice(&e.name);
                    }
                    out
                });
                crate::control::log(&format!("mods: start-up: {} of {} on", on.len(), config.len()));
                *STARTUP.lock().unwrap() = Startup::Ready(dir);
                STARTUP_READY.notify_all();
                first = false;
            } else {
                crate::control::log(&format!("mods: list changed: {} of {} on", on.len(), config.len()));
                *PENDING.lock().unwrap() = Some(w);
            }
        }
        std::thread::sleep(Duration::from_millis(500));
    }
}

// ---- the game thread: the streaming table -----------------------------------------

/// Names -> the game's ids, from the directory it loaded and its streaming table.
static IDS: Mutex<Option<HashMap<String, u32>>> = Mutex::new(None);

fn slot_info(id: u32) -> Option<*mut u8> {
    type SlotOf = unsafe extern "C" fn(u32) -> i32;
    let slot = unsafe { std::mem::transmute::<usize, SlotOf>(SLOT_OF)(id) };
    (slot >= 0).then(|| (STREAMING_INFO + slot as usize * 16) as *mut u8)
}

fn rd<T: Copy>(p: *const u8, off: usize) -> T {
    unsafe { (p.add(off) as *const T).read_unaligned() }
}

fn find_ids() -> HashMap<String, u32> {
    let mut by_pos: HashMap<(u32, u32), u32> = HashMap::new();
    for id in 0..LAST_ID {
        if let Some(info) = slot_info(id) {
            let (off, size) = (rd::<u32>(info, 8), rd::<u32>(info, 12));
            if size != 0 {
                by_pos.entry((off, size)).or_insert(id);
            }
        }
    }
    let dir = LOADED_DIR.lock().unwrap().clone().unwrap_or_default();
    // (the names sharing the empty model's position are found by name, in tick)
    parse_dir(&dir)
        .iter()
        .filter(|e| Some((e.offset, e.size)) != empty_model())
        .filter_map(|e| Some((entry_name(&e.name), *by_pos.get(&(e.offset, e.size))?)))
        .collect()
}

/// A model's id by name, as the game's directory loader finds it (for models left
/// out of the directory).
fn model_id(name: &str) -> Option<u32> {
    let stem = std::ffi::CString::new(name.strip_suffix(".nif")?).ok()?;
    let id = if unsafe { *(HASHED_NAMES as *const u8) } != 0 {
        let f: unsafe extern "C" fn(*const i8) -> i32 = unsafe { std::mem::transmute(MODEL_BY_NAME_HASHED) };
        unsafe { f(stem.as_ptr()) }
    } else {
        let f: unsafe extern "C" fn(*const i8, *mut i32) -> usize = unsafe { std::mem::transmute(MODEL_BY_NAME) };
        let mut id = -1;
        unsafe { f(stem.as_ptr(), &mut id) };
        id
    };
    (0..TXD_BASE as i32).contains(&id).then_some(id as u32)
}

/// Points the streaming table where `w` says and makes the game reload what changed.
/// Returns what couldn't be done yet (entries being streamed right now).
fn apply(w: Wanted, ids: &HashMap<String, u32>) -> Wanted {
    let mut busy = Wanted::new();
    let (mut changed, mut restart) = (0, 0);
    let mut models = HashSet::new();
    let mut txds = Vec::new();
    let mut moved = HashSet::new();
    let max = unsafe { *(MAX_ENTRY as *const u32) };
    for (name, (off, size, bytes)) in w {
        let Some(&id) = ids.get(&name) else {
            restart += 1;
            continue;
        };
        if size > max {
            restart += 1;
            continue;
        }
        let Some(info) = slot_info(id) else { continue };
        if (rd::<u32>(info, 8), rd::<u32>(info, 12)) == (off, size) {
            continue;
        }
        let state: u8 = rd(info, 4);
        if state > 1 {
            busy.insert(name, (off, size, bytes));
            continue;
        }
        unsafe {
            (info.add(8) as *mut u32).write_unaligned(off);
            (info.add(12) as *mut u32).write_unaligned(size);
        }
        moved.insert(id);
        changed += 1;
        if state == 1 {
            if id < TXD_BASE {
                models.insert(id);
            } else if id < COL_BASE {
                txds.push(id);
            }
        }
    }
    cut_chains(&moved);
    // a reloaded texture dictionary: the models using it reload too
    for &txd in &txds {
        let slot = (txd - TXD_BASE) as i16;
        for id in 0..TXD_BASE {
            let mi = unsafe { *((MODEL_INFOS + id as usize * 4) as *const u32) } as usize;
            if mi != 0 && rd::<i16>(mi as *const u8, 0x16) == slot {
                models.insert(id);
            }
        }
    }
    // the buildings' and objects' geometry, for the models that change
    let mut deleted = 0;
    if !models.is_empty() {
        #[cfg(target_arch = "x86")]
        type DeleteRw = unsafe extern "thiscall" fn(*mut u8);
        #[cfg(target_arch = "x86_64")]
        type DeleteRw = unsafe extern "C" fn(*mut u8);
        let delete: DeleteRw = unsafe { std::mem::transmute(DELETE_RW_OBJECT) };
        for pool in ENTITY_POOLS {
            let p = unsafe { *(pool as *const u32) } as usize as *const u8;
            if p.is_null() {
                continue;
            }
            // (the game's pool: 4-byte pointers in either game)
            let (objects, flags, count, stride) = (rd::<u32>(p, 0) as usize, rd::<u32>(p, 4) as usize, rd::<u32>(p, 8), rd::<u32>(p, 12));
            for i in 0..count as usize {
                if unsafe { *((flags + i) as *const u8) } & 0x80 != 0 {
                    continue;
                }
                let e = (objects + i * stride as usize) as *mut u8;
                if rd::<u32>(e, 0x18) == 0 || !models.contains(&(rd::<i16>(e, 0x10E) as u32)) {
                    continue;
                }
                if rd::<u32>(e, 0x80) != 0 || rd::<u32>(e, 0x58) != 0 {
                    continue;
                }
                unsafe { delete(e) };
                deleted += 1;
            }
        }
    }
    type Remove = unsafe extern "C" fn(u32);
    let remove: Remove = unsafe { std::mem::transmute(REMOVE_MODEL) };
    let (mut reloaded, mut in_use) = (0, 0);
    for &id in &models {
        let Some(info) = slot_info(id) else { continue };
        let mi = unsafe { *((MODEL_INFOS + id as usize * 4) as *const u32) } as usize;
        if rd::<u8>(info, 4) != 1 || mi == 0 {
            continue;
        }
        if rd::<i16>(mi as *const u8, 0x14) != 0 || rd::<u8>(info, 5) & 1 != 0 {
            in_use += 1;
            continue;
        }
        unsafe { remove(id) };
        reloaded += 1;
    }
    type TxdRefs = unsafe extern "C" fn(u32) -> i32;
    let txd_refs: TxdRefs = unsafe { std::mem::transmute(TXD_REFS) };
    for &id in &txds {
        let Some(info) = slot_info(id) else { continue };
        if rd::<u8>(info, 4) != 1 {
            continue;
        }
        if unsafe { txd_refs(id - TXD_BASE) } != 0 || rd::<u8>(info, 5) & 1 != 0 {
            in_use += 1;
            continue;
        }
        unsafe { remove(id) };
        reloaded += 1;
    }
    if changed + restart > 0 {
        let mut s = format!("Last change: {reloaded} models and texture packs reloaded");
        if in_use > 0 {
            s += &format!(", {in_use} in use (they change when they respawn)");
        }
        if restart > 0 {
            s += &format!(", {restart} files need a restart");
        }
        *LAST_APPLY.lock().unwrap() = s;
        write_status();
        crate::control::log(&format!(
            "mods: {changed} entries changed: {reloaded} reloading ({deleted} buildings/objects), {in_use} in use (they change when respawned), {restart} need a restart, {} busy",
            busy.len()
        ));
    }
    busy
}

/// Takes these entries out of the game's read-together chains (see the top).
fn cut_chains(ids: &HashSet<u32>) {
    if ids.is_empty() {
        return;
    }
    for id in 0..LAST_ID {
        if let Some(info) = slot_info(id) {
            let next = rd::<i16>(info, 6);
            if ids.contains(&id) || (next >= 0 && ids.contains(&(next as u32))) {
                unsafe { (info.add(6) as *mut i16).write_unaligned(-1) };
            }
        }
    }
}

/// In place of the game's call of CStreaming::LoadCdDirectory: then the entries it
/// pointed into our regions (the start-up World.dir) come out of the chains, before
/// anything is streamed.
unsafe extern "C" fn load_cd_directory(image: u32) {
    let original: unsafe extern "C" fn(u32) = unsafe { std::mem::transmute(ORIGINAL_LOAD_CD_DIRECTORY.load(Ordering::SeqCst)) };
    unsafe { original(image) };
    let base = BASE.load(Ordering::SeqCst) / SECTOR;
    let ours: HashSet<u32> = (0..LAST_ID)
        .filter(|&id| {
            slot_info(id).is_some_and(|i| {
                let (off, size) = (rd::<u32>(i, 8), rd::<u32>(i, 12));
                size != 0 && (off as u64 >= base || Some((off, size)) == empty_model())
            })
        })
        .collect();
    cut_chains(&ours);
    crate::control::log(&format!("mods: the game's directory is loaded ({} entries from mods)", ours.len()));
    DIR_LOADED.store(true, Ordering::SeqCst);
}

/// Once a frame, on the game's thread.
pub fn tick() {
    if !DIR_LOADED.load(Ordering::Relaxed) {
        return;
    }
    let Some(w) = PENDING.lock().unwrap().take() else { return };
    let mut ids = IDS.lock().unwrap();
    let ids = ids.get_or_insert_with(|| {
        let ids = find_ids();
        crate::control::log(&format!("mods: {} names in the streaming table", ids.len()));
        ids
    });
    for name in w.keys() {
        if !ids.contains_key(name)
            && let Some(id) = model_id(name)
        {
            ids.insert(name.clone(), id);
        }
    }
    let busy = apply(w, ids);
    if !busy.is_empty() {
        PENDING.lock().unwrap().get_or_insert(busy);
    }
}

// ---- the hooks -------------------------------------------------------------------

/// The game's open World.img handles: whether overlapped, and (if not) the position.
struct Img {
    pos: u64,
}

static IMGS: Mutex<Option<HashMap<usize, Img>>> = Mutex::new(None);
static SERVED: AtomicU32 = AtomicU32::new(0);

fn open_ours(path: &Path) -> *mut c_void {
    let w = wide(&path.to_string_lossy());
    // GENERIC_READ, share read, OPEN_EXISTING, normal
    unsafe { CreateFileW(w.as_ptr(), 0x8000_0000, 0x1, std::ptr::null_mut(), 3, 0x80, std::ptr::null_mut()) }
}

fn valid(h: *mut c_void) -> bool {
    !h.is_null() && h as isize != -1
}

unsafe extern "system" fn create_file_a(name: *const i8, access: u32, share: u32, sa: *mut c_void, disposition: u32, flags: u32, template: *mut c_void) -> *mut c_void {
    let call = |n: *const i8| unsafe { real::<CreateFileA>(&REAL_CREATE_FILE_A)(n, access, share, sa, disposition, flags, template) };
    if name.is_null() {
        return call(name);
    }
    let path = unsafe { CStr::from_ptr(name) }.to_string_lossy().to_ascii_lowercase().replace('/', "\\");
    // testing: bullygfx_mods\pure.txt: the first enabled mod's archive as the game's,
    // whole (as if installed), to compare with the merge
    if (path.ends_with("stream\\world.dir") || path.ends_with("stream\\world.img")) && mods_dir().join("pure.txt").is_file() {
        if let Some((dir, img)) = configured().into_iter().filter(|m| m.1).find_map(|m| archive_of(&m.0)) {
            crate::control::log(&format!("mods: pure: {path} -> the mod's"));
            return open_ours(if path.ends_with("dir") { &dir } else { &img });
        }
    }
    if path.ends_with("stream\\world.dir") {
        let dir = startup_dir();
        let h = match &dir {
            Some(bytes) => {
                let file = mods_dir().join("World.dir");
                // (written only when it changed, through a temporary file: another copy of the
                // game starting at the same time may be reading it)
                if std::fs::read(&file).ok().as_ref() != Some(bytes) {
                    let tmp = mods_dir().join(format!("World.dir.{}.tmp", std::process::id()));
                    if std::fs::write(&tmp, bytes).is_ok() && std::fs::rename(&tmp, &file).is_err() {
                        let _ = std::fs::remove_file(&tmp);
                    }
                }
                crate::control::log("mods: the game gets the World.dir with the mods in it");
                open_ours(&file)
            }
            None => call(name),
        };
        if valid(h) {
            *LOADED_DIR.lock().unwrap() = dir.or_else(|| std::fs::read(crate::win::game_file("Stream").join("World.dir")).ok());
        }
        return h;
    }
    let h = call(name);
    if LOG_OPENS {
        crate::control::log(&format!("open {path} -> {}", if valid(h) { "ok" } else { "missing" }));
    }
    if path.ends_with("stream\\world.img") && valid(h) {
        let _ = flags & FILE_FLAG_OVERLAPPED;
        IMGS.lock().unwrap().get_or_insert_default().insert(h as usize, Img { pos: 0 });
    }
    if !valid(h) && [".dds", ".tga", ".bmp", ".png"].iter().any(|x| path.ends_with(x)) {
        if let Some(file) = texture_for(&path) {
            let h2 = open_ours(&file);
            if valid(h2) {
                let n = SERVED.fetch_add(1, Ordering::Relaxed);
                if n < 20 || n.is_power_of_two() {
                    crate::control::log(&format!("mods: texture #{} {path} -> {} ({} converted so far)", n + 1, file.display(), CONVERTED.load(Ordering::Relaxed)));
                }
                return h2;
            }
        }
    }
    h
}

unsafe extern "system" fn read_file(h: *mut c_void, buf: *mut u8, n: u32, read: *mut u32, ovl: *mut Overlapped) -> i32 {
    let call = || unsafe { real::<ReadFile>(&REAL_READ_FILE)(h, buf, n, read, ovl) };
    let Some(pos) = IMGS.lock().unwrap().as_ref().and_then(|m| m.get(&(h as usize)).map(|i| i.pos)) else { return call() };
    let at = if ovl.is_null() { pos } else { unsafe { ((*ovl).offset_high as u64) << 32 | (*ovl).offset as u64 } };
    let base = BASE.load(Ordering::SeqCst);
    let got = if base != 0 && at >= base {
        let got = read_regions(at, unsafe { std::slice::from_raw_parts_mut(buf, n as usize) }) as u32;
        unsafe {
            if !read.is_null() {
                *read = got;
            }
            if !ovl.is_null() {
                (*ovl).internal = 0;
                (*ovl).internal_high = got as usize;
                if !(*ovl).event.is_null() {
                    SetEvent((*ovl).event);
                }
            }
        }
        got
    } else {
        let r = call();
        if r == 0 || read.is_null() || !ovl.is_null() {
            return r;
        }
        unsafe { *read }
    };
    if ovl.is_null() {
        if let Some(i) = IMGS.lock().unwrap().as_mut().and_then(|m| m.get_mut(&(h as usize))) {
            i.pos = at + got as u64;
        }
    }
    1
}

unsafe extern "system" fn set_file_pointer(h: *mut c_void, dist: i32, high: *mut i32, method: u32) -> u32 {
    let call = |d: i32, hi: *mut i32, m: u32| unsafe { real::<SetFilePointer>(&REAL_SET_FILE_POINTER)(h, d, hi, m) };
    let Some(pos) = IMGS.lock().unwrap().as_ref().and_then(|m| m.get(&(h as usize)).map(|i| i.pos)) else {
        return call(dist, high, method);
    };
    let base = BASE.load(Ordering::SeqCst);
    let d = if high.is_null() { dist as i64 } else { (unsafe { *high } as i64) << 32 | dist as u32 as i64 };
    let new = match method {
        0 => d,
        1 => pos as i64 + d,
        2 => total() as i64 + d,
        _ => return call(dist, high, method),
    };
    if new < 0 || base == 0 {
        return call(dist, high, method);
    }
    let new = new as u64;
    let r = if new < base {
        let mut hi = (new >> 32) as i32;
        call(new as u32 as i32, &mut hi, 0)
    } else {
        new as u32
    };
    if !high.is_null() {
        unsafe { *high = (new >> 32) as i32 };
    }
    if let Some(i) = IMGS.lock().unwrap().as_mut().and_then(|m| m.get_mut(&(h as usize))) {
        i.pos = new;
    }
    r
}

unsafe extern "system" fn get_file_size(h: *mut c_void, high: *mut u32) -> u32 {
    let tracked = IMGS.lock().unwrap().as_ref().is_some_and(|m| m.contains_key(&(h as usize)));
    if tracked && BASE.load(Ordering::SeqCst) != 0 {
        let total = total();
        if !high.is_null() {
            unsafe { *high = (total >> 32) as u32 };
        }
        return total as u32;
    }
    unsafe { real::<GetFileSize>(&REAL_GET_FILE_SIZE)(h, high) }
}

unsafe extern "system" fn close_handle(h: *mut c_void) -> i32 {
    if let Some(m) = IMGS.lock().unwrap().as_mut() {
        m.remove(&(h as usize));
    }
    unsafe { real::<CloseHandle>(&REAL_CLOSE_HANDLE)(h) }
}

unsafe fn patch(slot: usize, hook: usize, real: &AtomicUsize) {
    // (the game's import slot: 4 bytes in either game; in the 64-bit one it holds a jump
    // below 2 GB to the export, and gets one to the hook)
    let p = slot as *mut u32;
    if real.load(Ordering::SeqCst) != 0 {
        return;
    }
    let Some(target) = crate::lowmem::slot_target(hook) else { return };
    let mut old = 0;
    unsafe {
        VirtualProtect(p as *mut c_void, 4, PAGE_EXECUTE_READWRITE, &mut old);
        real.store(*p as usize, Ordering::SeqCst);
        *p = target;
        VirtualProtect(p as *mut c_void, 4, old, &mut old);
    }
}

/// What load_cd_directory runs first: the game's LoadCdDirectory (in the 64-bit game the
/// body its stub jumps to, the stub itself being pointed at ours).
static ORIGINAL_LOAD_CD_DIRECTORY: AtomicUsize = AtomicUsize::new(LOAD_CD_DIRECTORY);

/// The 64-bit game: LoadCdDirectory's stub (jmp rel32 to its body) pointed at ours, through
/// a jump below 2 GB (rel32 reaches it from the stub)
#[cfg(target_arch = "x86_64")]
unsafe fn hook_load_cd_directory() -> bool {
    let at = LOAD_CD_DIRECTORY as *mut u8;
    if unsafe { *at } != 0xE9 {
        return false;
    }
    let body = (LOAD_CD_DIRECTORY as i64 + 5 + unsafe { (at.add(1) as *const i32).read_unaligned() } as i64) as usize;
    let Some(target) = crate::lowmem::slot_target(load_cd_directory as *const () as usize) else { return false };
    ORIGINAL_LOAD_CD_DIRECTORY.store(body, Ordering::SeqCst);
    let rel = (target as i64 - (LOAD_CD_DIRECTORY as i64 + 5)) as i32;
    let mut old = 0;
    unsafe {
        VirtualProtect(at as *mut c_void, 5, PAGE_EXECUTE_READWRITE, &mut old);
        (at.add(1) as *mut i32).write_unaligned(rel);
        VirtualProtect(at as *mut c_void, 5, old, &mut old);
        FlushInstructionCache(GetCurrentProcess(), at as *const c_void, 5);
    }
    true
}

/// From DllMain: hooks the game's file access and starts the mod manager (the game
/// opens World.dir well after this; that waits for the enabled mods to be ready).
pub fn install() {
    *STARTUP.lock().unwrap() = Startup::Building;
    std::thread::spawn(manager);
    unsafe {
        patch(IAT_CREATE_FILE_A, create_file_a as *const () as usize, &REAL_CREATE_FILE_A);
        patch(IAT_READ_FILE, read_file as *const () as usize, &REAL_READ_FILE);
        patch(IAT_SET_FILE_POINTER, set_file_pointer as *const () as usize, &REAL_SET_FILE_POINTER);
        patch(IAT_GET_FILE_SIZE, get_file_size as *const () as usize, &REAL_GET_FILE_SIZE);
        patch(IAT_CLOSE_HANDLE, close_handle as *const () as usize, &REAL_CLOSE_HANDLE);
        #[cfg(target_arch = "x86_64")]
        if !hook_load_cd_directory() {
            crate::control::log("mods: LoadCdDirectory isn't where it should be: live changes only");
        }
        // the call of LoadCdDirectory (E8 rel32) goes through ours
        #[cfg(target_arch = "x86")]
        {
            let at = LOAD_CD_DIRECTORY_CALL as *mut u8;
            let rel = |target: usize| (target as i64 - (LOAD_CD_DIRECTORY_CALL as i64 + 5)) as i32;
            if *at == 0xE8 && (at.add(1) as *const i32).read_unaligned() == rel(LOAD_CD_DIRECTORY) {
                let mut old = 0;
                VirtualProtect(at as *mut c_void, 5, PAGE_EXECUTE_READWRITE, &mut old);
                (at.add(1) as *mut i32).write_unaligned(rel(load_cd_directory as *const () as usize));
                VirtualProtect(at as *mut c_void, 5, old, &mut old);
            } else {
                crate::control::log("mods: LoadCdDirectory isn't where it should be: live changes only");
            }
        }
    }
}
