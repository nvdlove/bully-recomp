//! Addresses in Bully: Scholarship Edition (Steam, SteamStub removed with
//! bullydumper). The image has no relocations, so these are fixed in memory.
//! Each one was read out of the game's code; the comment says where.

/// A code patch: the bytes that must be there, and what replaces them.
pub struct Patch {
    pub name: &'static str,
    pub addr: u32,
    pub old: &'static [u8],
    pub new: &'static [u8],
}

// --- window / display -------------------------------------------------------

/// NiApplication ctor: `m_bFullscreen = (bitDepth != 0)` (`setne cl`) -> always windowed.
pub const WINDOWED_CTOR: Patch = Patch {
    name: "window: NiApplication ctor",
    addr: 0x0082_B422,
    old: &[0x0F, 0x95, 0xC1],
    new: &[0x32, 0xC9, 0x90],
};
/// BullyMain::vf34 forces `m_bFullscreen = 1` before the window is made -> 0.
pub const WINDOWED_MAIN: Patch = Patch {
    name: "window: BullyMain",
    addr: 0x0040_3206,
    old: &[0xC6, 0x46, 0x34, 0x01],
    new: &[0xC6, 0x46, 0x34, 0x00],
};
/// The start-up check of the saved resolution against the display modes (0x405AB0;
/// on failure the settings are reset to 800x600). Windowed, any size works, so it
/// always passes: `mov al, 1; ret`. (An ultrawide window on a 16:9 monitor.)
pub const ANY_RESOLUTION: Patch = Patch {
    name: "window: any resolution",
    addr: 0x0040_5AB0,
    old: &[0x80, 0x3D, 0x74, 0xBC, 0xBC, 0x00, 0x00],
    new: &[0xB0, 0x01, 0xC3, 0x90, 0x90, 0x90, 0x90],
};
/// The helper every resolution switch goes through (fills a DEVMODE, calls
/// ChangeDisplaySettingsA). Made to report success without touching the desktop.
pub const NO_DISPLAY_CHANGE: Patch = Patch {
    name: "never change the desktop resolution",
    addr: 0x0040_5580,
    old: &[0x81, 0xEC, 0x9C, 0x00, 0x00, 0x00],
    new: &[0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3],
};
/// Focus-loss handler: `ShowWindow(hwnd, SW_MINIMIZE)` -> nothing.
pub const NO_MINIMIZE: Patch = Patch {
    name: "don't minimize on focus loss",
    addr: 0x0040_5F46,
    old: &[0x6A, 0x06, 0x51, 0xFF, 0x15, 0x24, 0xC2, 0x8F, 0x00],
    new: &[0x90; 9],
};
/// Main loop asks "is the app active?" (byte 0xBCBC39) before it updates or
/// renders a frame -> always yes.
pub const RUN_UNFOCUSED: Patch = Patch {
    name: "keep running when unfocused",
    addr: 0x0040_5740,
    old: &[0xA0, 0x39, 0xBC, 0xBC, 0x00, 0xC3],
    new: &[0xB0, 0x01, 0xC3, 0x90, 0x90, 0x90],
};
/// The loading screen's message pump (0x4056D0) reads that byte itself and spins
/// until the window is active: a game started while you're in another window sat
/// at the loading screen forever. `je 0x4056E5` -> fall through.
pub const LOADING_UNFOCUSED: Patch = Patch {
    name: "keep loading when unfocused",
    addr: 0x0040_5720,
    old: &[0x74, 0xC3],
    new: &[0x90, 0x90],
};
/// Focus-loss handler sets the "paused by focus loss" byte (0xC1A998) -> no.
pub const NO_PAUSE_UNFOCUSED: Patch = Patch {
    name: "don't pause on focus loss",
    addr: 0x0040_5F15,
    old: &[0x88, 0x1D, 0x98, 0xA9, 0xC1, 0x00],
    new: &[0x90; 6],
};
pub const PAUSED_BY_FOCUS: u32 = 0x00C1_A998;
/// Focus-loss handler (0x405EEE): takes the streaming mutex (WaitForSingleObject on
/// [0x20DCDAC]) so the CdStream reader stops until focus comes back. The game used to
/// be minimized and paused by then; with it kept running, the next streaming wait
/// (loading, area changes, teleports) waits on a reader that can't run, and the
/// handler that would give the mutex back runs on the same stuck thread: the game
/// freezes for good. Clicking over to this menu while the game was loading did exactly
/// that. -> `mov bl, 1; jmp 0x405F09` (skip taking it; bl is used after).
pub const NO_STREAM_LOCK_UNFOCUSED: Patch = Patch {
    name: "keep streaming when unfocused",
    addr: 0x0040_5EEE,
    old: &[0x8B, 0x15, 0xAC, 0xCD],
    new: &[0xB3, 0x01, 0xEB, 0x17],
};
/// Focus-gain handler (0x405DB0): after ShowWindow(SW_RESTORE) it calls
/// MoveWindow(hwnd, 0, 0, resW, resH, FALSE), which throws a windowed game into the
/// top-left corner (and shrinks it by its frame) whenever it gets focus back -> skip
/// the call and pop its 6 arguments.
pub const NO_MOVE_ON_FOCUS: Patch = Patch {
    name: "don't move the window on focus",
    addr: 0x0040_5E52,
    old: &[0xFF, 0x15, 0x20, 0xC2, 0x8F, 0x00],
    new: &[0x83, 0xC4, 0x18, 0x90, 0x90, 0x90],
};
/// Resolution change (0x405F60): SetWindowPos(hwnd, 0, 0, 0, resW, resH, 0) -> SWP_NOMOVE.
pub const NO_MOVE_ON_RESIZE: Patch = Patch {
    name: "don't move the window on a resolution change",
    addr: 0x0040_5FA4,
    old: &[0x6A, 0x00],
    new: &[0x6A, 0x02],
};

