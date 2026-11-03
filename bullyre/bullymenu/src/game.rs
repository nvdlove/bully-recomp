//! The running game: launching it (patched, windowed), attaching to it, and
//! reading/writing its memory.

use std::ffi::c_void;
use std::path::{Path, PathBuf};
use std::ptr::null_mut;

use crate::offsets::{self as off, Patch};
use crate::win::*;

/// Where Bully is when no folder has been picked: Steam's.
pub const DEFAULT_GAME_DIR: &str = r"C:\Program Files (x86)\Steam\steamapps\common\Bully Scholarship Edition";
static GAME_DIR: std::sync::Mutex<Option<PathBuf>> = std::sync::Mutex::new(None);

/// The game's folder: the one picked on the Launch page, else Steam's.
pub fn game_dir() -> PathBuf {
    GAME_DIR.lock().unwrap().clone().unwrap_or_else(|| PathBuf::from(DEFAULT_GAME_DIR))
}

pub fn set_game_dir(dir: &str) {
    let dir = dir.trim().trim_end_matches(['\\', '/']);
    *GAME_DIR.lock().unwrap() = (!dir.is_empty()).then(|| PathBuf::from(dir));
}

/// What's wrong with a folder as Bully's (None: it has the game).
pub fn check_game_dir(dir: &Path) -> Option<String> {
    if !dir.is_dir() {
        return Some(format!("{} isn't there", dir.display()));
    }
    let has_exe = dir.join("Bully.exe").exists() || dir.join(GAME_EXE).exists();
    if !has_exe || !dir.join("Stream").is_dir() || !dir.join("Config").is_dir() {
        return Some(format!("{} doesn't look like Bully: Scholarship Edition (no Bully.exe, Stream and Config)", dir.display()));
    }
    None
}

/// The 64-bit remaster (bullyre): its exe and renderer sit beside the menu.
pub const X64_EXE: &str = "bullyre_x64.exe";
pub const X64_GFX: &str = "bullygfx64.dll";
/// The SteamStub-free exe bullydumper writes. It has to sit in the game folder:
/// the game finds its data next to its own exe.
pub const GAME_EXE: &str = "Bully_unpacked.exe";
/// bullyre's own exe (bullyre/src/host): maps GAME_EXE into itself and runs it with the
/// reversed code built in, instead of the game's exe loading bullyre.dll.
pub const HOST_EXE: &str = "bullyre_host.exe";

pub struct LaunchOptions {
    pub windowed: bool,
    pub width: u32,
    pub height: u32,
    pub background: bool, // keep running (unpaused) when the window loses focus
    pub free_mouse: bool, // don't let DirectInput trap the cursor
    pub host: bool,       // run from bullyre's own exe (HOST_EXE) when it's in the game folder
    pub x64: bool,        // the 64-bit remaster (X64_EXE beside the menu) on the game folder
    pub seamless: bool,   // its seamless doors (off: the game's own, with their fade)
}

pub struct Game {
    pub process: HANDLE,
    pub pid: u32,
    pub launched: bool, // we started it, so the start-up patches are in
    /// The 64-bit remaster: its data is the game's (same addresses), its code isn't (no code
    /// patches, nothing run inside it, its window its own)
    pub x64: bool,
}

/// A process is 64-bit (not under WOW64) on 64-bit Windows.
fn is_x64(process: HANDLE) -> bool {
    let mut wow = 0;
    unsafe { IsWow64Process(process, &mut wow) != 0 && wow == 0 }
}

impl Drop for Game {
    fn drop(&mut self) {
        unsafe { CloseHandle(self.process) };
    }
}

fn window_of(pid: u32) -> Option<HWND> {
    let class = wide("Gamebryo Application");
    let h = unsafe { FindWindowW(class.as_ptr(), std::ptr::null()) };
    if h.is_null() {
        return None;
    }
    let mut owner = 0;
    unsafe { GetWindowThreadProcessId(h, &mut owner) };
    (pid == 0 || owner == pid).then_some(h)
}

impl Game {
    /// Attach to a Bully that is already running.
    pub fn find() -> Option<Game> {
        let h = window_of(0)?;
        let mut pid = 0;
        unsafe { GetWindowThreadProcessId(h, &mut pid) };
        let process = unsafe { OpenProcess(PROCESS_ALL_ACCESS, 0, pid) };
        (!process.is_null()).then(|| Game { process, pid, launched: false, x64: is_x64(process) })
    }

