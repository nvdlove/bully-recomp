//! A frame of the game's Direct3D 9 rendering, recorded for the replay renderer: every
//! draw with the complete device state it ran with, every clear, and every surface
//! copy, in order.
//!
//! The state is read back from the real device at each call (GetRenderState,
//! GetTexture, Get*ShaderConstantF, ...) rather than mirrored from the Set* calls:
//! D3DX effects save and restore state through state blocks, which change the device
//! without any Set* call we'd see. The game creates a pure device, which can't answer
//! Get*, so `hook::create_device` drops that flag.

use std::ffi::c_void;
use std::sync::{Arc, Mutex};

pub const DEV_PRESENT: u32 = 17;
pub const DEV_STRETCH_RECT: u32 = 34;
pub const DEV_COLOR_FILL: u32 = 35;
pub const DEV_CLEAR: u32 = 43;
pub const DEV_DRAW: u32 = 81;
pub const DEV_DRAW_INDEXED: u32 = 82;
pub const DEV_DRAW_UP: u32 = 83;
pub const DEV_DRAW_INDEXED_UP: u32 = 84;
pub const DEV_RESET: u32 = 16;

// IDirect3DDevice9 getters
const GET_RENDER_TARGET: usize = 38;
const GET_DEPTH_STENCIL: usize = 40;
const GET_VIEWPORT: usize = 48;
const GET_RENDER_STATE: usize = 58;
const GET_TEXTURE: usize = 64;
const GET_SAMPLER_STATE: usize = 68;
const GET_SCISSOR_RECT: usize = 76;
const GET_VERTEX_DECLARATION: usize = 88;
const GET_VERTEX_SHADER: usize = 93;
const GET_VS_CONSTANT_F: usize = 95;
const GET_STREAM_SOURCE: usize = 101;
const GET_INDICES: usize = 105;
const GET_PIXEL_SHADER: usize = 108;
const GET_PS_CONSTANT_F: usize = 110;

pub const VS_CONSTS: usize = crate::dxso::VS_CONSTANTS;
pub const PS_CONSTS: usize = crate::dxso::PS_CONSTANTS;
pub const SAMPLERS: usize = 16;
pub const RENDER_TARGETS: usize = 4;
/// Sampler state types kept (D3DSAMP_ADDRESSU = 1 .. D3DSAMP_SRGBTEXTURE = 11).
pub const SAMPLER_STATES: usize = 12;

/// The render states the replay uses, by D3DRENDERSTATETYPE.
pub const RENDER_STATES: &[u32] = &[
    7,   // ZENABLE
    8,   // FILLMODE
    14,  // ZWRITEENABLE
    15,  // ALPHATESTENABLE
    19,  // SRCBLEND
    20,  // DESTBLEND
    22,  // CULLMODE
    23,  // ZFUNC
    24,  // ALPHAREF
    25,  // ALPHAFUNC
    27,  // ALPHABLENDENABLE
    52,  // STENCILENABLE
    53,  // STENCILFAIL
    54,  // STENCILZFAIL
    55,  // STENCILPASS
    56,  // STENCILFUNC
    57,  // STENCILREF
    58,  // STENCILMASK
    59,  // STENCILWRITEMASK
    168, // COLORWRITEENABLE
    171, // BLENDOP
    174, // SCISSORTESTENABLE
    175, // SLOPESCALEDEPTHBIAS
    193, // BLENDFACTOR
    194, // SRGBWRITEENABLE
    195, // DEPTHBIAS
    196, // (unused by D3D9: bullyre_x64's seamless doors set PORTAL_DRAWS here, portal.rs)
    206, // SEPARATEALPHABLENDENABLE
    207, // SRCBLENDALPHA
    208, // DESTBLENDALPHA
    209, // BLENDOPALPHA
];

/// A render state's value in a draw, by D3DRENDERSTATETYPE.
pub fn rs(states: &[u32], ty: u32) -> u32 {
    RENDER_STATES.iter().position(|&t| t == ty).map_or(0, |i| states[i])
}

#[derive(Clone, Copy, Debug, Default)]
pub struct Stream {
    pub vb: u32,
    pub offset: u32,
    pub stride: u32,
}

pub struct Draw {
    pub indexed: bool,
    pub prim: u32,
    pub base_vertex: i32,
    pub min_index: u32,
    pub num_vertices: u32,
    /// Start index (indexed) or start vertex.
    pub start: u32,
    pub prims: u32,
    pub vs: u32,
    pub ps: u32,
    pub decl: u32,
    pub streams: [Stream; 4],
    pub ib: u32,
    pub vs_consts: Arc<[[f32; 4]; VS_CONSTS]>,
    pub ps_consts: Arc<[[f32; 4]; PS_CONSTS]>,
    pub textures: [u32; SAMPLERS],
    pub samplers: [[u32; SAMPLER_STATES]; SAMPLERS],
    pub states: Vec<u32>,
    pub viewport: Viewport,
    pub scissor: [i32; 4],
    pub rt: [u32; RENDER_TARGETS],
    pub ds: u32,
    /// Vertex data drawn from user memory (DrawPrimitiveUP), stream 0.
    pub up_vertices: Option<Arc<Vec<u8>>>,
    pub up_indices: Option<(Arc<Vec<u8>>, bool)>, // bytes, 32-bit
    /// Dynamic vertex buffers' data as it was at the draw (the game rewrites them
    /// within the frame), per stream; the stream's offset is then 0.
    pub vb_data: [Option<Arc<Vec<u8>>>; 4],
}