/// DirectInput mouse (created from GUID_SysMouse): SetCooperativeLevel(hwnd,
/// DISCL_EXCLUSIVE | DISCL_FOREGROUND) -> DISCL_NONEXCLUSIVE | DISCL_FOREGROUND.
/// Exclusive mode hides and traps the cursor whenever the game has focus, even in
/// its pause menu. Only takes effect if applied before the device is set up.
pub const MOUSE_SHARED: Patch = Patch {
    name: "don't trap the mouse",
    addr: 0x0050_BCA2,
    old: &[0x6A, 0x05],
    new: &[0x6A, 0x06],
};
/// Same for the keyboard: exclusive mode is also what disables the Windows key.
pub const KEYBOARD_SHARED: Patch = Patch {
    name: "don't take the keyboard exclusively",
    addr: 0x0050_BC62,
    old: &[0x6A, 0x05],
    new: &[0x6A, 0x06],
};
/// The input manager passed as `this` to the device set-up above: its keyboard and
/// mouse IDirectInputDevice8 pointers, and the window they're bound to.
pub const INPUT_KEYBOARD: u32 = 0x020C_ECF4;
pub const INPUT_MOUSE: u32 = 0x020C_ECF8;
pub const GAME_HWND: u32 = 0x00BD_77FC;

/// Resolution the game renders at (DEVMODE it builds; written from RESW/RESH).
pub const RES_W: u32 = 0x00BC_BC60;
pub const RES_H: u32 = 0x00BC_BC64;
/// Registry key the game keeps its video settings in (per adapter: "A0").
pub const REG_KEY: &str = r"Software\Rockstar Games\Bully Scholarship Edition\A0";

// --- frame rate -------------------------------------------------------------

/// Frame limiter (0x406140): frame time = perf_frequency / this; 0 or less -> 31.
/// The game itself only writes it for the music class minigame: 35 when it starts
/// (0x6FB28D), -1 when it ends (0x6FACBF, 0x6FC7FA).
pub const FPS_CAP: u32 = 0x00BC_BC48;
pub const MUSIC_CLASS_FPS: u32 = 35;

/// CTimer::Update (0x45B960): time step = real ms * time scale / 20 (1.0 = one 50 FPS
/// frame), then `if step < 0.5 && !paused && scale >= 1: step = 0.5`. That floor is one
/// 100 FPS frame, so above 100 FPS the world ran faster than real time. The `jp` that
/// skips the floor becomes a `jmp`: the step always follows real time.
pub const NO_MIN_TIMESTEP: Patch = Patch {
    name: "real-time step above 100 FPS",
    addr: 0x0045_BB88,
    old: &[0x7A, 0x35],
    new: &[0xEB, 0x35],
};
/// CdStreamSync (0x73A690), waiting for a streaming read: `if (sectorsToRead) {
/// locked = 1; WaitForSingleObject(semaphore, INFINITE); }`. The reader thread does
/// `sectorsToRead = 0; if (locked) ReleaseSemaphore(semaphore);` (0x73A85D). If the
/// read finishes between the check and `locked = 1`, the wake-up is lost and the game
/// freezes for good, mostly while loading: start-up, area changes, teleports. Now it
/// re-checks in a loop and never waits more than 1 ms at a time (same 26 bytes):
///   loop: cmp [esi+4], 0 / je done / mov [esi+0x1C], 1 / push 1 / push [esi+0x14]
///         call [WaitForSingleObject] / jmp loop
pub const CDSTREAM_SYNC: Patch = Patch {
    name: "streaming wait that can't miss its wake-up",
    addr: 0x0073_A6B6,
    old: &[
        0x8B, 0x56, 0x04, 0x85, 0xD2, 0x74, 0x13, 0xC7, 0x46, 0x1C, 0x01, 0x00, 0x00, 0x00, 0x8B, 0x46, 0x14, 0x6A, 0xFF, 0x50, 0xFF, 0x15, 0x48, 0xC0,
        0x8F, 0x00,
    ],
    new: &[
        0x83, 0x7E, 0x04, 0x00, 0x74, 0x14, 0xC7, 0x46, 0x1C, 0x01, 0x00, 0x00, 0x00, 0x6A, 0x01, 0xFF, 0x76, 0x14, 0xFF, 0x15, 0x48, 0xC0, 0x8F, 0x00,
        0xEB, 0xE6,
    ],
};