    pub fn window(&self) -> Option<HWND> {
        window_of(self.pid)
    }

    pub fn alive(&self) -> bool {
        let mut code = 0;
        unsafe { GetExitCodeProcess(self.process, &mut code) != 0 && code == STILL_ACTIVE }
    }

    pub fn read_bytes(&self, addr: u32, buf: &mut [u8]) -> bool {
        let mut n = 0;
        unsafe { ReadProcessMemory(self.process, addr as usize as *const c_void, buf.as_mut_ptr() as *mut c_void, buf.len(), &mut n) != 0 && n == buf.len() }
    }

    pub fn write_u32(&self, addr: u32, v: u32) -> bool {
        self.write_bytes(addr, &v.to_le_bytes())
    }

    pub fn u32(&self, addr: u32) -> Option<u32> {
        let mut b = [0u8; 4];
        self.read_bytes(addr, &mut b).then(|| u32::from_le_bytes(b))
    }

    pub fn f32(&self, addr: u32) -> Option<f32> {
        self.u32(addr).map(f32::from_bits)
    }

    pub fn write_bytes(&self, addr: u32, data: &[u8]) -> bool {
        let mut n = 0;
        unsafe { WriteProcessMemory(self.process, addr as usize as *mut c_void, data.as_ptr() as *const c_void, data.len(), &mut n) != 0 && n == data.len() }
    }

    /// Makes the (still suspended) game load `dll` from its folder before any of its
    /// own code runs: its entry point (call __security_init_cookie) becomes a jump to
    /// a stub that LoadLibraryA's the dll (through the game's own import), makes that
    /// call, and carries on.
    pub fn load_at_start(&self, dll: &str) -> Result<(), String> {
        const ENTRY: u32 = 0x0086_0E31;
        const COOKIE_INIT: u32 = 0x0087_1CF2;
        const AFTER: u32 = 0x0086_0E36;
        const IAT_LOAD_LIBRARY_A: u32 = 0x008F_C0C8;
        let expected = [0xE8u8, 0xBC, 0x0E, 0x01, 0x00];
        let mut cur = [0u8; 5];
        if !self.read_bytes(ENTRY, &mut cur) || cur != expected {
            return Err(format!("{dll}: the game's entry point isn't what it should be"));
        }
        let mem = unsafe { VirtualAllocEx(self.process, null_mut(), 0x100, 0x3000, PAGE_EXECUTE_READWRITE) };
        if mem.is_null() || mem as usize > u32::MAX as usize {
            return Err(format!("{dll}: couldn't allocate memory in the game"));
        }
        let stub = mem as usize as u32;
        let path = stub + 0x40;
        let rel = |from_next: u32, to: u32| to.wrapping_sub(from_next).to_le_bytes();
        let mut code = vec![0x68u8]; // push path
        code.extend(path.to_le_bytes());
        code.extend([0xFF, 0x15]); // call [LoadLibraryA]
        code.extend(IAT_LOAD_LIBRARY_A.to_le_bytes());
        code.push(0xE8); // call __security_init_cookie (what the entry point did)
        code.extend(rel(stub + code.len() as u32 + 4, COOKIE_INIT));
        code.push(0xE9); // jmp on
        code.extend(rel(stub + code.len() as u32 + 4, AFTER));
        let mut name = dll.as_bytes().to_vec();
        name.push(0);
        let mut jump = vec![0xE9u8];
        jump.extend(rel(ENTRY + 5, stub));
        if !self.write_bytes(stub, &code) || !self.write_bytes(path, &name) || !self.write_bytes(ENTRY, &jump) {
            return Err(format!("{dll}: couldn't write into the game"));
        }
        Ok(())
    }