#[derive(Clone, Copy, Debug, Default)]
pub struct Viewport {
    pub x: u32,
    pub y: u32,
    pub w: u32,
    pub h: u32,
    pub min_z: f32,
    pub max_z: f32,
}

/// The camera the game set (Gamebryo SetCameraData): location, direction, up, right,
/// and the frustum (left, right, top, bottom as tangents, near, far).
#[derive(Clone, Copy, Debug, Default)]
pub struct Camera {
    pub eye: [f32; 3],
    pub dir: [f32; 3],
    pub up: [f32; 3],
    pub right: [f32; 3],
    pub frustum: [f32; 6],
}

/// SetCameraData is about to run (its arguments: pointers to the vectors and frustum).
pub fn camera(eye: u32, dir: u32, up: u32, right: u32, frustum: u32) {
    if !ENABLED.load(std::sync::atomic::Ordering::Relaxed) || [eye, dir, up, right, frustum].contains(&0) {
        return;
    }
    // the game's own arguments, valid for the call
    let v3 = |p: u32| unsafe { *(p as usize as *const [f32; 3]) };
    let f = unsafe { *(frustum as usize as *const [f32; 6]) };
    push(Cmd::Camera(Camera { eye: v3(eye), dir: v3(dir), up: v3(up), right: v3(right), frustum: f }));
}

/// Render state 196 in the draws of another area seen through a doorway (portal.rs), and of the
/// sky around a doorway the camera is through (PORTAL_AROUND).
pub const PORTAL_DRAWS: u32 = u32::from_le_bytes(*b"PRT1");
/// Render state 196 in the draws of the area the camera is still in, drawn around a doorway
/// the player has gone through (and in it, what's in front of it: portal.rs's PORTAL_AROUND).
pub const PORTAL_AROUND_DRAWS: u32 = u32::from_le_bytes(*b"PRT2");

/// Render state 196's value in a draw of the seamless doors' (PORTAL_DRAWS, PORTAL_AROUND_DRAWS).
pub fn is_portal_draw(value: u32) -> bool {
    value == PORTAL_DRAWS || value == PORTAL_AROUND_DRAWS
}

/// The depth surface a doorway would be marked in (none: nothing is recorded, or no depth surface).
fn portal_surface() -> Option<u32> {
    if !ENABLED.load(std::sync::atomic::Ordering::Relaxed) {
        return None;
    }
    SHADOW.lock().ok().and_then(|g| g.as_ref().filter(|s| s.valid).map(|s| s.ds)).filter(|&ds| ds != 0)
}

/// Whether a doorway would be marked now (portal's checks, nothing sent).
pub fn portal_ready() -> bool {
    portal_surface().is_some()
}

/// (bullyre_x64's seamless doors) a doorway the next PORTAL_DRAWS draws are seen through: its
/// corners in the world and the camera it's seen from (`mode` portal.rs's PORTAL_IN or
/// PORTAL_AROUND).
/// False when nothing is recorded (the replay renderer off).
pub fn portal(corners: [[f32; 3]; 4], cam: Camera, mode: u32) -> bool {
    let Some(ds) = portal_surface() else { return false };
    hold_once(ds);
    push(Cmd::Portal { corners, cam, ds, mode });
    true
}

pub enum Cmd {
    /// The camera from here on.
    Camera(Camera),
    /// A doorway marked in the depth surface's stencil (portal.rs; `mode` PORTAL_IN or
    /// PORTAL_AROUND).
    Portal { corners: [[f32; 3]; 4], cam: Camera, ds: u32, mode: u32 },
    Draw(Box<Draw>),
    Clear { rt: [u32; RENDER_TARGETS], ds: u32, flags: u32, color: u32, z: f32, stencil: u32, viewport: Viewport, rects: Vec<[i32; 4]> },
    Stretch { src: u32, src_rect: Option<[i32; 4]>, dst: u32, dst_rect: Option<[i32; 4]>, filter: u32 },
    Fill { surface: u32, rect: Option<[i32; 4]>, color: u32 },
}

struct Recording {
    cmds: Vec<Cmd>,
    /// What `held` has, to take each reference once.
    held_set: std::collections::BTreeSet<u32>,
    /// Buffers' usage and index size, looked up once a frame.
    buffer_descs: std::collections::BTreeMap<u32, (u32, bool)>,
    /// A reference on every object the commands name, so none is freed (and its memory
    /// reused) before the replay has run.
    held: Vec<u32>,
    /// Draws without both shaders (fixed function), which the replay can't do.
    fixed_function: u32,
}

static FRAME: Mutex<Recording> = Mutex::new(Recording { cmds: Vec::new(), held_set: std::collections::BTreeSet::new(), buffer_descs: std::collections::BTreeMap::new(), held: Vec::new(), fixed_function: 0 });