/// CTimer: frames simulated so far, and this frame's time step (1.0 = 1/50 s).
pub const FRAME_COUNT: u32 = 0x00C1_A99C;
pub const TIME_STEP: u32 = 0x00C1_A9A0;

// --- lighting ---------------------------------------------------------------

/// The active timecycle: 24 hours x 21 weathers of 0x7C-byte keyframes, indexed
/// `(hour * 21 + weather) * 0x7C` by the timecycle update (0x6B0A4D), which blends
/// them every frame into the current lighting at 0x20C47A0.
pub const TIMECYCLE: u32 = 0x00BA_6C70;
pub const TC_RECORDS: usize = 24 * 21;
pub const TC_SIZE: usize = 0x7C;
// keyframe layout
pub const TC_AMBIENT: usize = 0x00; // props, world, peds: 3 x RGB bytes
pub const TC_SUN: usize = 0x09; // RGB
pub const TC_BACK: usize = 0x0C; // RGB, the fill light opposite the sun
pub const TC_SKY_TOP: usize = 0x2C; // RGB
pub const TC_SKY_BOTTOM: usize = 0x2F; // RGB
pub const TC_FAR_CLIP: usize = 0x48; // i16
pub const TC_FOG_START: usize = 0x4A; // i16
pub const TC_FILTER_MODE: usize = 0x5A; // 0 off, 1 modulate, 2 add
pub const TC_FILTER_RGBA: usize = 0x5B; // stored B, G, R, A
pub const TC_RANGES: usize = 0x60; // 6 floats: ambient lo/hi, sun lo/hi, back lo/hi
pub const TC_NEAR_FAR: usize = 0x78; // i32, max far/near clip ratio

// --- world ------------------------------------------------------------------

/// The area the player is in (AreaGetVisible). Ids index the area-name table at
/// 0x903058: 0 MainMap, ..., 14 Boys_Dorm, ..., 35 Girls_Dorm.
pub const AREA: u32 = 0x00BD_1008;
pub const AREA_BOYS_DORM: u32 = 14;

pub const PLAYER: u32 = 0x00C1_AEA8; // CPed* of the player
pub const PED_POOL: u32 = 0x00C0_F5F0; // pool of CPed (24 x 0x1F44)
pub const VEHICLE_POOL: u32 = 0x00C0_F5F4; // pool of CVehicle (15 x 0x8C0)
// pool layout: +0 objects, +4 byte map (0x80 = free), +8 capacity, +0xC stride

pub const ENT_MATRIX: u32 = 0x14; // CPlaceable: matrix pointer, position (at the feet) at +0x30
pub const ENT_POS: u32 = 0x04; // position used when there is no matrix
pub const ENT_MODEL: u32 = 0x10E; // int16 model id
pub const PED_DEAD: u32 = 0x1C98;
pub const PED_MAX_HEALTH: u32 = 0x1CA4;
pub const PED_HEALTH: u32 = 0x1CB8;
pub const PED_VEHICLE: u32 = 0x1554;
pub const VEH_HEALTH: u32 = 0x340;

// --- camera -----------------------------------------------------------------

/// TheCamera (used by CameraIsPointVisible); world -> view matrix at +4:
/// view = M * p + t, with y = depth, z = up, x = left.
pub const CAMERA_VIEW: u32 = 0x00C3_CC6C;
/// TheCamera+0x8C: its RwCamera. The view window (tan of the half-angles, x
/// then y) is at +0x198; near/far clip at +0x1B8/+0x1B0 (RwCamera getters).
pub const RW_CAMERA: u32 = 0x00C3_CCF4;
pub const VIEW_WINDOW: u32 = 0x198;
/// CameraGetFOV: FOV in degrees, or the override below when it is active.
pub const FOV: u32 = 0x00C3_CD04;
pub const FOV_OVERRIDE: u32 = 0x00C3_CDD0;
pub const FOV_OVERRIDE_ON: u32 = 0x00C3_CDD4;