    /// Applies a code patch if the original bytes are there. Already patched is fine.
    pub fn patch(&self, p: &Patch) -> Result<(), String> {
        let mut cur = vec![0u8; p.old.len()];
        if !self.read_bytes(p.addr, &mut cur) {
            return Err(format!("{}: can't read {:#x}", p.name, p.addr));
        }
        if cur == p.new {
            return Ok(());
        }
        if cur != p.old {
            return Err(format!("{}: unexpected bytes at {:#x} (different game version?)", p.name, p.addr));
        }
        let addr = p.addr as usize as *mut c_void;
        let mut old = 0;
        unsafe {
            VirtualProtectEx(self.process, addr, p.new.len(), PAGE_EXECUTE_READWRITE, &mut old);
            let ok = self.write_bytes(p.addr, p.new);
            VirtualProtectEx(self.process, addr, p.new.len(), old, &mut old);
            FlushInstructionCache(self.process, addr, p.new.len());
            if ok { Ok(()) } else { Err(format!("{}: write failed", p.name)) }
        }
    }

    /// Patches that are safe on a game that is already running.
    pub fn runtime_patches(&self, windowed: bool, background: bool) -> Vec<String> {
        // the minimize is gone, so the game always runs on unfocused: streaming must too
        let mut list = vec![&off::NO_DISPLAY_CHANGE, &off::NO_MINIMIZE, &off::NO_STREAM_LOCK_UNFOCUSED, &off::NO_MIN_TIMESTEP, &off::CDSTREAM_SYNC];
        if windowed {
            list.extend([&off::NO_MOVE_ON_FOCUS, &off::NO_MOVE_ON_RESIZE]);
        }
        if background {
            list.extend([&off::RUN_UNFOCUSED, &off::LOADING_UNFOCUSED, &off::NO_PAUSE_UNFOCUSED]);
        }
        let errs: Vec<String> = list.into_iter().filter_map(|p| self.patch(p).err()).collect();
        if background {
            self.write_bytes(off::PAUSED_BY_FOCUS, &[0]);
        }
        errs
    }

    /// (frames simulated so far, this frame's time step in 1/50 s)
    pub fn timer(&self) -> Option<(u32, f32)> {
        Some((self.u32(off::FRAME_COUNT)?, self.f32(off::TIME_STEP)?))
    }

    /// The game's own "is the mouse free" state: patched before start-up.
    pub fn mouse_is_shared(&self) -> bool {
        let mut b = [0u8; 2];
        self.read_bytes(off::MOUSE_SHARED.addr, &mut b) && b == off::MOUSE_SHARED.new
    }

    /// Switches the keyboard and mouse of a running game to non-exclusive mode, the
    /// way the game set them up but with DISCL_NONEXCLUSIVE: a small routine runs on
    /// its own thread inside the game and, per device, calls Unacquire,
    /// SetCooperativeLevel(hwnd, NONEXCLUSIVE | FOREGROUND) (retrying if the game
    /// re-acquires in between) and Acquire. The start-up patches are then written
    /// too, so a later re-initialisation stays non-exclusive.
    pub fn free_input(&self) -> Result<(), String> {
        const MEM_COMMIT_RESERVE: u32 = 0x3000;
        const MEM_RELEASE: u32 = 0x8000;
        let mem = unsafe { VirtualAllocEx(self.process, null_mut(), 0x200, MEM_COMMIT_RESERVE, PAGE_EXECUTE_READWRITE) };
        if mem.is_null() || mem as usize > u32::MAX as usize {
            return Err("couldn't allocate memory in the game".into());
        }
        let base = mem as usize as u32;
        let results = base + 0x100;
        if !self.write_bytes(base, &free_input_code(base)) || !self.write_bytes(results, &[0xEE; 8]) {
            return Err("couldn't write into the game".into());
        }
        let t = unsafe { CreateRemoteThread(self.process, null_mut(), 0, mem, null_mut(), 0, null_mut()) };
        if t.is_null() {
            unsafe { VirtualFreeEx(self.process, mem, 0, MEM_RELEASE) };
            return Err("couldn't start the input fix in the game".into());
        }
        let waited = unsafe { WaitForSingleObject(t, 5000) };
        unsafe { CloseHandle(t) };
        let mut r = [0u8; 8];
        let read = self.read_bytes(results, &mut r);
        if waited == 0 {
            unsafe { VirtualFreeEx(self.process, mem, 0, MEM_RELEASE) };
        }
        let kb = i32::from_le_bytes(r[..4].try_into().unwrap());
        let ms = i32::from_le_bytes(r[4..].try_into().unwrap());
        if !read || waited != 0 || kb != 0 || ms != 0 {
            return Err(format!("input fix failed (keyboard {kb:#010x}, mouse {ms:#010x})"));
        }
        for p in [&off::MOUSE_SHARED, &off::KEYBOARD_SHARED] {
            self.patch(p)?;
        }
        Ok(())
    }