/// References on a frame's objects, given back when dropped.
pub struct Held(Vec<u32>);

impl Held {
    pub fn len(&self) -> usize {
        self.0.len()
    }
}

impl Drop for Held {
    fn drop(&mut self) {
        for &o in &self.0 {
            unsafe { release(o) };
        }
    }
}

/// Keeps a reference the caller already has (a Get*'s) until the replay's done.
fn keep(obj: u32) {
    if obj != 0 {
        match FRAME.lock() {
            Ok(mut f) => f.held.push(obj),
            Err(_) => unsafe { release(obj) },
        }
    }
}

/// Takes a reference on an object the game passed in.
fn hold(obj: u32) {
    if obj != 0 {
        type AddRef = unsafe extern "system" fn(usize) -> u32;
        unsafe { std::mem::transmute::<usize, AddRef>(method(obj, 1))(obj as usize) };
        keep(obj);
    }
}

/// Time spent recording draws (for the perf report).
pub static CAPTURE_NS: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(0);

/// Whether D3D9 still draws (off: only the replay does).
pub static EXCLUSIVE: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
/// Testing ("hideskinned on"): the skinned draws (peds) left out of the replay, so a shot
/// with them and one without give the characters' pixels (tests/charparity.py).
pub static HIDE_SKINNED: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
/// DrawIndexedPrimitive's call site inside the skinned draw (0x87E7D0), as scene.rs has it.
const SKINNED_DRAW_SITE: u32 = 0x0087_EA56;

/// Whether the draw just recorded goes to the screen (a surface that isn't a texture's):
/// only those does D3D9 leave out in exclusive mode. Draws into render textures still
/// happen in D3D9 too, as the game reads some of them back through D3DX (Jimmy's
/// clothes are baked with D3DXLoadSurfaceFromSurface, which goes to the real device).
pub static LAST_TO_SCREEN: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

/// Testing: off = D3D9 leaves out every draw in exclusive mode.
pub static SCREEN_ONLY: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(true);

/// Whether a surface is a level of a texture (cached for the last one asked).
fn is_texture_level(surface: u32) -> bool {
    static LAST: std::sync::Mutex<(u32, bool)> = std::sync::Mutex::new((0, false));
    if surface == 0 {
        return false;
    }
    if let Ok(l) = LAST.lock()
        && l.0 == surface
    {
        return l.1;
    }
    // IID_IDirect3DBaseTexture9 {580CA87E-1D3C-4d54-991D-B7D3E3C298CE}
    const IID: [u8; 16] = [0x7E, 0xA8, 0x0C, 0x58, 0x3C, 0x1D, 0x54, 0x4D, 0x99, 0x1D, 0xB7, 0xD3, 0xE3, 0xC2, 0x98, 0xCE];
    type GetContainer = unsafe extern "system" fn(usize, *const u8, *mut usize) -> i32;
    let mut out = 0usize;
    let hr = unsafe { std::mem::transmute::<usize, GetContainer>(method(surface, 11))(surface as usize, IID.as_ptr(), &mut out) };
    let out = crate::win::obj32(out);
    let level = hr >= 0 && out != 0;
    if level {
        unsafe { release(out) };
    }
    if let Ok(mut l) = LAST.lock() {
        *l = (surface, level);
    }
    level
}

/// Which argument of a draw call is its primitive count.
pub fn primitive_count_arg(method: u32) -> Option<usize> {
    match method {
        DEV_DRAW => Some(2),
        DEV_DRAW_INDEXED => Some(5),
        DEV_DRAW_UP => Some(1),
        DEV_DRAW_INDEXED_UP => Some(3),
        _ => None,
    }
}

/// Whether to record at all (the replay renderer is on).
pub static ENABLED: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

unsafe fn method(obj: u32, i: usize) -> usize {
    unsafe { *(*(obj as *const *const usize)).add(i) }
}

unsafe fn release(obj: u32) {
    if obj != 0 {
        type Release = unsafe extern "system" fn(usize) -> u32;
        unsafe { std::mem::transmute::<usize, Release>(method(obj, 2))(obj as usize) };
    }
}

unsafe fn viewport(device: u32) -> Viewport {
    let mut v = [0u32; 6];
    type F = unsafe extern "system" fn(usize, *mut u32) -> i32;
    unsafe { std::mem::transmute::<usize, F>(method(device, GET_VIEWPORT))(device as usize, v.as_mut_ptr()) };
    Viewport { x: v[0], y: v[1], w: v[2], h: v[3], min_z: f32::from_bits(v[4]), max_z: f32::from_bits(v[5]) }
}

fn rect_at(p: u32) -> Option<[i32; 4]> {
    if p == 0 {
        return None;
    }
    let mut r = [0u8; 16];
    crate::win::read_bytes(p, &mut r).then(|| [0, 4, 8, 12].map(|i| i32::from_le_bytes(r[i..i + 4].try_into().unwrap())))
}

fn push(cmd: Cmd) {
    if let Ok(mut f) = FRAME.lock() {
        f.cmds.push(cmd);
    }
}

/// The frame so far; the recording starts over.
pub fn take() -> (Vec<Cmd>, u32, Held) {
    match FRAME.lock() {
        Ok(mut f) => {
            f.held_set.clear();
            f.buffer_descs.clear();
            (std::mem::take(&mut f.cmds), std::mem::take(&mut f.fixed_function), Held(std::mem::take(&mut f.held)))
        }
        Err(_) => (Vec::new(), 0, Held(Vec::new())),
    }
}

/// A device call, before it runs. `device` is the real IDirect3DDevice9, `args` its
/// arguments after `this`, `ret` where it was called from.
pub fn device_call(device: u32, method_index: u32, args: &[u32], ret: u32) {
    if !ENABLED.load(std::sync::atomic::Ordering::Relaxed) || device == 0 {
        return;
    }
    unsafe {
        match method_index {
            DEV_CLEAR => {
                let Some((rt, ds, viewport)) = with_shadow(device, |s| (s.rt, s.ds, s.viewport.unwrap_or_default())) else { return };
                for o in rt.into_iter().chain([ds]) {
                    hold_once(o);
                }
                let rects = (0..args[0]).filter_map(|i| rect_at(args[1] + 16 * i)).collect();
                push(Cmd::Clear { rt, ds, flags: args[2], color: args[3], z: f32::from_bits(args[4]), stencil: args[5], viewport, rects });
            }
            DEV_RESET => SHADOW_STALE.store(true, std::sync::atomic::Ordering::SeqCst),
            DEV_BEGIN_STATE_BLOCK | DEV_END_STATE_BLOCK | DEV_SET_RENDER_TARGET | DEV_SET_DEPTH_STENCIL | DEV_SET_VIEWPORT | DEV_SET_RENDER_STATE | DEV_SET_TEXTURE
            | DEV_SET_SAMPLER_STATE | DEV_SET_SCISSOR_RECT | DEV_SET_VERTEX_DECLARATION | DEV_SET_FVF | DEV_SET_VERTEX_SHADER | DEV_SET_VS_CONSTANT_F
            | DEV_SET_STREAM_SOURCE | DEV_SET_INDICES | DEV_SET_PIXEL_SHADER | DEV_SET_PS_CONSTANT_F => {
                if let Ok(mut g) = SHADOW.lock()
                    && let Some(sh) = g.as_mut()
                {
                    sh.follow(method_index, args);
                }
            }
            DEV_STRETCH_RECT => {
                hold(args[0]);
                hold(args[2]);
                push(Cmd::Stretch { src: args[0], src_rect: rect_at(args[1]), dst: args[2], dst_rect: rect_at(args[3]), filter: args[4] })
            }
            DEV_COLOR_FILL => {
                hold(args[0]);
                push(Cmd::Fill { surface: args[0], rect: rect_at(args[1]), color: args[2] })
            }
            DEV_DRAW | DEV_DRAW_INDEXED | DEV_DRAW_UP | DEV_DRAW_INDEXED_UP => {
                if ret == SKINNED_DRAW_SITE && HIDE_SKINNED.load(std::sync::atomic::Ordering::Relaxed) {
                    return;
                }
                let t = std::time::Instant::now();
                record_draw(device, method_index, args);
                CAPTURE_NS.fetch_add(t.elapsed().as_nanos() as u64, std::sync::atomic::Ordering::Relaxed);
            }
            _ => {}
        }
    }
}