    pub fn area(&self) -> Option<u32> {
        self.u32(off::AREA)
    }

    /// The simulation is stopped (pause menu, front end, loading).
    pub fn paused(&self) -> Option<bool> {
        self.f32(off::TIME_STEP).map(|s| s <= 0.0)
    }

    /// There is a player ped: not on the title screen or while a save is loading.
    pub fn has_player(&self) -> bool {
        self.u32(off::PLAYER).is_some_and(|p| p != 0)
    }

    pub fn fps_cap(&self) -> Option<u32> {
        self.u32(off::FPS_CAP)
    }

    pub fn set_fps_cap(&self, fps: u32) -> bool {
        self.write_bytes(off::FPS_CAP, &fps.to_le_bytes())
    }

    pub fn resolution(&self) -> Option<(u32, u32)> {
        Some((self.u32(off::RES_W)?, self.u32(off::RES_H)?)).filter(|&(w, h)| (320..=16384).contains(&w) && (200..=16384).contains(&h))
    }

    /// Starts the game suspended, patches it, then lets it run. With `opts.host` (and
    /// bullyre_host.exe in the game folder) the host exe is started instead: it maps the
    /// game into itself, then waits on an event while the patches go in (bullyre/src/host,
    /// HandOverToLauncher); bullyre is built into it, so no dll is loaded.
    pub fn launch(opts: &LaunchOptions) -> Result<Game, String> {
        if opts.x64 {
            return Game::launch_x64(opts.seamless);
        }
        let game_dir = game_dir();
        if let Some(e) = check_game_dir(&game_dir) {
            return Err(e);
        }
        let dir = game_dir.as_path();
        let exe = dir.join(GAME_EXE);
        ensure_exe(&exe)?;
        large_address_aware(&exe)?;
        set_video_settings(opts)?;
        let host = opts.host && dir.join(HOST_EXE).exists();
        let started = if host { dir.join(HOST_EXE) } else { exe.clone() };

        // Run the game DPI-aware so Windows doesn't bitmap-stretch it (sharp picture,
        // and window pixels = render pixels for the ESP overlay).
        unsafe { std::env::set_var("__COMPAT_LAYER", "HighDpiAware") };
        let exe_w = wide(&started.to_string_lossy());
        let dir_w = wide(&dir.to_string_lossy());
        let mut si: STARTUPINFOW = unsafe { std::mem::zeroed() };
        si.cb = std::mem::size_of::<STARTUPINFOW>() as u32;
        let mut pi: PROCESS_INFORMATION = unsafe { std::mem::zeroed() };
        let ok = unsafe { CreateProcessW(exe_w.as_ptr(), null_mut(), null_mut(), null_mut(), 0, CREATE_SUSPENDED, null_mut(), dir_w.as_ptr(), &mut si, &mut pi) };
        unsafe { std::env::remove_var("__COMPAT_LAYER") };
        if ok == 0 {
            let e = unsafe { GetLastError() };
            let hint = match e {
                14001 => " (missing VC++ 2005 runtime: run VCRedist\\vcredist_x86.exe in the game folder)",
                _ => "",
            };
            return Err(format!("couldn't start {}: error {e}{hint}", started.display()));
        }
        let game = Game { process: pi.hProcess, pid: pi.dwProcessId, launched: true, x64: false };
        // the host: let it map the game, then patch while it waits on `go`
        let mut go = null_mut();
        if host {
            let pid = pi.dwProcessId;
            let event = |what: &str| unsafe { CreateEventW(null_mut(), 1, 0, wide(&format!(r"Local\bullyre_host_{what}_{pid}")).as_ptr()) };
            let mapped = event("mapped");
            go = event("go");
            unsafe {
                ResumeThread(pi.hThread);
                CloseHandle(pi.hThread);
            }
            let waits = [mapped, pi.hProcess];
            let r = if mapped.is_null() || go.is_null() { u32::MAX } else { unsafe { WaitForMultipleObjects(2, waits.as_ptr(), 0, 60_000) } };
            if !mapped.is_null() {
                unsafe { CloseHandle(mapped) };
            }
            if r != WAIT_OBJECT_0 {
                if !go.is_null() {
                    unsafe { CloseHandle(go) };
                }
                return Err(match r {
                    1 => format!("{HOST_EXE} stopped before running the game (its message box says why)"),
                    u32::MAX => "couldn't make the events to hand over to bullyre's host exe".into(),
                    _ => format!("{HOST_EXE} didn't map the game within a minute"),
                });
            }
        }
        let mut patches = Vec::new();
        if opts.windowed {
            patches.extend([&off::WINDOWED_CTOR, &off::WINDOWED_MAIN, &off::ANY_RESOLUTION]);
        }
        if opts.free_mouse {
            patches.extend([&off::MOUSE_SHARED, &off::KEYBOARD_SHARED]);
        }
        let mut errs: Vec<String> = patches.into_iter().filter_map(|p| game.patch(p).err()).collect();
        errs.extend(game.runtime_patches(opts.windowed, opts.background));
        // the reversed game code (bullyre), when it's there (the host has it built in)
        if !host
            && dir.join("bullyre.dll").exists()
            && let Err(e) = game.load_at_start("bullyre.dll")
        {
            errs.push(e);
        }
        unsafe {
            if host {
                SetEvent(go);
                CloseHandle(go);
            } else {
                ResumeThread(pi.hThread);
                CloseHandle(pi.hThread);
            }
        }
        if !errs.is_empty() {
            return Err(format!("started, but: {}", errs.join("; ")));
        }
        Ok(game)
    }
}