unsafe fn record_draw(device: u32, m: u32, args: &[u32]) {
    LAST_TO_SCREEN.store(false, std::sync::atomic::Ordering::Relaxed);
    // a count no real draw has: say what it was, and leave it out
    let prims = args[primitive_count_arg(m).unwrap_or(1)];
    if prims == 0 {
        return; // draws nothing (D3D9 refuses it)
    }
    if prims > 1 << 24 {
        static SAID: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
        if SAID.fetch_add(1, std::sync::atomic::Ordering::Relaxed) < 10 {
            crate::control::log(&format!("draw with {prims} primitives left out: method {m} args {args:x?}"));
        }
        return;
    }
    let up = matches!(m, DEV_DRAW_UP | DEV_DRAW_INDEXED_UP);
    let indexed = matches!(m, DEV_DRAW_INDEXED | DEV_DRAW_INDEXED_UP);
    let d = with_shadow(device, |sh| {
        if sh.vs == 0 || sh.ps == 0 {
            return None;
        }
        let d = Draw {
            indexed,
            prim: args[0],
            base_vertex: 0,
            min_index: 0,
            num_vertices: 0,
            start: 0,
            prims: 0,
            vs: sh.vs,
            ps: sh.ps,
            decl: sh.decl.unwrap_or(0),
            streams: if up { [Stream::default(); 4] } else { sh.streams },
            ib: if indexed && !up { sh.ib } else { 0 },
            vs_consts: sh.vs_consts.clone(),
            ps_consts: sh.ps_consts.clone(),
            textures: sh.textures,
            samplers: sh.samplers,
            states: RENDER_STATES.iter().map(|&t| sh.states[t as usize]).collect(),
            viewport: sh.viewport.unwrap_or_default(),
            scissor: sh.scissor.unwrap_or_default(),
            rt: sh.rt,
            ds: sh.ds,
            up_vertices: None,
            up_indices: None,
            vb_data: Default::default(),
        };
        // the UP draws leave stream 0 (and the indices) unset
        if up {
            sh.streams[0] = Stream::default();
            if indexed {
                sh.ib = 0;
            }
        }
        Some(d)
    })
    .flatten();
    let Some(mut d) = d else {
        if let Ok(mut f) = FRAME.lock() {
            f.fixed_function += 1;
        }
        return;
    };
    match m {
        DEV_DRAW => (d.start, d.prims) = (args[1], args[2]),
        DEV_DRAW_INDEXED => {
            d.base_vertex = args[1] as i32;
            d.min_index = args[2];
            d.num_vertices = args[3];
            d.start = args[4];
            d.prims = args[5];
        }
        DEV_DRAW_UP => {
            // (type, prims, data, stride): copy what it draws
            d.prims = args[1];
            let stride = args[3];
            let n = vertex_count(d.prim, d.prims);
            let mut bytes = vec![0u8; (n * stride) as usize];
            crate::win::read_bytes(args[2], &mut bytes);
            d.up_vertices = Some(Arc::new(bytes));
            d.streams[0] = Stream { vb: 0, offset: 0, stride };
        }
        DEV_DRAW_INDEXED_UP => {
            // (type, min index, vertex count, prims, indices, index format, data, stride)
            d.min_index = args[1];
            d.num_vertices = args[2];
            d.prims = args[3];
            let big = args[5] == 102; // D3DFMT_INDEX32
            let n = vertex_count(d.prim, d.prims) * if big { 4 } else { 2 };
            let mut idx = vec![0u8; n as usize];
            crate::win::read_bytes(args[4], &mut idx);
            d.up_indices = Some((Arc::new(idx), big));
            let stride = args[7];
            let mut bytes = vec![0u8; ((args[1] + args[2]) * stride) as usize];
            crate::win::read_bytes(args[6], &mut bytes);
            d.up_vertices = Some(Arc::new(bytes));
            d.streams[0] = Stream { vb: 0, offset: 0, stride };
        }
        _ => {}
    }
    if d.up_vertices.is_none() {
        unsafe { snapshot_dynamic(&mut d) };
    }
    hold_all([d.vs, d.ps, d.decl, d.ib, d.ds].into_iter().chain(d.streams.iter().map(|s| s.vb)).chain(d.textures).chain(d.rt));
    LAST_TO_SCREEN.store(!is_texture_level(d.rt[0]), std::sync::atomic::Ordering::Relaxed);
    push(Cmd::Draw(Box::new(d)));
}

// ---- the device state, followed from the Set* calls ---------------------------------
//
// Reading the whole state back with Get* for every draw costs ~100 calls a draw (each
// taking the multithreaded device's lock). Instead the state is followed from the Set*
// calls the game makes, and read back in full only when something changed it behind our
// back: a state block applied (IDirect3DStateBlock9::Apply is hooked), a device reset,
// the recording starting. SetRenderTarget resets the viewport and scissor rect, which
// are then read back.

const DEV_SET_RENDER_TARGET: u32 = 37;
const DEV_SET_DEPTH_STENCIL: u32 = 39;
const DEV_SET_VIEWPORT: u32 = 47;
const DEV_SET_RENDER_STATE: u32 = 57;
const DEV_CREATE_STATE_BLOCK: usize = 59;
const DEV_BEGIN_STATE_BLOCK: u32 = 60;
const DEV_END_STATE_BLOCK: u32 = 61;
const DEV_SET_TEXTURE: u32 = 65;
const DEV_SET_SAMPLER_STATE: u32 = 69;
const DEV_SET_SCISSOR_RECT: u32 = 75;
const DEV_SET_VERTEX_DECLARATION: u32 = 87;
const DEV_SET_FVF: u32 = 89;
const DEV_SET_VERTEX_SHADER: u32 = 92;
const DEV_SET_VS_CONSTANT_F: u32 = 94;
const DEV_SET_STREAM_SOURCE: u32 = 100;
const DEV_SET_INDICES: u32 = 104;
const DEV_SET_PIXEL_SHADER: u32 = 107;
const DEV_SET_PS_CONSTANT_F: u32 = 109;
const STATE_BLOCK_APPLY: usize = 5;

struct Shadow {
    valid: bool,
    /// Between BeginStateBlock and EndStateBlock Set* calls are recorded, not applied.
    recording_block: bool,
    vs: u32,
    ps: u32,
    decl: Option<u32>, // None: set through SetFVF, read back
    streams: [Stream; 4],
    ib: u32,
    vs_consts: Arc<[[f32; 4]; VS_CONSTS]>,
    ps_consts: Arc<[[f32; 4]; PS_CONSTS]>,
    textures: [u32; SAMPLERS],
    samplers: [[u32; SAMPLER_STATES]; SAMPLERS],
    states: [u32; 256],
    viewport: Option<Viewport>,
    scissor: Option<[i32; 4]>,
    rt: [u32; RENDER_TARGETS],
    ds: u32,
}