impl Game {
    /// The 64-bit remaster on the picked folder: bullyre_x64.exe (beside the menu) with
    /// `--play --game-dir <folder>`, started in that folder. It needs none of the 32-bit game's
    /// patches (it's windowed, keeps running unfocused and shares the mouse itself) and loads its
    /// renderer, bullygfx64.dll, from beside itself; the display settings are the game's own.
    pub fn launch_x64(seamless: bool) -> Result<Game, String> {
        let dir = game_dir();
        if let Some(e) = check_game_dir(&dir) {
            return Err(e);
        }
        ensure_exe(&dir.join(GAME_EXE))?;
        // (the menu's own copy, unpacked; else one beside the menu)
        let bin = if crate::payload::available() { crate::payload::unpack()? } else { crate::beside_exe(X64_EXE).with_file_name("") };
        let ours = bin.join(X64_EXE);
        if !ours.exists() {
            return Err(format!("{X64_EXE} isn't there ({})", ours.display()));
        }
        let dir_s = dir.to_string_lossy().trim_end_matches('\\').to_string();
        let doors = if seamless { "" } else { " --no-seamless-doors" };
        let mut cmd = wide(&format!("\"{}\" --play --game-dir \"{}\"{}", ours.display(), dir_s, doors));
        let exe_w = wide(&ours.to_string_lossy());
        let dir_w = wide(&dir_s);
        let mut si: STARTUPINFOW = unsafe { std::mem::zeroed() };
        si.cb = std::mem::size_of::<STARTUPINFOW>() as u32;
        let mut pi: PROCESS_INFORMATION = unsafe { std::mem::zeroed() };
        let ok = unsafe { CreateProcessW(exe_w.as_ptr(), cmd.as_mut_ptr(), null_mut(), null_mut(), 0, 0, null_mut(), dir_w.as_ptr(), &mut si, &mut pi) };
        if ok == 0 {
            let e = unsafe { GetLastError() };
            return Err(format!("couldn't start {}: error {e}", ours.display()));
        }
        unsafe { CloseHandle(pi.hThread) };
        let game = Game { process: pi.hProcess, pid: pi.dwProcessId, launched: true, x64: true };
        if !bin.join(X64_GFX).exists() {
            return Err(format!("started without the new renderer: {X64_GFX} isn't beside {X64_EXE}"));
        }
        Ok(game)
    }
}