static SHADOW: Mutex<Option<Shadow>> = Mutex::new(None);

/// Set when the device state may have changed without a Set* call we saw.
pub static SHADOW_STALE: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(true);

static REAL_APPLY: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);

unsafe extern "system" fn state_block_apply(this: u32) -> i32 {
    SHADOW_STALE.store(true, std::sync::atomic::Ordering::SeqCst);
    type Apply = unsafe extern "system" fn(usize) -> i32;
    let real = REAL_APPLY.load(std::sync::atomic::Ordering::SeqCst);
    if real == 0 {
        return 0;
    }
    unsafe { std::mem::transmute::<usize, Apply>(real)(this as usize) }
}

/// Hooks Apply in the state blocks' vtable (found from a state block of our own).
unsafe fn hook_state_blocks(device: u32) {
    static DONE: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
    if DONE.swap(true, std::sync::atomic::Ordering::SeqCst) {
        return;
    }
    type Create = unsafe extern "system" fn(usize, u32, *mut usize) -> i32;
    let mut block = 0usize;
    const D3DSBT_PIXELSTATE: u32 = 2;
    if unsafe { std::mem::transmute::<usize, Create>(method(device, DEV_CREATE_STATE_BLOCK))(device as usize, D3DSBT_PIXELSTATE, &mut block) } < 0 || block == 0 {
        crate::control::log("state blocks: couldn't make one to hook");
        return;
    }
    let slot = unsafe { (*(block as *const *mut usize)).add(STATE_BLOCK_APPLY) };
    let mut old = 0;
    unsafe {
        let size = std::mem::size_of::<usize>();
        crate::win::VirtualProtect(slot as *mut c_void, size, crate::win::PAGE_EXECUTE_READWRITE, &mut old);
        if *slot != state_block_apply as *const () as usize {
            REAL_APPLY.store(*slot, std::sync::atomic::Ordering::SeqCst);
            *slot = state_block_apply as *const () as usize;
        }
        crate::win::VirtualProtect(slot as *mut c_void, size, old, &mut old);
        release(crate::win::obj32(block));
    }
}

/// An object's pointer from a Get* (its reference given back straight away).
unsafe fn peek(device: u32, slot: usize, arg: Option<u32>) -> u32 {
    let mut native = 0usize;
    unsafe {
        match arg {
            Some(a) => {
                type F = unsafe extern "system" fn(usize, u32, *mut usize) -> i32;
                std::mem::transmute::<usize, F>(method(device, slot))(device as usize, a, &mut native);
            }
            None => {
                type F = unsafe extern "system" fn(usize, *mut usize) -> i32;
                std::mem::transmute::<usize, F>(method(device, slot))(device as usize, &mut native);
            }
        }
    }
    let out = crate::win::obj32(native);
    unsafe { release(out) };
    out
}

unsafe fn scissor_rect(device: u32) -> [i32; 4] {
    let mut r = [0i32; 4];
    type Rect = unsafe extern "system" fn(usize, *mut i32) -> i32;
    unsafe { std::mem::transmute::<usize, Rect>(method(device, GET_SCISSOR_RECT))(device as usize, r.as_mut_ptr()) };
    r
}

impl Shadow {
    /// Everything, read back from the device.
    unsafe fn read(device: u32) -> Shadow {
        let mut s = Shadow {
            valid: true,
            recording_block: false,
            vs: 0,
            ps: 0,
            decl: None,
            streams: [Stream::default(); 4],
            ib: 0,
            vs_consts: Arc::new([[0.0; 4]; VS_CONSTS]),
            ps_consts: Arc::new([[0.0; 4]; PS_CONSTS]),
            textures: [0; SAMPLERS],
            samplers: [[0; SAMPLER_STATES]; SAMPLERS],
            states: [0; 256],
            viewport: None,
            scissor: None,
            rt: [0; RENDER_TARGETS],
            ds: 0,
        };
        unsafe {
            s.vs = peek(device, GET_VERTEX_SHADER, None);
            s.ps = peek(device, GET_PIXEL_SHADER, None);
            s.decl = Some(peek(device, GET_VERTEX_DECLARATION, None));
            for (i, st) in s.streams.iter_mut().enumerate() {
                let mut vb = 0usize;
                type F = unsafe extern "system" fn(usize, u32, *mut usize, *mut u32, *mut u32) -> i32;
                std::mem::transmute::<usize, F>(method(device, GET_STREAM_SOURCE))(device as usize, i as u32, &mut vb, &mut st.offset, &mut st.stride);
                let vb = crate::win::obj32(vb);
                release(vb);
                st.vb = vb;
            }
            s.ib = peek(device, GET_INDICES, None);
            type Consts = unsafe extern "system" fn(usize, u32, *mut f32, u32) -> i32;
            std::mem::transmute::<usize, Consts>(method(device, GET_VS_CONSTANT_F))(device as usize, 0, Arc::make_mut(&mut s.vs_consts).as_mut_ptr() as *mut f32, VS_CONSTS as u32);
            std::mem::transmute::<usize, Consts>(method(device, GET_PS_CONSTANT_F))(device as usize, 0, Arc::make_mut(&mut s.ps_consts).as_mut_ptr() as *mut f32, PS_CONSTS as u32);
            type State = unsafe extern "system" fn(usize, u32, *mut u32) -> i32;
            for &ty in RENDER_STATES {
                std::mem::transmute::<usize, State>(method(device, GET_RENDER_STATE))(device as usize, ty, &mut s.states[ty as usize]);
            }
            type Samp = unsafe extern "system" fn(usize, u32, u32, *mut u32) -> i32;
            for i in 0..SAMPLERS {
                s.textures[i] = peek(device, GET_TEXTURE, Some(i as u32));
                for t in 1..SAMPLER_STATES {
                    std::mem::transmute::<usize, Samp>(method(device, GET_SAMPLER_STATE))(device as usize, i as u32, t as u32, &mut s.samplers[i][t]);
                }
            }
            s.viewport = Some(viewport(device));
            s.scissor = Some(scissor_rect(device));
            for (i, t) in s.rt.iter_mut().enumerate() {
                *t = peek(device, GET_RENDER_TARGET, Some(i as u32));
            }
            s.ds = peek(device, GET_DEPTH_STENCIL, None);
        }
        s
    }

    /// A Set* call, before it runs.
    unsafe fn follow(&mut self, method_index: u32, args: &[u32]) {
        if self.recording_block {
            if method_index == DEV_END_STATE_BLOCK {
                self.recording_block = false;
            }
            return;
        }
        // (pointers the game hands D3D: valid for the call)
        let read_f32s = |p: u32, out: &mut [[f32; 4]]| {
            if p != 0 {
                unsafe { std::ptr::copy_nonoverlapping(p as usize as *const [f32; 4], out.as_mut_ptr(), out.len()) };
            }
        };
        match method_index {
            DEV_BEGIN_STATE_BLOCK => self.recording_block = true,
            DEV_SET_RENDER_TARGET => {
                if (args[0] as usize) < RENDER_TARGETS {
                    self.rt[args[0] as usize] = args[1];
                }
                self.viewport = None;
                self.scissor = None;
            }
            DEV_SET_DEPTH_STENCIL => self.ds = args[0],
            DEV_SET_VIEWPORT => {
                self.viewport = (args[0] != 0).then(|| {
                    let v = unsafe { *(args[0] as usize as *const [u32; 6]) };
                    Viewport { x: v[0], y: v[1], w: v[2], h: v[3], min_z: f32::from_bits(v[4]), max_z: f32::from_bits(v[5]) }
                });
            }
            DEV_SET_RENDER_STATE => {
                if (args[0] as usize) < 256 {
                    self.states[args[0] as usize] = args[1];
                }
            }
            DEV_SET_TEXTURE => {
                if (args[0] as usize) < SAMPLERS {
                    self.textures[args[0] as usize] = args[1];
                }
            }
            DEV_SET_SAMPLER_STATE => {
                if (args[0] as usize) < SAMPLERS && (args[1] as usize) < SAMPLER_STATES {
                    self.samplers[args[0] as usize][args[1] as usize] = args[2];
                }
            }
            DEV_SET_SCISSOR_RECT => {
                self.scissor = (args[0] != 0).then(|| unsafe { *(args[0] as usize as *const [i32; 4]) });
            }
            DEV_SET_VERTEX_DECLARATION => self.decl = Some(args[0]),
            DEV_SET_FVF => self.decl = None,
            DEV_SET_VERTEX_SHADER => self.vs = args[0],
            DEV_SET_PIXEL_SHADER => self.ps = args[0],
            DEV_SET_VS_CONSTANT_F => {
                let (start, n) = (args[0] as usize, args[2] as usize);
                if start < VS_CONSTS {
                    let end = (start + n).min(VS_CONSTS);
                    read_f32s(args[1], &mut Arc::make_mut(&mut self.vs_consts)[start..end]);
                }
            }
            DEV_SET_PS_CONSTANT_F => {
                let (start, n) = (args[0] as usize, args[2] as usize);
                if start < PS_CONSTS {
                    let end = (start + n).min(PS_CONSTS);
                    read_f32s(args[1], &mut Arc::make_mut(&mut self.ps_consts)[start..end]);
                }
            }
            DEV_SET_STREAM_SOURCE => {
                if (args[0] as usize) < 4 {
                    self.streams[args[0] as usize] = Stream { vb: args[1], offset: args[2], stride: args[3] };
                }
            }
            DEV_SET_INDICES => self.ib = args[0],
            _ => {}
        }
    }
}