/// The routine `free_input` runs in the game (32-bit thread proc). It stores the
/// keyboard and mouse HRESULTs at base + 0x100.
fn free_input_code(base: u32) -> Vec<u8> {
    let res = base + 0x100;
    let mut c: Vec<u8> = vec![0x60]; // pushad
    let mut calls = Vec::new();
    for (i, dev) in [off::INPUT_KEYBOARD, off::INPUT_MOUSE].into_iter().enumerate() {
        c.push(0xBE); // mov esi, &device
        c.extend(dev.to_le_bytes());
        calls.push(c.len());
        c.extend([0xE8, 0, 0, 0, 0]); // call fix
        c.push(0xA3); // mov [res + i*4], eax
        c.extend((res + 4 * i as u32).to_le_bytes());
    }
    c.extend([0x61, 0x31, 0xC0, 0xC2, 0x04, 0x00]); // popad; xor eax,eax; ret 4
    let fix = c.len();
    c.extend([0xBF, 50, 0, 0, 0]); // mov edi, 50 (tries)
    let top = c.len();
    c.extend([0x8B, 0x1E, 0x85, 0xDB]); // mov ebx,[esi]; test ebx,ebx
    let jz_none = c.len();
    c.extend([0x74, 0]);
    c.extend([0x8B, 0x0B, 0x53, 0xFF, 0x51, 0x20]); // Unacquire(dev)
    c.extend([0x8B, 0x0B, 0x6A, 0x06, 0xFF, 0x35]); // SetCooperativeLevel(dev, [hwnd], 6)
    c.extend(off::GAME_HWND.to_le_bytes());
    c.extend([0x53, 0xFF, 0x51, 0x34]);
    c.extend([0x85, 0xC0]); // test eax,eax
    let jz_ok = c.len();
    c.extend([0x74, 0]);
    c.push(0x4F); // dec edi
    let back = top as isize - (c.len() + 2) as isize;
    c.extend([0x75, back as i8 as u8]); // jnz top
    c.push(0xC3); // ret: eax = last HRESULT
    let ok = c.len();
    c.extend([0x50, 0x8B, 0x0B, 0x53, 0xFF, 0x51, 0x1C, 0x58, 0xC3]); // Acquire(dev), return S_OK
    let none = c.len();
    c.extend([0x83, 0xC8, 0xFF, 0xC3]); // no device: -1
    c[jz_none + 1] = (none - (jz_none + 2)) as u8;
    c[jz_ok + 1] = (ok - (jz_ok + 2)) as u8;
    for at in calls {
        c[at + 1..at + 5].copy_from_slice(&((fix as i32) - (at as i32 + 5)).to_le_bytes());
    }
    c
}

/// Marks the exe large-address-aware if it isn't (older bullydumper output wasn't):
/// 4 GB of address space instead of 2, which the replacement renderer needs next to
/// the D3D9 driver.
fn large_address_aware(exe: &Path) -> Result<(), String> {
    let mut data = std::fs::read(exe).map_err(|e| format!("{}: {e}", exe.display()))?;
    let pe = data.get(0x3c..0x40).map(|b| u32::from_le_bytes(b.try_into().unwrap()) as usize).ok_or("exe too small")?;
    if data.get(pe..pe + 4) != Some(b"PE\0\0") {
        return Err(format!("{}: not a PE file", exe.display()));
    }
    let at = pe + 22; // COFF header Characteristics
    let c = u16::from_le_bytes([data[at], data[at + 1]]);
    if c & 0x20 != 0 {
        return Ok(());
    }
    data[at..at + 2].copy_from_slice(&(c | 0x20).to_le_bytes());
    std::fs::write(exe, &data).map_err(|e| format!("{}: {e}", exe.display()))
}

/// Puts Bully_unpacked.exe in the game folder if it isn't there yet: bullydumper's output
/// when one is found beside the menu, else the copy bundled into the menu.
fn ensure_exe(exe: &Path) -> Result<(), String> {
    if exe.exists() {
        return Ok(());
    }
    let mut starts: Vec<PathBuf> = Vec::new();
    if let Ok(p) = std::env::current_exe() {
        starts.push(p);
    }
    if let Ok(p) = std::env::current_dir() {
        starts.push(p);
    }
    for s in starts {
        for dir in s.ancestors() {
            let cand = dir.join("bullydumper").join("Bully.unpacked.exe");
            if cand.exists() {
                let ok = unsafe { CopyFileW(wide(&cand.to_string_lossy()).as_ptr(), wide(&exe.to_string_lossy()).as_ptr(), 1) };
                return if ok != 0 { Ok(()) } else { Err(format!("couldn't copy {} to {}", cand.display(), exe.display())) };
            }
        }
    }
    // (the copy bundled into the menu, when it carries one)
    if let Some(data) = crate::payload::game_exe() {
        return std::fs::write(exe, &data).map_err(|e| format!("couldn't write {}: {e}", exe.display()));
    }
    Err(format!("{} not found: run bullydumper to make Bully.unpacked.exe and copy it there as {GAME_EXE}", exe.display()))
}