/// The device state now (read back where not known).
fn with_shadow<R>(device: u32, f: impl FnOnce(&mut Shadow) -> R) -> Option<R> {
    let mut guard = SHADOW.lock().ok()?;
    let stale = SHADOW_STALE.swap(false, std::sync::atomic::Ordering::SeqCst);
    if stale || guard.as_ref().is_none_or(|s| !s.valid) {
        unsafe { hook_state_blocks(device) };
        let recording = guard.as_ref().is_some_and(|s| s.recording_block);
        let mut s = unsafe { Shadow::read(device) };
        s.recording_block = recording;
        *guard = Some(s);
    }
    let s = guard.as_mut()?;
    unsafe {
        if s.viewport.is_none() {
            s.viewport = Some(viewport(device));
        }
        if s.scissor.is_none() {
            s.scissor = Some(scissor_rect(device));
        }
        if s.decl.is_none() {
            s.decl = Some(peek(device, GET_VERTEX_DECLARATION, None));
        }
    }
    Some(f(s))
}

/// Takes a reference on each object for the frame, once a frame per object.
fn hold_all(objs: impl Iterator<Item = u32>) {
    let mut fresh = Vec::new();
    if let Ok(mut f) = FRAME.lock() {
        for o in objs {
            if o != 0 && f.held_set.insert(o) {
                fresh.push(o);
            }
        }
        for &o in &fresh {
            type AddRef = unsafe extern "system" fn(usize) -> u32;
            unsafe { std::mem::transmute::<usize, AddRef>(method(o, 1))(o as usize) };
            f.held.push(o);
        }
    }
}

/// Takes a reference on an object for the frame, once a frame per object.
fn hold_once(obj: u32) {
    if obj == 0 {
        return;
    }
    let fresh = FRAME.lock().is_ok_and(|mut f| f.held_set.insert(obj));
    if fresh {
        hold(obj);
    }
}

const D3DUSAGE_DYNAMIC: u32 = 0x200;

/// The buffer's usage flags and whether it's 32-bit (index buffers).
unsafe fn buffer_desc(buf: u32) -> (u32, bool) {
    if let Ok(f) = FRAME.lock()
        && let Some(&d) = f.buffer_descs.get(&buf)
    {
        return d;
    }
    let d = unsafe { buffer_desc_now(buf) };
    if let Ok(mut f) = FRAME.lock() {
        f.buffer_descs.insert(buf, d);
    }
    d
}

unsafe fn buffer_desc_now(buf: u32) -> (u32, bool) {
    let mut desc = [0u32; 6];
    type GetDesc = unsafe extern "system" fn(usize, *mut u32) -> i32;
    unsafe { std::mem::transmute::<usize, GetDesc>(method(buf, 13))(buf as usize, desc.as_mut_ptr()) };
    (desc[2], desc[0] == 102)
}

/// Copies what the draw reads from dynamic buffers, rebasing the draw onto the copies.
unsafe fn snapshot_dynamic(d: &mut Draw) {
    let count = vertex_count(d.prim, d.prims);
    let (first, n) = if d.indexed { ((d.base_vertex + d.min_index as i32).max(0) as u32, d.num_vertices) } else { (d.start, count) };
    let mut rebased = false;
    for i in 0..4 {
        let s = d.streams[i];
        if s.vb == 0 || s.stride == 0 || unsafe { buffer_desc(s.vb) }.0 & D3DUSAGE_DYNAMIC == 0 {
            continue;
        }
        let from = (s.offset + first * s.stride) as usize;
        let len = (n * s.stride) as usize;
        let bytes = unsafe { crate::d3d::with_buffer(s.vb, |b| b.get(from..from + len).map(|r| r.to_vec())) }.flatten();
        if let Some(bytes) = bytes {
            d.vb_data[i] = Some(Arc::new(bytes));
            d.streams[i].offset = 0;
            rebased = true;
        }
    }
    if rebased {
        // every stream must be rebased the same way
        for i in 0..4 {
            let s = d.streams[i];
            if s.vb != 0 && d.vb_data[i].is_none() && s.stride != 0 {
                let from = (s.offset + first * s.stride) as usize;
                let len = (n * s.stride) as usize;
                d.vb_data[i] = unsafe { crate::d3d::with_buffer(s.vb, |b| b.get(from..from + len).map(|r| Arc::new(r.to_vec()))) }.flatten();
                d.streams[i].offset = 0;
            }
        }
        if d.indexed {
            d.base_vertex = -(d.min_index as i32);
        } else {
            d.start = 0;
        }
    }
    if d.indexed && d.ib != 0 {
        let (usage, big) = unsafe { buffer_desc(d.ib) };
        if usage & D3DUSAGE_DYNAMIC != 0 {
            let size = if big { 4 } else { 2 };
            let from = (d.start * size) as usize;
            let len = (count * size) as usize;
            if let Some(bytes) = unsafe { crate::d3d::with_buffer(d.ib, |b| b.get(from..from + len).map(|r| r.to_vec())) }.flatten() {
                d.up_indices = Some((Arc::new(bytes), big));
                d.start = 0;
            }
        }
    }
}

pub fn vertex_count(prim: u32, prims: u32) -> u32 {
    match prim {
        1 => prims,         // points
        2 => prims * 2,     // lines
        3 => prims + 1,     // line strip
        4 => prims * 3,     // triangles
        5 | 6 => prims + 2, // strip, fan
        _ => 0,
    }
}

#[allow(dead_code)]
fn _unused(_: *mut c_void) {}