fn reg_dword(key: HKEY, name: &str, v: u32) -> bool {
    let n = wide(name);
    unsafe { RegSetValueExW(key, n.as_ptr(), 0, REG_DWORD, v.to_le_bytes().as_ptr(), 4) == 0 }
}

pub fn reg_read(name: &str) -> Option<u32> {
    unsafe {
        let mut key: HKEY = null_mut();
        if RegCreateKeyExW(HKEY_CURRENT_USER, wide(off::REG_KEY).as_ptr(), 0, null_mut(), 0, KEY_ALL_ACCESS, null_mut(), &mut key, null_mut()) != 0 {
            return None;
        }
        let (mut ty, mut v, mut n) = (0u32, 0u32, 4u32);
        let r = RegQueryValueExW(key, wide(name).as_ptr(), null_mut(), &mut ty, &mut v as *mut u32 as *mut u8, &mut n);
        RegCloseKey(key);
        (r == 0 && ty == REG_DWORD).then_some(v)
    }
}

/// The game's own video settings: WIN (windowed) and the resolution.
fn set_video_settings(o: &LaunchOptions) -> Result<(), String> {
    unsafe {
        let mut key: HKEY = null_mut();
        if RegCreateKeyExW(HKEY_CURRENT_USER, wide(off::REG_KEY).as_ptr(), 0, null_mut(), 0, KEY_ALL_ACCESS, null_mut(), &mut key, null_mut()) != 0 {
            return Err("couldn't open the game's registry settings".into());
        }
        let ok = reg_dword(key, "WIN", o.windowed as u32) && (!o.windowed || (reg_dword(key, "RESW", o.width) && reg_dword(key, "RESH", o.height)));
        RegCloseKey(key);
        if ok { Ok(()) } else { Err("couldn't write the game's registry settings".into()) }
    }
}

/// Gives the game window a normal title bar and a client area of w x h.
/// Centers it on the desktop the first time.
pub fn frame_window(h: HWND, w: u32, hh: u32, center: bool) {
    // resizable: dragging an edge changes the game's resolution (see Game::request_resolution)
    const STYLE: u32 = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_VISIBLE | WS_CLIPSIBLINGS;
    unsafe {
        let dpi = GetDpiForWindow(h).max(96);
        let mut r = RECT { left: 0, top: 0, right: w as i32, bottom: hh as i32 };
        AdjustWindowRectExForDpi(&mut r, STYLE, 0, 0, dpi);
        let (fw, fh) = (r.right - r.left, r.bottom - r.top);
        SetWindowLongW(h, GWL_STYLE, STYLE as i32);
        let mut flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE;
        let (mut x, mut y) = (0, 0);
        if center {
            let mut work = RECT::default();
            SystemParametersInfoW(SPI_GETWORKAREA, 0, &mut work as *mut RECT as *mut c_void, 0);
            x = work.left + ((work.right - work.left) - fw).max(0) / 2;
            y = work.top + ((work.bottom - work.top) - fh).max(0) / 2;
        } else {
            flags |= SWP_NOMOVE;
        }
        SetWindowPos(h, null_mut(), x, y, fw, fh, flags);
    }
}

/// True when the window has a title bar and a client area of w x h.
/// Whether the window has our frame (title bar and resizable edges).
pub fn is_styled(h: HWND) -> bool {
    let style = unsafe { GetWindowLongW(h, GWL_STYLE) } as u32;
    style & WS_CAPTION == WS_CAPTION && style & WS_THICKFRAME != 0
}

pub fn client_size(h: HWND) -> (u32, u32) {
    let mut c = RECT::default();
    unsafe { GetClientRect(h, &mut c) };
    (c.right.max(0) as u32, c.bottom.max(0) as u32)
}

pub fn is_framed(h: HWND, w: u32, hh: u32) -> bool {
    let mut c = RECT::default();
    unsafe {
        GetClientRect(h, &mut c);
        GetWindowLongW(h, GWL_STYLE) as u32 & WS_CAPTION == WS_CAPTION && c.right == w as i32 && c.bottom == hh as i32
    }
}
