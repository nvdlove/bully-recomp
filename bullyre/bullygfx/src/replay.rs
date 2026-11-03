//! The replay renderer: runs each frame the game recorded (`capture`) again on wgpu,
//! with the game's own shaders translated (`dxso`), its vertex and index buffers, its
//! textures, samplers and render states, and its own render targets. Everything the
//! game draws goes through here the same way: world, characters, sky, HUD, post
//! effects. The result is shown in a window laid over the game's (F8 or the control
//! script turns it on).

use std::collections::BTreeMap;

/// The caches are looked up several times per draw, thousands of draws a frame: a
/// fast multiply-rotate hash instead of the default (DoS-resistant, slower) one.
type HashMap<K, V> = std::collections::HashMap<K, V, std::hash::BuildHasherDefault<Fx>>;

#[derive(Default, Clone, Copy)]
struct Fx(u64);

impl Fx {
    #[inline]
    fn add(&mut self, v: u64) {
        self.0 = (self.0.rotate_left(5) ^ v).wrapping_mul(0x51_7c_c1_b7_27_22_0a_95);
    }
}

impl std::hash::Hasher for Fx {
    fn write(&mut self, bytes: &[u8]) {
        let mut chunks = bytes.chunks_exact(8);
        for c in &mut chunks {
            self.add(u64::from_le_bytes(c.try_into().unwrap()));
        }
        let rest = chunks.remainder();
        if !rest.is_empty() {
            let mut b = [0u8; 8];
            b[..rest.len()].copy_from_slice(rest);
            self.add(u64::from_le_bytes(b));
        }
    }
    fn write_u8(&mut self, i: u8) {
        self.add(i as u64);
    }
    fn write_u16(&mut self, i: u16) {
        self.add(i as u64);
    }
    fn write_u32(&mut self, i: u32) {
        self.add(i as u64);
    }
    fn write_u64(&mut self, i: u64) {
        self.add(i);
    }
    fn write_usize(&mut self, i: usize) {
        self.add(i as u64);
    }
    fn finish(&self) -> u64 {
        self.0
    }
}
use std::ffi::c_void;
use std::num::{NonZeroIsize, NonZeroU64};
use std::sync::Arc;

use wgpu::util::DeviceExt;

use crate::buffers;
use crate::capture::{self, rs, Cmd, Draw, Viewport, PS_CONSTS, VS_CONSTS};
use crate::d3d;
use crate::settings::{self, Settings, TextureMode};
use crate::textures;
use crate::dxso::{self, Input};
use crate::win::*;

const GAME_HWND: u32 = 0x00BD_77FC;
const TOGGLE_KEY: i32 = 0x77; // F8

/// Per-draw uniform block: vertex constants, pixel constants, frame block.
const VC_SIZE: u64 = (VS_CONSTS * 16) as u64;
const PC_SIZE: u64 = (PS_CONSTS * 16) as u64;
const FRAME_SIZE: u64 = 32;
const PC_AT: u64 = VC_SIZE;
const FRAME_AT: u64 = VC_SIZE + PC_SIZE; // 7680, a multiple of 256
const DRAW_BLOCK: u64 = FRAME_AT + 256;

fn log(s: &str) {
    crate::control::log(s);
}

unsafe fn method(obj: u32, i: usize) -> usize {
    unsafe { *(*(obj as *const *const usize)).add(i) }
}

unsafe fn add_ref(obj: u32) -> u32 {
    type AddRef = unsafe extern "system" fn(usize) -> u32;
    unsafe { std::mem::transmute::<usize, AddRef>(method(obj, 1))(obj as usize) }
}

unsafe fn release(obj: u32) {
    if obj != 0 {
        type Release = unsafe extern "system" fn(usize) -> u32;
        unsafe { std::mem::transmute::<usize, Release>(method(obj, 2))(obj as usize) };
    }
}

// ---- D3D9 enums to wgpu ------------------------------------------------------

/// D3DSURFACE_DESC: format, type, usage, pool, multisample, quality, width, height.
/// The texture a surface is a level of, if it is one. A texture's surfaces count their
/// references apart from it: the game holds the texture, not the surface.
unsafe fn container_texture(surface: u32) -> Option<u32> {
    // IID_IDirect3DBaseTexture9 {580CA87E-1D3C-4d54-991D-B7D3E3C298CE}
    const IID: [u8; 16] = [0x7E, 0xA8, 0x0C, 0x58, 0x3C, 0x1D, 0x54, 0x4D, 0x99, 0x1D, 0xB7, 0xD3, 0xE3, 0xC2, 0x98, 0xCE];
    type GetContainer = unsafe extern "system" fn(usize, *const u8, *mut usize) -> i32;
    let mut out = 0usize;
    let hr = unsafe { std::mem::transmute::<usize, GetContainer>(method(surface, 11))(surface as usize, IID.as_ptr(), &mut out) };
    if hr < 0 || out == 0 {
        return None;
    }
    let out = crate::win::obj32(out);
    unsafe { release(out) };
    Some(out)
}

unsafe fn surface_desc(surface: u32) -> Option<[u32; 8]> {
    type GetDesc = unsafe extern "system" fn(usize, *mut u32) -> i32;
    let mut d = [0u32; 8];
    (unsafe { std::mem::transmute::<usize, GetDesc>(method(surface, 12))(surface as usize, d.as_mut_ptr()) } >= 0).then_some(d)
}

fn color_format(fmt: u32) -> Option<wgpu::TextureFormat> {
    use wgpu::TextureFormat as F;
    Some(match fmt {
        21 | 22 | 23 | 25 | 26 => F::Bgra8Unorm, // A8R8G8B8, X8R8G8B8, R5G6B5, A1R5G5B5, A4R4G4B4
        31 | 35 => F::Rgb10a2Unorm,               // A2B10G10R10, A2R10G10B10
        111 => F::R16Float,
        112 => F::Rg16Float,
        113 => F::Rgba16Float,
        114 => F::R32Float,
        115 => F::Rg32Float,
        116 => F::Rgba32Float,
        _ => return None,
    })
}

fn depth_format(fmt: u32) -> Option<wgpu::TextureFormat> {
    use wgpu::TextureFormat as F;
    Some(match fmt {
        75 | 77 | 79 | 83 => F::Depth24PlusStencil8, // D24S8, D24X8, D24X4S4, D24FS8
        80 => F::Depth16Unorm,
        71 | 82 => F::Depth32Float, // D32, D32F_LOCKABLE
        _ => return None,
    })
}

fn has_stencil(f: wgpu::TextureFormat) -> bool {
    f == wgpu::TextureFormat::Depth24PlusStencil8
}

fn compare(f: u32) -> wgpu::CompareFunction {
    use wgpu::CompareFunction as C;
    match f {
        1 => C::Never,
        2 => C::Less,
        3 => C::Equal,
        4 => C::LessEqual,
        5 => C::Greater,
        6 => C::NotEqual,
        7 => C::GreaterEqual,
        _ => C::Always,
    }
}

fn blend_factor(b: u32) -> wgpu::BlendFactor {
    use wgpu::BlendFactor as B;
    match b {
        1 => B::Zero,
        2 => B::One,
        3 => B::Src,
        4 => B::OneMinusSrc,
        5 => B::SrcAlpha,
        6 => B::OneMinusSrcAlpha,
        7 => B::DstAlpha,
        8 => B::OneMinusDstAlpha,
        9 => B::Dst,
        10 => B::OneMinusDst,
        11 => B::SrcAlphaSaturated,
        14 => B::Constant,
        15 => B::OneMinusConstant,
        _ => B::One,
    }
}

fn blend_op(o: u32) -> wgpu::BlendOperation {
    use wgpu::BlendOperation as O;
    match o {
        2 => O::Subtract,
        3 => O::ReverseSubtract,
        4 => O::Min,
        5 => O::Max,
        _ => O::Add,
    }
}

fn stencil_op(o: u32) -> wgpu::StencilOperation {
    use wgpu::StencilOperation as S;
    match o {
        2 => S::Zero,
        3 => S::Replace,
        4 => S::IncrementClamp,
        5 => S::DecrementClamp,
        6 => S::Invert,
        7 => S::IncrementWrap,
        8 => S::DecrementWrap,
        _ => S::Keep,
    }
}

fn address(a: u32) -> wgpu::AddressMode {
    match a {
        2 | 5 => wgpu::AddressMode::MirrorRepeat,
        3 | 4 => wgpu::AddressMode::ClampToEdge,
        _ => wgpu::AddressMode::Repeat,
    }
}

/// D3DDECLTYPE as a wgpu vertex format and the shader input type it arrives as.
fn vertex_format(ty: u8) -> Option<(wgpu::VertexFormat, Input)> {
    use wgpu::VertexFormat as V;
    Some(match ty {
        0 => (V::Float32, Input::Float),
        1 => (V::Float32x2, Input::Float),
        2 => (V::Float32x3, Input::Float),
        3 => (V::Float32x4, Input::Float),
        4 => (V::Unorm8x4Bgra, Input::Float), // D3DCOLOR: bytes B, G, R, A
        5 => (V::Uint8x4, Input::Uint),
        6 => (V::Sint16x2, Input::Sint),
        7 => (V::Sint16x4, Input::Sint),
        8 => (V::Unorm8x4, Input::Float),
        9 => (V::Snorm16x2, Input::Float),
        10 => (V::Snorm16x4, Input::Float),
        11 => (V::Unorm16x2, Input::Float),
        12 => (V::Unorm16x4, Input::Float),
        15 => (V::Float16x2, Input::Float),
        16 => (V::Float16x4, Input::Float),
        _ => return None,
    })
}

// ---- keys ------------------------------------------------------------------

#[derive(Clone, PartialEq, Eq, Hash)]
struct Element {
    stream: u16,
    offset: u16,
    ty: u8,
    usage: u8,
    index: u8,
}

/// How a vertex declaration feeds a vertex shader: per stream, stride and attributes.
#[derive(Clone, PartialEq, Eq, Hash)]
struct VertexLayout {
    /// (stream, stride, offset into the stream, [(format, offset, location)]); an element
    /// reaching past the stride (D3D9 reads on into the next vertex, WebGPU refuses) is
    /// its own buffer slot at its offset.
    streams: Vec<(u32, u32, u64, Vec<(wgpu::VertexFormat, u64, u32)>)>,
    inputs: Vec<(u32, Input)>,
}

#[derive(Clone, PartialEq, Eq, Hash)]
struct PipelineKey {
    vs: u64,
    ps: u64,
    layout: VertexLayout,
    topology: wgpu::PrimitiveTopology,
    strip_index: Option<wgpu::IndexFormat>,
    cull: Option<wgpu::Face>,
    colors: Vec<(wgpu::TextureFormat, Option<wgpu::BlendState>, u32)>,
    depth: Option<(wgpu::TextureFormat, bool, wgpu::CompareFunction, Option<StencilKey>, i32, u32)>,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
struct StencilKey {
    func: u32,
    fail: u32,
    zfail: u32,
    pass: u32,
    mask: u32,
    write: u32,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
struct SamplerKey([u32; 8]);

/// What sampler slots a shader pair binds, and as what.
type SamplerSig = Vec<(u32, u32)>; // (slot, texture type 2 = 2D / 3 = cube)

struct ShaderInfo {
    hash: u64,
    shader: dxso::Shader,
    /// Bytes of its float constants it reads (all of them with relative addressing).
    const_bytes: usize,
}

struct CachedBuffer {
    buffer: wgpu::Buffer,
    sig: u64,
    checked: u64,
    size: u64,
}

struct Rt {
    texture: wgpu::Texture,
    view: wgpu::TextureView,
    format: wgpu::TextureFormat,
    width: u32,
    height: u32,
    rendered: bool,
    /// The game's format has no alpha (X8R8G8B8, R5G6B5, X1R5G5B5): D3D9 samples alpha
    /// as 1 whatever was drawn, so our copy keeps alpha at 1 and never writes it.
    opaque: bool,
}

struct GameTexture {
    view: Option<wgpu::TextureView>, // None: unreadable
    cube: bool,
    sig: u64,
    checked: u64,
}

fn signature(bytes: &[u8]) -> u64 {
    let mut h: u64 = 0xcbf2_9ce4_8422_2325 ^ bytes.len() as u64;
    let n = bytes.len();
    for at in [0, n / 4, n / 2, 3 * n / 4, n.saturating_sub(64)] {
        for &b in bytes.iter().skip(at).take(64) {
            h = (h ^ b as u64).wrapping_mul(0x100_0000_01b3);
        }
    }
    h
}

unsafe extern "system" fn window_proc(h: *mut c_void, m: u32, w: usize, l: isize) -> isize {
    if m == WM_NCHITTEST {
        return HTTRANSPARENT;
    }
    // never activated or focused by a click: the game keeps focus (and its input)
    const WM_MOUSEACTIVATE: u32 = 0x0021;
    const MA_NOACTIVATE: isize = 3;
    if m == WM_MOUSEACTIVATE {
        return MA_NOACTIVATE;
    }
    unsafe { DefWindowProcW(h, m, w, l) }
}

fn client_size(h: *mut c_void) -> (u32, u32) {
    let mut r = RECT::default();
    unsafe { GetClientRect(h, &mut r) };
    (r.right.max(1) as u32, r.bottom.max(1) as u32)
}

const BLIT_SHADER: &str = r#"
@group(0) @binding(0) var src: texture_2d<f32>;
@group(0) @binding(1) var smp: sampler;
struct Rect { uv: vec4<f32> }; // x0, y0, x1, y1 in the source
@group(0) @binding(2) var<uniform> rect: Rect;
struct Out { @builtin(position) pos: vec4<f32>, @location(0) uv: vec2<f32> };
@vertex fn vs(@builtin(vertex_index) i: u32) -> Out {
    let c = vec2<f32>(f32(i & 1u), f32(i >> 1u));
    var o: Out;
    o.pos = vec4<f32>(c.x * 2.0 - 1.0, 1.0 - c.y * 2.0, 0.0, 1.0);
    o.uv = mix(rect.uv.xy, rect.uv.zw, c);
    return o;
}
@fragment fn fs(v: Out) -> @location(0) vec4<f32> {
    return textureSampleLevel(src, smp, v.uv, 0.0);
}
"#;

/// The finished frame to our window, with the customiser's picture settings.
const PRESENT_SHADER: &str = r#"
@group(0) @binding(0) var src: texture_2d<f32>;
struct Params {
    uv: vec4<f32>,      // x0, y0, x1, y1 in the source
    grade: vec4<f32>,   // exposure, contrast, saturation, gamma
    tint: vec4<f32>,    // rgb, strength
    extra: vec4<f32>,   // vignette, sharpen, bloom
};
@group(0) @binding(1) var<uniform> p: Params;
@group(0) @binding(2) var bloom: texture_2d<f32>;
@group(0) @binding(3) var bloom_smp: sampler;
struct Out { @builtin(position) pos: vec4<f32>, @location(0) uv: vec2<f32>, @location(1) screen: vec2<f32> };
@vertex fn vs(@builtin(vertex_index) i: u32) -> Out {
    let c = vec2<f32>(f32(i & 1u), f32(i >> 1u));
    var o: Out;
    o.pos = vec4<f32>(c.x * 2.0 - 1.0, 1.0 - c.y * 2.0, 0.0, 1.0);
    o.uv = mix(p.uv.xy, p.uv.zw, c);
    o.screen = c;
    return o;
}
fn at(q: vec2<i32>) -> vec3<f32> {
    let size = vec2<i32>(textureDimensions(src));
    return textureLoad(src, clamp(q, vec2<i32>(0), size - 1), 0).rgb;
}
@fragment fn fs(v: Out) -> @location(0) vec4<f32> {
    let q = vec2<i32>(v.uv * vec2<f32>(textureDimensions(src)));
    var c = at(q);
    if p.extra.y > 0.0 {
        let n = (at(q + vec2<i32>(1, 0)) + at(q - vec2<i32>(1, 0)) + at(q + vec2<i32>(0, 1)) + at(q - vec2<i32>(0, 1))) * 0.25;
        c = c + (c - n) * p.extra.y;
    }
    if p.extra.z > 0.0 {
        c = c + textureSampleLevel(bloom, bloom_smp, v.uv, 0.0).rgb * p.extra.z;
    }
    c = c * p.grade.x;
    c = (c - 0.5) * p.grade.y + 0.5;
    let l = dot(c, vec3<f32>(0.2126, 0.7152, 0.0722));
    c = mix(vec3<f32>(l), c, p.grade.z);
    c = mix(c, c * p.tint.rgb, p.tint.w);
    c = pow(max(c, vec3<f32>(0.0)), vec3<f32>(1.0 / p.grade.w));
    let d = length(v.screen - 0.5) * 1.41421;
    c = c * (1.0 - p.extra.x * smoothstep(0.35, 1.0, d));
    return vec4<f32>(c, 1.0);
}
"#;

pub struct Replay {
    window: *mut c_void,
    parent: *mut c_void,
    size: (u32, u32),
    surface: wgpu::Surface<'static>,
    device: wgpu::Device,
    queue: wgpu::Queue,
    config: wgpu::SurfaceConfiguration,
    uniform_layout: wgpu::BindGroupLayout,
    texture_layouts: HashMap<SamplerSig, (wgpu::BindGroupLayout, wgpu::PipelineLayout)>,
    shaders: HashMap<u32, Option<Arc<ShaderInfo>>>,
    modules: HashMap<(u64, Vec<(u32, Input)>), Option<wgpu::ShaderModule>>,
    pipelines: HashMap<PipelineKey, Option<wgpu::RenderPipeline>>,
    decls: HashMap<u32, Vec<Element>>,
    vbs: HashMap<u32, CachedBuffer>,
    ibs: HashMap<u32, (CachedBuffer, bool)>,
    textures: HashMap<u32, GameTexture>,
    rts: HashMap<u32, Rt>,
    depths: HashMap<u32, (wgpu::TextureView, wgpu::TextureFormat, u32, u32)>,
    tex_surface: HashMap<u32, Option<u32>>, // render-target texture -> its level 0 surface
    samplers: HashMap<SamplerKey, wgpu::Sampler>,
    white_2d: wgpu::TextureView,
    white_cube: wgpu::TextureView,
    /// For a 2D texture bound where the shader samples a cube (D3D9 reads black).
    black_cube: wgpu::TextureView,
    blit_layout: wgpu::BindGroupLayout,
    blit_pipelines: HashMap<(wgpu::TextureFormat, bool), wgpu::RenderPipeline>,
    blit_sampler: wgpu::Sampler,
    /// The final pass: its layout and a pipeline per window format.
    present_layout: wgpu::BindGroupLayout,
    present_pipelines: HashMap<wgpu::TextureFormat, wgpu::RenderPipeline>,
    /// The customiser's settings.
    custom: Settings,
    /// Replacement textures by game texture (None: none for it), whether that was
    /// looked up by the texture's file name, and when.
    replaced: HashMap<u32, (Option<wgpu::TextureView>, bool, u64)>,
    checker: Option<wgpu::TextureView>,
    pub gpu: String,
    /// The replay's time for recent frames (ms, smoothed).
    last_ms: f32,
    /// Depth-only views of the depth surfaces (for sampling).
    depth_samples: HashMap<u32, wgpu::TextureView>,
    wet: crate::wet::Wet,
    /// FXAA and bloom on the finished frame.
    post: crate::post::Post,
    /// Seamless doors' doorways (portal.rs).
    portal: crate::portal::Portal,
    /// The camera set last, and the one the 3D scene in the back buffer was drawn with
    /// (and its depth surface).
    camera: Option<capture::Camera>,
    scene: Option<(capture::Camera, u32)>,
    /// How wet the streets are (0 dry .. 1 full puddles), and when that was last updated.
    wetness: f32,
    wet_clock: Option<std::time::Instant>,
    frame: u64,
    reported: std::collections::HashSet<String>,
    /// Game objects the caches are keyed by, each with a reference of ours so its
    /// address can't be reused while cached; let go once the game has.
    held: std::collections::HashSet<u32>,
    /// Textures held for their surfaces (render targets we copy), and those surfaces.
    surfaces_of: HashMap<u32, Vec<u32>>,
    /// Seconds: whole replay, encoding, submit + present; frames counted.
    perf: [f64; 11],
    /// This frame's texture work: seconds, new uploads, their bytes, fingerprint checks.
    frame_tex: (f64, u32, usize, u32),
    last_draws: u64,
    uniform_bytes: Vec<u8>,
    /// Last frame: distinct vertex and pixel shader constant blocks, bytes uploaded.
    unique_constants: (usize, usize, usize),
    /// Texture bind groups by what's in them.
    bind_groups: HashMap<(SamplerSig, Vec<(u32, wgpu::TextureView, wgpu::Sampler)>), wgpu::BindGroup>,
    /// The per-draw constants (buffer, its bind group, capacity), grown as needed.
    uniforms: Option<(wgpu::Buffer, wgpu::BindGroup, u64)>,
}

impl Replay {
    pub fn new() -> Result<Replay, String> {
        let parent = read_u32(GAME_HWND).ok_or("no game window")? as usize as *mut c_void;
        if parent.is_null() {
            return Err("no game window".into());
        }
        let size = client_size(parent);
        let inst = unsafe { GetModuleHandleW(std::ptr::null()) };
        let class = wide("bullygfx");
        let wc = WNDCLASSEXW {
            cbSize: std::mem::size_of::<WNDCLASSEXW>() as u32,
            style: 0,
            lpfnWndProc: window_proc,
            cbClsExtra: 0,
            cbWndExtra: 0,
            hInstance: inst,
            hIcon: std::ptr::null_mut(),
            hCursor: std::ptr::null_mut(),
            hbrBackground: std::ptr::null_mut(),
            lpszMenuName: std::ptr::null(),
            lpszClassName: class.as_ptr(),
            hIconSm: std::ptr::null_mut(),
        };
        unsafe { RegisterClassExW(&wc) };
        let window = unsafe {
            CreateWindowExW(0, class.as_ptr(), class.as_ptr(), WS_CHILD | WS_CLIPSIBLINGS | WS_DISABLED, 0, 0, size.0 as i32, size.1 as i32, parent, std::ptr::null_mut(), inst, std::ptr::null_mut())
        };
        if window.is_null() {
            return Err("couldn't create the window".into());
        }
        let mut handle = raw_window_handle::Win32WindowHandle::new(NonZeroIsize::new(window as isize).unwrap());
        handle.hinstance = NonZeroIsize::new(inst as isize);
        let target = || wgpu::SurfaceTargetUnsafe::RawHandle {
            raw_display_handle: Some(raw_window_handle::RawDisplayHandle::Windows(raw_window_handle::WindowsDisplayHandle::new())),
            raw_window_handle: raw_window_handle::RawWindowHandle::Win32(handle),
        };
        // Vulkan first: wgpu's D3D12 pipelines fail in a 32-bit process
        let mut found = None;
        let mut errors = Vec::new();
        for backends in [wgpu::Backends::VULKAN, wgpu::Backends::DX12] {
            let mut desc = wgpu::InstanceDescriptor::new_without_display_handle();
            desc.backends = backends;
            desc.backend_options.dx12.shader_compiler = wgpu::Dx12Compiler::Fxc;
            let instance = wgpu::Instance::new(desc);
            let surface = match unsafe { instance.create_surface_unsafe(target()) } {
                Ok(s) => s,
                Err(e) => {
                    errors.push(format!("{backends:?} surface: {e}"));
                    continue;
                }
            };
            match pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::HighPerformance,
                compatible_surface: Some(&surface),
                force_fallback_adapter: false,
                ..Default::default()
            })) {
                Ok(adapter) => {
                    found = Some((surface, adapter));
                    break;
                }
                Err(e) => errors.push(format!("{backends:?} adapter: {e}")),
            }
        }
        let (surface, adapter) = found.ok_or_else(|| errors.join("; "))?;
        // (FLOAT32_FILTERABLE: the game's High shaders draw their shadow maps into R32F
        // targets and sample them where every texture is bound as filterable)
        let features = adapter.features() & (wgpu::Features::TEXTURE_COMPRESSION_BC | wgpu::Features::DEPTH32FLOAT_STENCIL8 | wgpu::Features::FLOAT32_FILTERABLE);
        let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor { label: Some("bullygfx replay"), required_features: features, ..Default::default() }))
            .map_err(|e| format!("device: {e}"))?;
        let wet = crate::wet::Wet::new(&device);
        log(&format!("replay renderer on {} ({:?}), float32 filtering {}", adapter.get_info().name, adapter.get_info().backend, device.features().contains(wgpu::Features::FLOAT32_FILTERABLE)));
        device.on_uncaptured_error(std::sync::Arc::new(|e: wgpu::Error| {
            static SEEN: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
            if SEEN.fetch_add(1, std::sync::atomic::Ordering::Relaxed) < 10 {
                let mut text = format!("{e}");
                text.truncate(1500);
                log(&format!("wgpu error: {text}"));
            }
        }));
        let caps = surface.get_capabilities(&adapter);
        let format = caps.formats.iter().copied().find(|f| !f.is_srgb()).unwrap_or(caps.formats[0]);
        let present_mode = if caps.present_modes.contains(&wgpu::PresentMode::Immediate) { wgpu::PresentMode::Immediate } else { wgpu::PresentMode::Fifo };
        let config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format,
            color_space: wgpu::SurfaceColorSpace::Auto,
            width: size.0,
            height: size.1,
            present_mode,
            desired_maximum_frame_latency: 2,
            alpha_mode: caps.alpha_modes[0],
            view_formats: vec![],
        };
        surface.configure(&device, &config);

        let dynamic_uniform = |binding, size: u64, visibility| wgpu::BindGroupLayoutEntry {
            binding,
            visibility,
            ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Uniform, has_dynamic_offset: true, min_binding_size: NonZeroU64::new(size) },
            count: None,
        };
        let both = wgpu::ShaderStages::VERTEX | wgpu::ShaderStages::FRAGMENT;
        let uniform_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("constants"),
            entries: &[dynamic_uniform(0, VC_SIZE, both), dynamic_uniform(1, PC_SIZE, both), dynamic_uniform(2, FRAME_SIZE, both)],
        });
        let plain = |cube: bool, value: u8| {
            let layers = if cube { 6 } else { 1 };
            let t = device.create_texture_with_data(
                &queue,
                &wgpu::TextureDescriptor {
                    label: Some("white"),
                    size: wgpu::Extent3d { width: 1, height: 1, depth_or_array_layers: layers },
                    mip_level_count: 1,
                    sample_count: 1,
                    dimension: wgpu::TextureDimension::D2,
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    usage: wgpu::TextureUsages::TEXTURE_BINDING,
                    view_formats: &[],
                },
                wgpu::util::TextureDataOrder::LayerMajor,
                &[value, value, value, 255].repeat(layers as usize),
            );
            t.create_view(&wgpu::TextureViewDescriptor {
                dimension: Some(if cube { wgpu::TextureViewDimension::Cube } else { wgpu::TextureViewDimension::D2 }),
                ..Default::default()
            })
        };
        let blit_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("blit"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture { sample_type: wgpu::TextureSampleType::Float { filterable: false }, view_dimension: wgpu::TextureViewDimension::D2, multisampled: false },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry { binding: 1, visibility: wgpu::ShaderStages::FRAGMENT, ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::NonFiltering), count: None },
                wgpu::BindGroupLayoutEntry {
                    binding: 2,
                    visibility: wgpu::ShaderStages::VERTEX,
                    ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Uniform, has_dynamic_offset: false, min_binding_size: None },
                    count: None,
                },
            ],
        });
        let present_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("present"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture { sample_type: wgpu::TextureSampleType::Float { filterable: false }, view_dimension: wgpu::TextureViewDimension::D2, multisampled: false },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::VERTEX | wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Uniform, has_dynamic_offset: false, min_binding_size: None },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 2,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture { sample_type: wgpu::TextureSampleType::Float { filterable: true }, view_dimension: wgpu::TextureViewDimension::D2, multisampled: false },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry { binding: 3, visibility: wgpu::ShaderStages::FRAGMENT, ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering), count: None },
            ],
        });
        let post = crate::post::Post::new(&device, &queue);
        let portal = crate::portal::Portal::new(&device);
        let blit_sampler = device.create_sampler(&wgpu::SamplerDescriptor { label: Some("blit"), ..Default::default() });
        Ok(Replay {
            window,
            parent,
            size,
            surface,
            white_2d: plain(false, 255),
            white_cube: plain(true, 255),
            black_cube: plain(true, 0),
            device,
            queue,
            config,
            uniform_layout,
            texture_layouts: HashMap::default(),
            shaders: HashMap::default(),
            modules: HashMap::default(),
            pipelines: HashMap::default(),
            decls: HashMap::default(),
            vbs: HashMap::default(),
            ibs: HashMap::default(),
            textures: HashMap::default(),
            rts: HashMap::default(),
            depths: HashMap::default(),
            tex_surface: HashMap::default(),
            samplers: HashMap::default(),
            blit_layout,
            blit_pipelines: HashMap::default(),
            blit_sampler,
            present_layout,
            present_pipelines: HashMap::default(),
            custom: Settings::default(),
            replaced: HashMap::default(),
            checker: None,
            gpu: adapter.get_info().name,
            last_ms: 0.0,
            depth_samples: HashMap::default(),
            wet,
            post,
            portal,
            camera: None,
            scene: None,
            wetness: 0.0,
            wet_clock: None,
            frame: 0,
            reported: Default::default(),
            held: Default::default(),
            surfaces_of: HashMap::default(),
            perf: [0.0; 11],
            frame_tex: (0.0, 0, 0, 0),
            last_draws: 0,
            uniform_bytes: Vec::new(),
            unique_constants: (0, 0, 0),
            bind_groups: HashMap::default(),
            uniforms: None,
        })
    }

    fn report(&mut self, what: String) {
        if self.reported.len() < 200 && self.reported.insert(what.clone()) {
            log(&what);
        }
    }

    pub fn show(&self, on: bool) {
        unsafe { ShowWindow(self.window, if on { SW_SHOWNA } else { SW_HIDE }) };
    }

    fn follow_parent(&mut self) {
        let size = client_size(self.parent);
        if size != self.size {
            self.size = size;
            unsafe { SetWindowPos(self.window, HWND_TOP, 0, 0, size.0 as i32, size.1 as i32, SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS) };
            self.config.width = size.0;
            self.config.height = size.1;
            self.surface.configure(&self.device, &self.config);
        }
    }

    // ---- resources ------------------------------------------------------------

    fn hold(&mut self, obj: u32) {
        if obj != 0 && self.held.insert(obj) {
            unsafe { add_ref(obj) };
        }
    }

    /// A surface's texture if it has one (what the game keeps), else the surface.
    fn hold_surface(&mut self, surface: u32) {
        match unsafe { container_texture(surface) } {
            Some(tex) => {
                self.hold(tex);
                let list = self.surfaces_of.entry(tex).or_default();
                if !list.contains(&surface) {
                    list.push(surface);
                }
            }
            None => self.hold(surface),
        }
    }

    fn forget(&mut self, obj: u32) {
        for s in self.surfaces_of.remove(&obj).unwrap_or_default() {
            self.rts.remove(&s);
            self.depths.remove(&s);
            self.depth_samples.remove(&s);
        }
        self.depth_samples.remove(&obj);
        self.replaced.remove(&obj);
        self.tex_surface.remove(&obj);
        self.bind_groups.clear();
        self.shaders.remove(&obj);
        self.decls.remove(&obj);
        self.vbs.remove(&obj);
        self.ibs.remove(&obj);
        self.textures.remove(&obj);
        self.rts.remove(&obj);
        self.depths.remove(&obj);
    }

    /// Drops what only we still reference.
    fn sweep(&mut self) {
        let gone: Vec<u32> = self
            .held
            .iter()
            .copied()
            .filter(|&o| unsafe {
                let n = add_ref(o);
                release(o);
                n <= 2 // ours, plus the one just taken
            })
            .collect();
        if !gone.is_empty() {
            let kinds = |o: &u32| {
                [("shader", self.shaders.contains_key(o)), ("decl", self.decls.contains_key(o)), ("vb", self.vbs.contains_key(o)), ("ib", self.ibs.contains_key(o)), ("texture", self.textures.contains_key(o)), ("rt", self.rts.contains_key(o)), ("depth", self.depths.contains_key(o)), ("rt texture", self.surfaces_of.contains_key(o))]
                    .iter()
                    .filter(|k| k.1)
                    .map(|k| k.0)
                    .collect::<Vec<_>>()
                    .join("+")
            };
            let mut count: BTreeMap<String, u32> = BTreeMap::new();
            for o in &gone {
                *count.entry(kinds(o)).or_default() += 1;
            }
            log(&format!("sweep: {count:?}"));
        }
        for o in gone {
            self.held.remove(&o);
            self.forget(o);
            unsafe { release(o) };
        }
    }

    /// Before the device is reset: every reference of ours goes (the reset fails while
    /// video memory resources are still referenced), and with them the caches.
    pub fn release_all(&mut self) {
        for o in std::mem::take(&mut self.held) {
            self.forget(o);
            unsafe { release(o) };
        }
        self.tex_surface.clear();
        self.surfaces_of.clear();
    }

    fn shader(&mut self, obj: u32) -> Option<Arc<ShaderInfo>> {
        if let Some(s) = self.shaders.get(&obj) {
            return s.clone();
        }
        let info = (|| {
            type GetFunction = unsafe extern "system" fn(usize, *mut u8, *mut u32) -> i32;
            let get: GetFunction = unsafe { std::mem::transmute(method(obj, 4)) };
            let mut size = 0u32;
            if unsafe { get(obj as usize, std::ptr::null_mut(), &mut size) } < 0 || size < 4 {
                return Err("no bytecode".to_string());
            }
            let mut code = vec![0u8; size as usize];
            unsafe { get(obj as usize, code.as_mut_ptr(), &mut size) };
            let tokens: Vec<u32> = code.chunks_exact(4).map(|c| u32::from_le_bytes(c.try_into().unwrap())).collect();
            let shader = dxso::parse(&tokens)?;
            let all = match shader.kind {
                dxso::Kind::Vertex => VC_SIZE,
                dxso::Kind::Pixel => PC_SIZE,
            } as usize;
            let const_bytes = shader.float_constants_read().map_or(all, |n| (n as usize * 16).min(all));
            Ok(Arc::new(ShaderInfo { hash: crate::shaders::hash(&tokens), shader, const_bytes }))
        })();
        let info = match info {
            Ok(i) => Some(i),
            Err(e) => {
                self.report(format!("shader {obj:#x}: {e}"));
                None
            }
        };
        self.hold(obj);
        self.shaders.insert(obj, info.clone());
        info
    }

    fn module(&mut self, info: &ShaderInfo, inputs: &[(u32, Input)]) -> Option<wgpu::ShaderModule> {
        let key = (info.hash, inputs.to_vec());
        if let Some(m) = self.modules.get(&key) {
            return m.clone();
        }
        let map: BTreeMap<u32, Input> = inputs.iter().copied().collect();
        let module = match info.shader.to_wgsl_with(&map) {
            Ok(text) => Some(self.device.create_shader_module(wgpu::ShaderModuleDescriptor { label: Some("game shader"), source: wgpu::ShaderSource::Wgsl(text.into()) })),
            Err(e) => {
                self.report(format!("shader {:016x}: {e}", info.hash));
                None
            }
        };
        self.modules.insert(key, module.clone());
        module
    }

    fn decl(&mut self, decl: u32) -> Vec<Element> {
        if let Some(d) = self.decls.get(&decl) {
            return d.clone();
        }
        let mut elements = [0u8; 8 * 65];
        let mut n = 65u32;
        type GetDeclaration = unsafe extern "system" fn(usize, *mut u8, *mut u32) -> i32;
        let f: GetDeclaration = unsafe { std::mem::transmute(method(decl, 4)) };
        let mut out = Vec::new();
        if unsafe { f(decl as usize, elements.as_mut_ptr(), &mut n) } >= 0 {
            for e in elements.chunks_exact(8).take(n as usize) {
                let stream = u16::from_le_bytes([e[0], e[1]]);
                if stream == 0xFF {
                    break;
                }
                out.push(Element { stream, offset: u16::from_le_bytes([e[2], e[3]]), ty: e[4], usage: e[6], index: e[7] });
            }
        }
        self.hold(decl);
        self.decls.insert(decl, out.clone());
        out
    }

    /// A static buffer's contents on the GPU (checked against the game's once a frame).
    fn static_buffer(&mut self, buf: u32, index: bool) -> Option<(wgpu::Buffer, bool)> {
        let t = std::time::Instant::now();
        let r = self.static_buffer_inner(buf, index);
        self.perf[4] += t.elapsed().as_secs_f64();
        r
    }

    fn static_buffer_inner(&mut self, buf: u32, index: bool) -> Option<(wgpu::Buffer, bool)> {
        // (a draw with no buffer bound: D3D9 would fail the call; the draw is left out, named by
        // the caller's error. buffers::watch on 0 read a null vtable, 2026-10-30's soak)
        if buf == 0 {
            return None;
        }
        let frame = self.frame;
        // cached, and not written since
        if !buffers::dirty(buf) {
            if index {
                if let Some((b, big)) = self.ibs.get(&buf) {
                    return Some((b.buffer.clone(), *big));
                }
            } else if let Some(b) = self.vbs.get(&buf) {
                return Some((b.buffer.clone(), false));
            }
        }
        buffers::watch(buf);
        buffers::clean(buf);
        // IB desc: format, type, usage, pool, size; VB: format, type, usage, pool, size, fvf
        let mut desc = [0u32; 6];
        type GetDesc = unsafe extern "system" fn(usize, *mut u32) -> i32;
        unsafe { std::mem::transmute::<usize, GetDesc>(method(buf, 13))(buf as usize, desc.as_mut_ptr()) };
        let big = index && desc[0] == 102;
        let have = if index { self.ibs.get(&buf).map(|b| b.0.sig) } else { self.vbs.get(&buf).map(|b| b.sig) };
        let fresh = unsafe {
            d3d::with_buffer(buf, |bytes| {
                let sig = signature(bytes);
                if have == Some(sig) { None } else { Some((bytes.to_vec(), sig)) }
            })
        }?;
        let entry = match fresh {
            Some((mut bytes, sig)) => {
                bytes.resize((bytes.len().div_ceil(4) * 4).max(4) + if index { 0 } else { VERTEX_PAD }, 0);
                let usage = if index { wgpu::BufferUsages::INDEX } else { wgpu::BufferUsages::VERTEX };
                let buffer = self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("game buffer"), contents: &bytes, usage });
                CachedBuffer { size: bytes.len() as u64, buffer, sig, checked: frame }
            }
            None => {
                let old = if index { &mut self.ibs.get_mut(&buf)?.0 } else { self.vbs.get_mut(&buf)? };
                old.checked = frame;
                return Some((old.buffer.clone(), big));
            }
        };
        let out = entry.buffer.clone();
        self.hold(buf);
        if index {
            self.ibs.insert(buf, (entry, big));
        } else {
            self.vbs.insert(buf, entry);
        }
        Some((out, big))
    }

    /// Our copy of a D3D render target surface (made on first use, the game's size and
    /// format).
    fn rt(&mut self, surface: u32) -> Option<&Rt> {
        if !self.rts.contains_key(&surface) {
            let d = unsafe { surface_desc(surface) }?;
            let Some(format) = color_format(d[0]) else {
                self.report(format!("render target format {} not handled", d[0]));
                return None;
            };
            let texture = self.device.create_texture(&wgpu::TextureDescriptor {
                label: Some("game render target"),
                size: wgpu::Extent3d { width: d[6].max(1), height: d[7].max(1), depth_or_array_layers: 1 },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_SRC | wgpu::TextureUsages::COPY_DST,
                view_formats: &[],
            });
            let view = texture.create_view(&Default::default());
            self.hold_surface(surface);
            if format == wgpu::TextureFormat::R32Float {
                log(&format!("frame {}: R32F target {surface:#x} {}x{}", self.frame, d[6], d[7]));
            }
            if DEBUG_RT.load(std::sync::atomic::Ordering::Relaxed) && d[6] == 512 {
                log(&format!("frame {}: our copy of {surface:#x} made (container {:?})", self.frame, unsafe { container_texture(surface) }.map(|t| format!("{t:#x}"))));
            }
            let opaque = matches!(d[0], 20 | 22 | 23 | 24);
            if opaque {
                // alpha 1 from the start
                let mut enc = self.device.create_command_encoder(&Default::default());
                let _ = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                    label: Some("opaque target"),
                    color_attachments: &[Some(wgpu::RenderPassColorAttachment { view: &view, depth_slice: None, resolve_target: None, ops: wgpu::Operations { load: wgpu::LoadOp::Clear(wgpu::Color::BLACK), store: wgpu::StoreOp::Store } })],
                    depth_stencil_attachment: None,
                    timestamp_writes: None,
                    occlusion_query_set: None,
                    multiview_mask: None,
                });
                self.queue.submit([enc.finish()]);
            }
            self.rts.insert(surface, Rt { texture, view, format, width: d[6].max(1), height: d[7].max(1), rendered: false, opaque });
        }
        self.rts.get(&surface)
    }

    fn depth(&mut self, surface: u32) -> Option<(wgpu::TextureView, wgpu::TextureFormat, u32, u32)> {
        if let Some(d) = self.depths.get(&surface) {
            return Some(d.clone());
        }
        let d = unsafe { surface_desc(surface) }?;
        let Some(format) = depth_format(d[0]) else {
            self.report(format!("depth format {} not handled", d[0]));
            return None;
        };
        let texture = self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some("game depth"),
            size: wgpu::Extent3d { width: d[6].max(1), height: d[7].max(1), depth_or_array_layers: 1 },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format,
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::TEXTURE_BINDING,
            view_formats: &[],
        });
        let sample_view = texture.create_view(&wgpu::TextureViewDescriptor { aspect: wgpu::TextureAspect::DepthOnly, ..Default::default() });
        self.depth_samples.insert(surface, sample_view);
        let entry = (texture.create_view(&Default::default()), format, d[6].max(1), d[7].max(1));
        self.hold_surface(surface);
        self.depths.insert(surface, entry.clone());
        Some(entry)
    }

    /// A render-target texture's level 0 surface (so sampling it reads our copy).
    fn surface_of(&mut self, tex: u32) -> Option<u32> {
        if let Some(s) = self.tex_surface.get(&tex) {
            return *s;
        }
        type GetType = unsafe extern "system" fn(usize) -> u32;
        type GetLevelDesc = unsafe extern "system" fn(usize, u32, *mut u32) -> i32;
        type GetSurfaceLevel = unsafe extern "system" fn(usize, u32, *mut usize) -> i32;
        let s = unsafe {
            (|| {
                if std::mem::transmute::<usize, GetType>(method(tex, 10))(tex as usize) != 3 {
                    return None;
                }
                let mut desc = [0u32; 8];
                std::mem::transmute::<usize, GetLevelDesc>(method(tex, 17))(tex as usize, 0, desc.as_mut_ptr());
                if desc[2] & 1 == 0 {
                    return None; // not a render target
                }
                let mut surface = 0usize;
                std::mem::transmute::<usize, GetSurfaceLevel>(method(tex, 18))(tex as usize, 0, &mut surface);
                let surface = crate::win::obj32(surface);
                release(surface);
                (surface != 0).then_some(surface)
            })()
        };
        self.tex_surface.insert(tex, s);
        s
    }

    /// The view to sample for a game texture: our render target, the game's texture
    /// uploaded, or white.
    fn texture_view(&mut self, tex: u32, cube: bool) -> wgpu::TextureView {
        let t = std::time::Instant::now();
        let r = self.texture_view_inner(tex, cube);
        let r = self.customise(tex, cube, r);
        let dt = t.elapsed().as_secs_f64();
        self.perf[5] += dt;
        self.frame_tex.0 += dt;
        r
    }

    fn texture_view_inner(&mut self, tex: u32, cube: bool) -> wgpu::TextureView {
        let white = if cube { self.white_cube.clone() } else { self.white_2d.clone() };
        if tex == 0 {
            return white;
        }
        if !cube && let Some(surface) = self.surface_of(tex) {
            let needs_readback = self.rt(surface).is_some_and(|r| !r.rendered);
            if needs_readback {
                // drawn before the replay started (e.g. Jimmy's outfit): copy it once
                let data = unsafe { d3d::texture(tex) };
                let frame = self.frame;
                self.report(format!("frame {frame}: readback {tex:#x} (surface {surface:#x}): {:?}", data.as_ref().map(|d| (d.width, d.height, d.format))));
                if let Some(data) = data {
                    let r = &self.rts[&surface];
                    if data.width == r.width && data.height == r.height && r.format == wgpu::TextureFormat::Bgra8Unorm {
                        let bgra: Vec<u8> = data.levels[0].chunks_exact(4).flat_map(|p| [p[2], p[1], p[0], p[3]]).collect();
                        self.queue.write_texture(
                            wgpu::TexelCopyTextureInfo { texture: &r.texture, mip_level: 0, origin: wgpu::Origin3d::ZERO, aspect: wgpu::TextureAspect::All },
                            &bgra,
                            wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(r.width * 4), rows_per_image: Some(r.height) },
                            wgpu::Extent3d { width: r.width, height: r.height, depth_or_array_layers: 1 },
                        );
                    }
                }
                if let Some(r) = self.rts.get_mut(&surface) {
                    r.rendered = true;
                }
            }
            return self.rts.get(&surface).map_or(white, |r| r.view.clone());
        }
        let frame = self.frame;
        // a 2D texture where the shader samples a cube (or the other way round): a
        // mismatched view would make the whole frame's commands invalid (black)
        if let Some(t) = self.textures.get(&tex)
            && t.cube != cube
        {
            self.report(format!("texture {tex:#x} is {} but sampled as {}: drawn plain", if t.cube { "a cube" } else { "2D" }, if cube { "a cube" } else { "2D" }));
            return if cube { self.black_cube.clone() } else { white };
        }
        // an unreadable one is tried again soon: a Lock that failed because the game had
        // the texture locked on its thread (filling or re-baking a managed texture, e.g.
        // a ped's) must not leave it white for as long as it lives
        if let Some(t) = self.textures.get(&tex)
            && frame - t.checked < if t.view.is_some() { 120 } else { 30 }
        {
            return t.view.clone().unwrap_or(white);
        }
        // first sight: a texture of the other kind than the shader samples gets the
        // stand-in (and isn't cached as that kind)
        if !self.textures.contains_key(&tex) && unsafe { d3d::is_cube(tex) } != cube {
            self.report(format!("texture {tex:#x} is {} but sampled as {}: drawn plain", if cube { "2D" } else { "a cube" }, if cube { "a cube" } else { "2D" }));
            return if cube { self.black_cube.clone() } else { white };
        }
        // unchanged since we read it? (a fingerprint from the locked memory: reading
        // the whole texture again every couple of seconds costs a lot of memory churn
        // in a 32-bit process full of big mod textures)
        let raw_sig = if cube { None } else { unsafe { d3d::texture_signature(tex, signature) } };
        self.frame_tex.3 += 1;
        if let (Some(s), Some(t)) = (raw_sig, self.textures.get_mut(&tex))
            && t.sig == s
        {
            t.checked = frame;
            return t.view.clone().unwrap_or(white);
        }
        let data = if cube { unsafe { d3d::cube_texture(tex) } } else { unsafe { d3d::texture(tex) } };
        let Some(data) = data else {
            self.hold(tex);
            self.textures.insert(tex, GameTexture { view: None, cube, sig: 0, checked: frame });
            return white;
        };
        let sig = raw_sig.unwrap_or_else(|| signature(&data.levels[0]) ^ ((data.width as u64) << 32 | data.height as u64) ^ data.format as u64);
        if let Some(t) = self.textures.get_mut(&tex)
            && t.sig == sig
        {
            t.checked = frame;
            return t.view.clone().unwrap_or(white);
        }
        let format = match data.format {
            d3d::FMT_DXT1 => wgpu::TextureFormat::Bc1RgbaUnorm,
            d3d::FMT_DXT3 => wgpu::TextureFormat::Bc2RgbaUnorm,
            d3d::FMT_DXT5 => wgpu::TextureFormat::Bc3RgbaUnorm,
            _ => wgpu::TextureFormat::Rgba8Unorm,
        };
        let layers = if cube { 6 } else { 1 };
        let texture = self.device.create_texture_with_data(
            &self.queue,
            &wgpu::TextureDescriptor {
                label: Some("game texture"),
                size: wgpu::Extent3d { width: data.width, height: data.height, depth_or_array_layers: layers },
                mip_level_count: data.levels.len() as u32 / layers,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format,
                usage: wgpu::TextureUsages::TEXTURE_BINDING,
                view_formats: &[],
            },
            wgpu::util::TextureDataOrder::LayerMajor,
            &{
                self.frame_tex.1 += 1;
                self.frame_tex.2 += data.levels.iter().map(Vec::len).sum::<usize>();
                data.levels.concat()
            },
        );
        let view = texture.create_view(&wgpu::TextureViewDescriptor {
            dimension: Some(if cube { wgpu::TextureViewDimension::Cube } else { wgpu::TextureViewDimension::D2 }),
            ..Default::default()
        });
        self.hold(tex);
        self.textures.insert(tex, GameTexture { view: Some(view.clone()), cube, sig, checked: frame });
        view
    }

    fn sampler(&mut self, states: &[u32; capture::SAMPLER_STATES]) -> wgpu::Sampler {
        // ADDRESSU/V/W, MAGFILTER, MINFILTER, MIPFILTER, MAXMIPLEVEL, MAXANISOTROPY
        let key = SamplerKey([states[1], states[2], states[3], states[5], states[6], states[7], states[9], states[10]]);
        if let Some(s) = self.samplers.get(&key) {
            return s.clone();
        }
        let filter = |f: u32| if f >= 2 { wgpu::FilterMode::Linear } else { wgpu::FilterMode::Nearest };
        let aniso = if states[6] == 3 || states[5] == 3 { states[10].clamp(1, 16) as u16 } else { 1 };
        let linear_all = states[5] >= 2 && states[6] >= 2 && states[7] == 2;
        let s = self.device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("game sampler"),
            address_mode_u: address(states[1]),
            address_mode_v: address(states[2]),
            address_mode_w: address(states[3]),
            mag_filter: filter(states[5]),
            min_filter: filter(states[6]),
            mipmap_filter: if states[7] == 2 { wgpu::MipmapFilterMode::Linear } else { wgpu::MipmapFilterMode::Nearest },
            lod_min_clamp: states[9] as f32,
            lod_max_clamp: if states[7] == 0 { states[9] as f32 } else { 32.0 },
            anisotropy_clamp: if linear_all { aniso.max(self.custom.aniso) } else { 1 },
            ..Default::default()
        });
        self.samplers.insert(key, s.clone());
        s
    }

    fn layouts(&mut self, sig: &SamplerSig) -> (wgpu::BindGroupLayout, wgpu::PipelineLayout) {
        if let Some(l) = self.texture_layouts.get(sig) {
            return l.clone();
        }
        let mut entries = Vec::new();
        for &(slot, ty) in sig {
            entries.push(wgpu::BindGroupLayoutEntry {
                binding: 2 * slot,
                visibility: wgpu::ShaderStages::VERTEX | wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Texture {
                    sample_type: wgpu::TextureSampleType::Float { filterable: true },
                    view_dimension: if ty == 3 { wgpu::TextureViewDimension::Cube } else { wgpu::TextureViewDimension::D2 },
                    multisampled: false,
                },
                count: None,
            });
            entries.push(wgpu::BindGroupLayoutEntry {
                binding: 2 * slot + 1,
                visibility: wgpu::ShaderStages::VERTEX | wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                count: None,
            });
        }
        let group = self.device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor { label: Some("textures"), entries: &entries });
        let pipeline = self.device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor { label: None, bind_group_layouts: &[Some(&self.uniform_layout), Some(&group)], immediate_size: 0 });
        self.texture_layouts.insert(sig.clone(), (group.clone(), pipeline.clone()));
        (group, pipeline)
    }

    fn pipeline(&mut self, key: &PipelineKey, vs: wgpu::ShaderModule, ps: wgpu::ShaderModule, layout: &wgpu::PipelineLayout) -> Option<wgpu::RenderPipeline> {
        if let Some(p) = self.pipelines.get(key) {
            return p.clone();
        }
        let buffers: Vec<Option<wgpu::VertexBufferLayout>> = key
            .layout
            .streams
            .iter()
            .map(|(_, stride, _, attrs)| {
                let attributes: Vec<wgpu::VertexAttribute> = attrs.iter().map(|&(format, offset, location)| wgpu::VertexAttribute { format, offset, shader_location: location }).collect();
                Some(wgpu::VertexBufferLayout { array_stride: *stride as u64, step_mode: wgpu::VertexStepMode::Vertex, attributes: Box::leak(attributes.into_boxed_slice()) })
            })
            .collect();
        let targets: Vec<Option<wgpu::ColorTargetState>> = key
            .colors
            .iter()
            .map(|&(format, blend, mask)| {
                let blend = if format.is_srgb() || matches!(format, wgpu::TextureFormat::R32Float | wgpu::TextureFormat::Rg32Float | wgpu::TextureFormat::Rgba32Float) { None } else { blend };
                Some(wgpu::ColorTargetState { format, blend, write_mask: wgpu::ColorWrites::from_bits_truncate(mask) })
            })
            .collect();
        let depth_stencil = key.depth.map(|(format, write, cmp, stencil, bias, slope)| wgpu::DepthStencilState {
            format,
            depth_write_enabled: Some(write),
            depth_compare: Some(cmp),
            stencil: match stencil {
                Some(s) if has_stencil(format) => {
                    let face = wgpu::StencilFaceState { compare: compare(s.func), fail_op: stencil_op(s.fail), depth_fail_op: stencil_op(s.zfail), pass_op: stencil_op(s.pass) };
                    wgpu::StencilState { front: face, back: face, read_mask: s.mask, write_mask: s.write }
                }
                _ => Default::default(),
            },
            bias: wgpu::DepthBiasState { constant: bias, slope_scale: f32::from_bits(slope), clamp: 0.0 },
        });
        let scope = self.device.push_error_scope(wgpu::ErrorFilter::Validation);
        let p = self.device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("game draw"),
            layout: Some(layout),
            vertex: wgpu::VertexState { module: &vs, entry_point: Some("main"), compilation_options: Default::default(), buffers: &buffers },
            primitive: wgpu::PrimitiveState {
                topology: key.topology,
                strip_index_format: key.strip_index,
                front_face: wgpu::FrontFace::Cw,
                cull_mode: key.cull,
                ..Default::default()
            },
            depth_stencil,
            multisample: Default::default(),
            fragment: Some(wgpu::FragmentState { module: &ps, entry_point: Some("main"), compilation_options: Default::default(), targets: &targets }),
            multiview_mask: None,
            cache: None,
        });
        let err = pollster::block_on(scope.pop());
        let p = match err {
            Some(e) => {
                self.report(format!("pipeline vs {:016x} ps {:016x}: {e}", key.vs, key.ps));
                None
            }
            None => Some(p),
        };
        self.pipelines.insert(key.clone(), p.clone());
        p
    }

    fn blit_pipeline(&mut self, format: wgpu::TextureFormat, alpha: bool) -> wgpu::RenderPipeline {
        if let Some(p) = self.blit_pipelines.get(&(format, alpha)) {
            return p.clone();
        }
        let module = self.device.create_shader_module(wgpu::ShaderModuleDescriptor { label: Some("blit"), source: wgpu::ShaderSource::Wgsl(BLIT_SHADER.into()) });
        let layout = self.device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor { label: None, bind_group_layouts: &[Some(&self.blit_layout)], immediate_size: 0 });
        let p = self.device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("blit"),
            layout: Some(&layout),
            vertex: wgpu::VertexState { module: &module, entry_point: Some("vs"), compilation_options: Default::default(), buffers: &[] },
            primitive: wgpu::PrimitiveState { topology: wgpu::PrimitiveTopology::TriangleStrip, ..Default::default() },
            depth_stencil: None,
            multisample: Default::default(),
            fragment: Some(wgpu::FragmentState {
                module: &module,
                entry_point: Some("fs"),
                compilation_options: Default::default(),
                targets: &[Some(wgpu::ColorTargetState { format, blend: None, write_mask: if alpha { wgpu::ColorWrites::ALL } else { wgpu::ColorWrites::COLOR } })],
            }),
            multiview_mask: None,
            cache: None,
        });
        self.blit_pipelines.insert((format, alpha), p.clone());
        p
    }

    /// The back buffer to the window, through the picture settings.
    /// The back buffer through the post passes: the image to present and its bloom.
    fn post_frame(&mut self, enc: &mut wgpu::CommandEncoder, backbuffer: u32) -> Option<(wgpu::TextureView, wgpu::TextureView)> {
        let r = self.rts.get(&backbuffer)?;
        let (view, size, format) = (r.view.clone(), (r.width, r.height), r.format);
        let c = &self.custom;
        let (aa, bloom, threshold) = (c.aa, c.bloom, c.bloom_threshold);
        Some(self.post.run(&self.device, enc, &view, size, format, aa, bloom, threshold))
    }

    fn present_pass(&mut self, pass: &mut wgpu::RenderPass, src: &wgpu::TextureView, bloom: &wgpu::TextureView, format: wgpu::TextureFormat) {
        if !self.present_pipelines.contains_key(&format) {
            let module = self.device.create_shader_module(wgpu::ShaderModuleDescriptor { label: Some("present"), source: wgpu::ShaderSource::Wgsl(PRESENT_SHADER.into()) });
            let layout = self.device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor { label: None, bind_group_layouts: &[Some(&self.present_layout)], immediate_size: 0 });
            let p = self.device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("present"),
                layout: Some(&layout),
                vertex: wgpu::VertexState { module: &module, entry_point: Some("vs"), compilation_options: Default::default(), buffers: &[] },
                primitive: wgpu::PrimitiveState { topology: wgpu::PrimitiveTopology::TriangleStrip, ..Default::default() },
                depth_stencil: None,
                multisample: Default::default(),
                fragment: Some(wgpu::FragmentState { module: &module, entry_point: Some("fs"), compilation_options: Default::default(), targets: &[Some(format.into())] }),
                multiview_mask: None,
                cache: None,
            });
            self.present_pipelines.insert(format, p);
        }
        let c = &self.custom;
        #[rustfmt::skip]
        let params: [f32; 16] = [
            0.0, 0.0, 1.0, 1.0,
            c.exposure, c.contrast, c.saturation, c.gamma,
            c.tint[0], c.tint[1], c.tint[2], c.tint_strength,
            // (the bloom's levels are added up: a fifth each)
            c.vignette, c.sharpen, c.bloom * 0.2, 0.0,
        ];
        let buffer = self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("present params"), contents: &params.map(f32::to_le_bytes).concat(), usage: wgpu::BufferUsages::UNIFORM });
        let bind = self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: None,
            layout: &self.present_layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::TextureView(src) },
                wgpu::BindGroupEntry { binding: 1, resource: buffer.as_entire_binding() },
                wgpu::BindGroupEntry { binding: 2, resource: wgpu::BindingResource::TextureView(bloom) },
                wgpu::BindGroupEntry { binding: 3, resource: wgpu::BindingResource::Sampler(&self.post.sampler) },
            ],
        });
        pass.set_pipeline(&self.present_pipelines[&format]);
        pass.set_bind_group(0, &bind, &[]);
        pass.draw(0..4, 0..1);
    }

    // ---- the customiser ----------------------------------------------------------

    /// New settings from the menu. `first`: the ones there at start (counters are
    /// only remembered, not acted on).
    pub fn configure(&mut self, s: &Settings, first: bool) {
        let old = std::mem::replace(&mut self.custom, s.clone());
        if first {
            return;
        }
        if s.textures != old.textures || s.reload != old.reload {
            self.replaced.clear();
            self.bind_groups.clear();
            if s.reload != old.reload {
                log("replacement textures reloaded");
            }
        }
        if s.export != old.export {
            self.export_textures();
        }
        if s.aniso != old.aniso {
            self.samplers.clear();
            self.bind_groups.clear();
        }
        if s.dry_seq != old.dry_seq || s.demo_seq != old.demo_seq {
            self.wetness = 0.0;
        }
    }

    /// Every game texture seen so far, as PNGs in bullygfx_textures/export.
    fn export_textures(&mut self) {
        let list: Vec<(u32, u64)> = self.textures.iter().filter(|(_, t)| !t.cube && t.view.is_some()).map(|(&k, t)| (k, t.sig)).collect();
        let mut out = Vec::new();
        for (tex, sig) in list {
            if self.surface_of(tex).is_some() {
                continue; // drawn by the game, not a file
            }
            if let Some(data) = unsafe { d3d::texture(tex) } {
                out.push((textures::name_of(tex).unwrap_or_else(|| textures::unnamed(sig)), data));
            }
        }
        textures::export(out, settings::textures_dir().join("export"));
    }

    fn upload_rgba(&self, img: &textures::Image, label: &str) -> wgpu::TextureView {
        let texture = self.device.create_texture_with_data(
            &self.queue,
            &wgpu::TextureDescriptor {
                label: Some(label),
                size: wgpu::Extent3d { width: img.width, height: img.height, depth_or_array_layers: 1 },
                mip_level_count: img.levels.len() as u32,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8Unorm,
                usage: wgpu::TextureUsages::TEXTURE_BINDING,
                view_formats: &[],
            },
            wgpu::util::TextureDataOrder::LayerMajor,
            &img.levels.concat(),
        );
        texture.create_view(&Default::default())
    }

    /// The replacement for a game texture, from bullygfx_textures/replace/<name>.png.
    fn replacement(&mut self, tex: u32) -> Option<wgpu::TextureView> {
        // a miss without a name is tried again now and then: the name may come (a mesh
        // naming it), or the content (streamed in) and with it the unnamed_ hash
        if let Some((v, named, when)) = self.replaced.get(&tex)
            && (v.is_some() || *named || self.frame - when < 120)
        {
            return v.clone();
        }
        let named = textures::name_of(tex);
        let is_named = named.is_some();
        let name = named.or_else(|| self.textures.get(&tex).filter(|t| t.view.is_some()).map(|t| textures::unnamed(t.sig)))?;
        let path = settings::textures_dir().join("replace").join(format!("{name}.png"));
        let view = if path.exists() {
            match textures::load_png(&path) {
                Ok(img) => Some(self.upload_rgba(&img, "replacement")),
                Err(e) => {
                    self.report(format!("{}: {e}", path.display()));
                    None
                }
            }
        } else {
            None
        };
        self.replaced.insert(tex, (view.clone(), is_named, self.frame));
        view
    }

    /// What the texture mode makes of a game texture's view.
    fn customise(&mut self, tex: u32, cube: bool, view: wgpu::TextureView) -> wgpu::TextureView {
        if tex == 0 || cube || self.custom.textures == TextureMode::Game || self.surface_of(tex).is_some() {
            return view;
        }
        match self.custom.textures {
            TextureMode::White => self.white_2d.clone(),
            TextureMode::Checker => {
                if self.checker.is_none() {
                    self.checker = Some(self.upload_rgba(&textures::checker(), "checker"));
                }
                self.checker.clone().unwrap()
            }
            TextureMode::Replaced => self.replacement(tex).unwrap_or(view),
            TextureMode::Game => view,
        }
    }

    /// The wet streets pass, when it's on (and raining, in auto).
    fn wet_streets(&mut self, enc: &mut wgpu::CommandEncoder, backbuffer: u32, ds: u32, cam: &capture::Camera) {
        const WEATHER: usize = 0x00CE_756C;
        const RAIN: i16 = 2;
        let raining = unsafe { *(WEATHER as *const i16) } == RAIN;
        let on = match self.custom.wet {
            0 => false,
            2 => true,
            _ => raining,
        };
        // wetness builds up while it rains (about half a minute to full puddles) and dries
        // off after (about a minute and a half); "always" is simply wet
        let now = std::time::Instant::now();
        let dt = self.wet_clock.map_or(0.0, |t| now.duration_since(t).as_secs_f32()).min(0.5);
        self.wet_clock = Some(now);
        self.wetness = match self.custom.wet {
            2 => 1.0,
            _ if on => (self.wetness + dt / 30.0).min(1.0),
            _ => (self.wetness - dt / 90.0).max(0.0),
        };
        let on = self.wetness > 0.001 && self.custom.wet != 0;
        static SAID: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(99);
        let state = on as u32 * 2 + raining as u32 + self.custom.wet as u32 * 4;
        if SAID.swap(state, std::sync::atomic::Ordering::Relaxed) != state {
            log(&format!("wet: mode {} on {on} raining {raining} camera {cam:?} ds {ds:#x} depth {:?} rt {:?}", self.custom.wet, self.depths.get(&ds).map(|d| (d.2, d.3)), self.rts.get(&backbuffer).map(|r| (r.width, r.height))));
        }
        // indoors (any area but the outside world, 0) nothing is wet: the wetness
        // carries on, for when the player walks back out
        let indoors = crate::win::read_u32(0x00BD_1008).is_some_and(|area| area != 0);
        if !on || indoors || self.custom.wet_amount <= 0.0 {
            return;
        }
        let (Some(rt), Some(depth)) = (self.rts.get(&backbuffer), self.depth_samples.get(&ds)) else { return };
        let d = self.depths.get(&ds).map(|d| (d.2, d.3));
        if d != Some((rt.width, rt.height)) {
            return;
        }
        let (texture, view, w, h, format) = (rt.texture.clone(), rt.view.clone(), rt.width, rt.height, rt.format);
        let depth = depth.clone();
        let strength = self.custom.wet_amount;
        let level = self.wetness;
        self.wet.pass(&self.device, enc, (&texture, &view, w, h, format), &depth, cam, strength, level);
    }

    pub fn wetness(&self) -> f32 {
        self.wetness
    }

    pub fn held_count(&self) -> usize {
        self.held.len()
    }

    pub fn frame_ms(&self) -> f32 {
        self.last_ms
    }

    pub fn replaced_count(&self) -> usize {
        self.replaced.values().filter(|v| v.0.is_some()).count()
    }

    /// Draws `src` (uv rect in 0..1) into the target area of `pass`.
    fn blit(&mut self, pass: &mut wgpu::RenderPass, src: &wgpu::TextureView, uv: [f32; 4], format: wgpu::TextureFormat) {
        self.blit_masked(pass, src, uv, format, true);
    }

    /// `alpha`: whether the copy writes alpha.
    fn blit_masked(&mut self, pass: &mut wgpu::RenderPass, src: &wgpu::TextureView, uv: [f32; 4], format: wgpu::TextureFormat, alpha: bool) {
        let pipeline = self.blit_pipeline(format, alpha);
        let rect = self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("blit rect"), contents: &uv.map(f32::to_le_bytes).concat(), usage: wgpu::BufferUsages::UNIFORM });
        let bind = self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: None,
            layout: &self.blit_layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::TextureView(src) },
                wgpu::BindGroupEntry { binding: 1, resource: wgpu::BindingResource::Sampler(&self.blit_sampler) },
                wgpu::BindGroupEntry { binding: 2, resource: rect.as_entire_binding() },
            ],
        });
        pass.set_pipeline(&pipeline);
        pass.set_bind_group(0, &bind, &[]);
        pass.draw(0..4, 0..1);
    }

    // ---- a draw ------------------------------------------------------------------

    /// Everything a draw needs, resolved; None if something in it can't be done yet.
    fn prepare(&mut self, d: &Draw, constants: [u32; 3]) -> Result<Prepared, String> {
        let t_layout = std::time::Instant::now();
        let vs = self.shader(d.vs).ok_or("vertex shader")?;
        let ps = self.shader(d.ps).ok_or("pixel shader")?;
        // vertex layout: the declaration's elements the vertex shader reads
        let elements = self.decl(d.decl);
        let mut streams: BTreeMap<u32, Vec<(wgpu::VertexFormat, u64, u32)>> = BTreeMap::new();
        let mut inputs = Vec::new();
        for (&reg, &(usage, index)) in &vs.shader.inputs {
            let Some(e) = elements.iter().find(|e| e.usage as u32 == usage && e.index as u32 == index) else { continue };
            let Some((format, input)) = vertex_format(e.ty) else {
                return Err(format!("vertex type {}", e.ty));
            };
            streams.entry(e.stream as u32).or_default().push((format, e.offset as u64, reg));
            inputs.push((reg, input));
        }
        let mut slots = Vec::new();
        for (s, attrs) in streams {
            let stride = d.streams[s as usize].stride;
            let (fit, over): (Vec<_>, Vec<_>) = attrs.into_iter().partition(|&(f, offset, _)| stride == 0 || offset + f.size() <= stride as u64);
            if !fit.is_empty() {
                slots.push((s, stride, 0, fit));
            }
            for (f, offset, location) in over {
                slots.push((s, stride, offset, vec![(f, 0, location)]));
            }
        }
        let layout = VertexLayout { streams: slots, inputs: inputs.clone() };
        let vs_module = self.module(&vs, &inputs).ok_or("vertex shader translation")?;
        let ps_module = self.module(&ps, &[]).ok_or("pixel shader translation")?;
        self.perf[10] += t_layout.elapsed().as_secs_f64();

        // targets
        let mut colors = Vec::new();
        let mut size = None;
        for (i, &rt) in d.rt.iter().enumerate() {
            if rt == 0 {
                continue;
            }
            let r = self.rt(rt).ok_or("render target")?;
            size.get_or_insert((r.width, r.height));
            let st = &d.states;
            let blend = (rs(st, 27) != 0).then(|| {
                let color = wgpu::BlendComponent { src_factor: blend_factor(rs(st, 19)), dst_factor: blend_factor(rs(st, 20)), operation: blend_op(rs(st, 171)) };
                let alpha = if rs(st, 206) != 0 {
                    wgpu::BlendComponent { src_factor: blend_factor(rs(st, 207)), dst_factor: blend_factor(rs(st, 208)), operation: blend_op(rs(st, 209)) }
                } else {
                    color
                };
                wgpu::BlendState { color, alpha }
            });
            let mask = if i == 0 { rs(st, 168) & 0xF } else { 0xF };
            let mask = if r.opaque { mask & !8 } else { mask };
            colors.push((r.format, blend, mask));
        }
        let (tw, th) = size.ok_or("no render target")?;
        let st = &d.states;
        // (the seamless doors' draws keep to their stencil even without the depth test: the sky)
        let portal_draw = capture::is_portal_draw(rs(st, 196));
        let z_on = rs(st, 7) != 0;
        let depth = if d.ds != 0 && (z_on || portal_draw) {
            let (view, format, w, h) = self.depth(d.ds).ok_or("depth surface")?;
            if (w, h) != (tw, th) {
                // D3D9 allows a bigger depth buffer than the target; wgpu doesn't
                None
            } else {
                let stencil = if rs(st, 196) == capture::PORTAL_DRAWS {
                    // (another area through a doorway: only where it was marked, portal.rs)
                    Some(StencilKey { func: 3, fail: 1, zfail: 1, pass: 1, mask: 0xFF, write: 0 })
                } else if rs(st, 196) == capture::PORTAL_AROUND_DRAWS {
                    // (around a doorway and in front of it: 1 <= the mark, portal.rs's PORTAL_AROUND)
                    Some(StencilKey { func: 4, fail: 1, zfail: 1, pass: 1, mask: 0xFF, write: 0 })
                } else {
                    (rs(st, 52) != 0).then(|| StencilKey { func: rs(st, 56), fail: rs(st, 53), zfail: rs(st, 54), pass: rs(st, 55), mask: rs(st, 58), write: rs(st, 59) })
                };
                Some((view, format, stencil))
            }
        } else {
            None
        };
        let key = PipelineKey {
            vs: vs.hash,
            ps: ps.hash,
            layout: layout.clone(),
            topology: match d.prim {
                1 => wgpu::PrimitiveTopology::PointList,
                2 => wgpu::PrimitiveTopology::LineList,
                3 => wgpu::PrimitiveTopology::LineStrip,
                5 => wgpu::PrimitiveTopology::TriangleStrip,
                _ => wgpu::PrimitiveTopology::TriangleList, // lists, and fans made into lists
            },
            strip_index: None,
            cull: if NO_CULL.load(std::sync::atomic::Ordering::Relaxed) { None } else { match rs(st, 22) {
                2 => Some(wgpu::Face::Front), // D3DCULL_CW culls clockwise; front faces are clockwise
                3 => Some(wgpu::Face::Back),  // D3DCULL_CCW
                _ => None,
            } },
            colors,
            depth: depth.as_ref().map(|(_, format, stencil)| (*format, z_on && rs(st, 14) != 0, compare(if z_on { rs(st, 23) } else { 8 }), *stencil, (f32::from_bits(rs(st, 195)) * 16_777_215.0) as i32, rs(st, 175))),
        };

        // textures: what either shader samples
        let mut sig: SamplerSig = vs.shader.samplers.iter().chain(ps.shader.samplers.iter()).map(|(&s, &t)| (s, t)).collect();
        sig.sort();
        sig.dedup_by_key(|x| x.0);
        let (group_layout, pipeline_layout) = self.layouts(&sig);

        // indices (fans become lists; strips get their index format)
        let mut key = key;
        let (index, fan_indices) = self.indices(d, &mut key)?;
        let t = std::time::Instant::now();
        let pipeline = self.pipeline(&key, vs_module, ps_module, &pipeline_layout).ok_or("pipeline")?;
        self.perf[7] += t.elapsed().as_secs_f64();

        let mut entries_views = Vec::new();
        for &(slot, ty) in &sig {
            let tex = d.textures.get(slot as usize).copied().unwrap_or(0);
            let view = self.texture_view(tex, ty == 3);
            if DEBUG_RT.load(std::sync::atomic::Ordering::Relaxed) && tex != 0 && ty != 3 {
                type GetLevelDesc = unsafe extern "system" fn(usize, u32, *mut u32) -> i32;
                let mut desc = [0u32; 8];
                unsafe { std::mem::transmute::<usize, GetLevelDesc>(method(tex, 17))(tex as usize, 0, desc.as_mut_ptr()) };
                if desc[6] == 512 && desc[7] == 512 {
                    let rt = self.surface_of(tex);
                    let cached = self.textures.get(&tex).map(|t| (t.view.is_some(), t.sig, t.checked));
                    let frame = self.frame;
                    let rendered = rt.and_then(|s| self.rts.get(&s)).map(|r| r.rendered);
                    self.report(format!("frame {}: s{slot} tex {tex:#x} fmt {:#x} usage {:#x} pool {} rt {rt:x?} rendered {rendered:?} cached {cached:x?}", frame / 100 * 100, desc[0], desc[2], desc[3]));
                }
            }
            if SAMPLER_REPORT.load(std::sync::atomic::Ordering::Relaxed) {
                self.report(format!("sampler s{slot} states {:?} alpha test {} func {} ref {} blend {}", &d.samplers[slot as usize], rs(&d.states, 15), rs(&d.states, 25), rs(&d.states, 24), rs(&d.states, 27)));
            }
            let sampler = self.sampler(&d.samplers[slot as usize]);
            entries_views.push((slot, view, sampler));
        }
        let entries: Vec<wgpu::BindGroupEntry> = entries_views
            .iter()
            .flat_map(|(slot, view, sampler)| {
                [
                    wgpu::BindGroupEntry { binding: 2 * slot, resource: wgpu::BindingResource::TextureView(view) },
                    wgpu::BindGroupEntry { binding: 2 * slot + 1, resource: wgpu::BindingResource::Sampler(sampler) },
                ]
            })
            .collect();
        let t = std::time::Instant::now();
        let bind_key = (sig.clone(), entries_views.clone());
        let textures = match self.bind_groups.get(&bind_key) {
            Some(g) => g.clone(),
            None => {
                let g = self.device.create_bind_group(&wgpu::BindGroupDescriptor { label: None, layout: &group_layout, entries: &entries });
                if self.bind_groups.len() > 16384 {
                    self.bind_groups.clear();
                }
                self.bind_groups.insert(bind_key, g.clone());
                g
            }
        };
        self.perf[6] += t.elapsed().as_secs_f64();

        // vertex buffers
        let mut vertex_buffers = Vec::new();
        for (slot, (stream, _, base, _)) in layout.streams.iter().enumerate() {
            let s = d.streams[*stream as usize];
            let buffer = match &d.up_vertices {
                Some(bytes) if *stream == 0 => {
                    let mut b = bytes.to_vec();
                    b.resize(b.len().div_ceil(4) * 4 + VERTEX_PAD, 0);
                    self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("up vertices"), contents: &b, usage: wgpu::BufferUsages::VERTEX })
                }
                _ => match &d.vb_data[*stream as usize] {
                    Some(bytes) => {
                        let mut b = bytes.to_vec();
                        b.resize(b.len().div_ceil(4) * 4 + VERTEX_PAD, 0);
                        self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("dynamic vertices"), contents: &b, usage: wgpu::BufferUsages::VERTEX })
                    }
                    None => self.static_buffer(s.vb, false).ok_or("vertex buffer")?.0,
                },
            };
            vertex_buffers.push((slot as u32, buffer, s.offset as u64 + base));
        }
        let _ = fan_indices;
        Ok(Prepared {
            pipeline,
            textures,
            vertex_buffers,
            index,
            constants,
            color_views: d.rt.iter().filter(|&&r| r != 0).map(|r| self.rts[r].view.clone()).collect(),
            depth_view: depth.map(|(v, _, _)| v),
            target: (d.rt, d.ds),
            size: (tw, th),
        })
    }

    /// The draw's index buffer, as (buffer, 32-bit) and how many to draw; for a fan,
    /// a list made from it.
    fn indices(&mut self, d: &Draw, key: &mut PipelineKey) -> Result<(Option<(wgpu::Buffer, bool)>, ()), String> {
        let fan = d.prim == 6;
        if !d.indexed {
            if fan {
                // 0, i, i + 1 for each triangle
                let idx: Vec<u32> = (0..d.prims).flat_map(|i| [d.start, d.start + i + 1, d.start + i + 2]).collect();
                let b = self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("fan"), contents: &idx.iter().flat_map(|v| v.to_le_bytes()).collect::<Vec<u8>>(), usage: wgpu::BufferUsages::INDEX });
                return Ok((Some((b, true)), ()));
            }
            return Ok((None, ()));
        }
        let (buffer, big) = match &d.up_indices {
            Some((bytes, big)) => {
                let mut b = bytes.to_vec();
                b.resize(b.len().div_ceil(4).max(1) * 4, 0);
                (self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("up indices"), contents: &b, usage: wgpu::BufferUsages::INDEX }), *big)
            }
            None => self.static_buffer(d.ib, true).ok_or("index buffer")?,
        };
        if fan {
            return Err("indexed fans not handled".into());
        }
        if key.topology == wgpu::PrimitiveTopology::TriangleStrip || key.topology == wgpu::PrimitiveTopology::LineStrip {
            key.strip_index = Some(if big { wgpu::IndexFormat::Uint32 } else { wgpu::IndexFormat::Uint16 });
        }
        Ok((Some((buffer, big)), ()))
    }

    // ---- a frame -------------------------------------------------------------------

    /// Replays the frame and shows it; `backbuffer` is the surface the game presents.
    fn dump(&mut self, d: &Draw) {
        log(&format!(
            "draw: indexed {} prim {} base {} min {} n {} start {} prims {} vs {:#x} ps {:#x} decl {:#x} ib {:#x} rt {:x?} ds {:#x}",
            d.indexed, d.prim, d.base_vertex, d.min_index, d.num_vertices, d.start, d.prims, d.vs, d.ps, d.decl, d.ib, d.rt, d.ds
        ));
        log(&format!("  viewport {:?} scissor {:?}", d.viewport, d.scissor));
        log(&format!("  states {:?}", capture::RENDER_STATES.iter().zip(&d.states).collect::<Vec<_>>()));
        log(&format!("  streams {:?}", d.streams));
        log(&format!("  textures {:x?}", d.textures));
        let decl = self.decl(d.decl);
        for e in &decl {
            log(&format!("  element stream {} offset {} type {} usage {} index {}", e.stream, e.offset, e.ty, e.usage, e.index));
        }
        for (i, c) in d.vs_consts.iter().enumerate().take(16) {
            log(&format!("  vc{i} {c:?}"));
        }
        for (i, c) in d.ps_consts.iter().enumerate().take(16) {
            log(&format!("  pc{i} {c:?}"));
        }
        log(&format!("  vertex shader:
{}", d3d::disassemble(d.vs)));
        log(&format!("  pixel shader:
{}", d3d::disassemble(d.ps)));
    }

    /// Logs the frame's structure: its clears, copies and runs of draws, and into what.
    fn describe(&mut self, cmds: &[Cmd], backbuffer: u32) {
        let mut name = |s: u32, me: &mut Self| -> String {
            if s == 0 {
                return "-".into();
            }
            let d = unsafe { surface_desc(s) }.unwrap_or_default();
            format!("{s:#x}{} {}x{} fmt {} usage {:#x}", if s == backbuffer { " (back buffer)" } else { "" }, d[6], d[7], d[0], d[2]) + if me.rts.contains_key(&s) { "" } else { "" }
        };
        log(&format!("frame: {} commands", cmds.len()));
        let mut i = 0;
        while i < cmds.len() {
            match &cmds[i] {
                Cmd::Clear { rt, ds, flags, color, .. } => log(&format!("  clear flags {flags} color {color:#010x} rt {} ds {}", name(rt[0], self), name(*ds, self))),
                Cmd::Stretch { src, dst, .. } => log(&format!("  stretch {} -> {}", name(*src, self), name(*dst, self))),
                Cmd::Fill { surface, color, .. } => log(&format!("  fill {} {color:#010x}", name(*surface, self))),
                Cmd::Camera(c) => log(&format!("  camera at {:?} frustum {:?}", c.eye, c.frustum)),
                Cmd::Portal { corners, ds, mode, .. } => log(&format!("  portal (mode {mode}) {corners:?} ds {}", name(*ds, self))),
                Cmd::Draw(d) => {
                    let (rt, ds) = (d.rt, d.ds);
                    let mut n = 0;
                    while i < cmds.len() && matches!(&cmds[i], Cmd::Draw(x) if x.rt == rt && x.ds == ds) {
                        n += 1;
                        i += 1;
                    }
                    log(&format!("  {n} draws into {} (+{} more targets) ds {} ", name(rt[0], self), rt[1..].iter().filter(|&&r| r != 0).count(), name(ds, self)));
                    continue;
                }
            }
            i += 1;
        }
    }

    pub fn replay(&mut self, cmds: Vec<Cmd>, fixed_function: u32, backbuffer: u32, shot: Option<&std::path::Path>) {
        if DESCRIBE.swap(false, std::sync::atomic::Ordering::Relaxed) {
            self.describe(&cmds, backbuffer);
        }
        // debugging: replay only the first N commands
        let t_start = std::time::Instant::now();
        let (secs, new, bytes, checks) = std::mem::take(&mut self.frame_tex);
        if secs > 0.015 {
            log(&format!("slow textures: {:.0} ms, {new} new ({:.1} MB), {checks} fingerprint checks", secs * 1e3, bytes as f64 / 1048576.0));
        }
        self.perf[3] += 1.0;
        if self.perf[3] >= 300.0 {
            let n = self.perf[3];
            let capture = capture::CAPTURE_NS.swap(0, std::sync::atomic::Ordering::Relaxed) as f64 / 1e6 / n;
            log(&format!(
                "perf ms/frame: capture {capture:.2} replay total {:.2} encode {:.2} submit+present {:.2} | buffers {:.2} textures {:.2} bind groups {:.2} pipelines {:.2}",
                self.perf[0] * 1e3 / n,
                self.perf[1] * 1e3 / n,
                self.perf[2] * 1e3 / n,
                self.perf[4] * 1e3 / n,
                self.perf[5] * 1e3 / n,
                self.perf[6] * 1e3 / n,
                self.perf[7] * 1e3 / n
            ));
            log(&format!(
                "perf ms/frame: uniform copies {:.2}, prepare {:.2} (of it shaders+layout {:.2}), {} draws last frame, {}+{} constant blocks ({:.1} MB)",
                self.perf[8] * 1e3 / n,
                self.perf[9] * 1e3 / n,
                self.perf[10] * 1e3 / n,
                self.last_draws,
                self.unique_constants.0,
                self.unique_constants.1,
                self.unique_constants.2 as f64 / 1048576.0
            ));
            let vb_bytes: u64 = self.vbs.values().map(|b| b.size).sum::<u64>() + self.ibs.values().map(|b| b.0.size).sum::<u64>();
            log(&format!(
                "caches: textures {} rts {} depths {} vbs+ibs {} ({} MB) shaders {} modules {} pipelines {} bind groups {} held {} replaced {}",
                self.textures.len(),
                self.rts.len(),
                self.depths.len(),
                self.vbs.len() + self.ibs.len(),
                vb_bytes >> 20,
                self.shaders.len(),
                self.modules.len(),
                self.pipelines.len(),
                self.bind_groups.len(),
                self.held.len(),
                self.replaced.len()
            ));
            self.perf = [0.0; 11];
        }
        let limit = LIMIT.load(std::sync::atomic::Ordering::Relaxed) as usize;
        let mut cmds = cmds;
        if limit > 0 {
            cmds.truncate(limit);
        }
        self.follow_parent();
        self.frame += 1;
        if self.frame % SWEEP_FRAMES == 0 {
            self.sweep();
        }
        if fixed_function > 0 {
            self.report(format!("{fixed_function} fixed-function draws a frame (not replayed)"));
        }
        let draws = cmds.iter().filter(|c| matches!(c, Cmd::Draw(_))).count() as u64;
        self.last_draws = draws;
        // the draws' constants, each block once: draws with the same constants (the
        // capture shares unchanged ones between draws) point at the same bytes
        let mut uniforms = std::mem::take(&mut self.uniform_bytes);
        uniforms.clear();
        // (only the registers the shader reads, the same bytes once)
        let mut vs_at: HashMap<&[u8], u32> = HashMap::default();
        let mut ps_at: HashMap<&[u8], u32> = HashMap::default();
        let mut frame_at: HashMap<[u32; 8], u32> = HashMap::default();
        let push = |u: &mut Vec<u8>, bytes: &[u8]| -> u32 {
            let at = u.len().next_multiple_of(256);
            u.resize(at, 0);
            u.extend_from_slice(bytes);
            at as u32
        };
        let mut block = 0u64;
        let mut prepared = Vec::with_capacity(cmds.len());
        for c in &cmds {
            match c {
                Cmd::Draw(d) => {
                    let wanted = DUMP_DRAW.load(std::sync::atomic::Ordering::Relaxed);
                    // (u32::MAX: the first draw seen through a doorway, portal.rs)
                    if wanted == block as u32 + 1 || (wanted == u32::MAX && capture::is_portal_draw(rs(&d.states, 196))) {
                        DUMP_DRAW.store(0, std::sync::atomic::Ordering::Relaxed);
                        log(&format!("draw {block} of the frame"));
                        self.dump(d);
                    }
                    let t_u = std::time::Instant::now();
                    let vs_len = self.shader(d.vs).map_or(VC_SIZE as usize, |s| s.const_bytes);
                    let ps_len = self.shader(d.ps).map_or(PC_SIZE as usize, |s| s.const_bytes);
                    let vc: &[u8] = unsafe { std::slice::from_raw_parts(d.vs_consts.as_ptr() as *const u8, vs_len) };
                    let pc: &[u8] = unsafe { std::slice::from_raw_parts(d.ps_consts.as_ptr() as *const u8, ps_len) };
                    let vs_off = *vs_at.entry(vc).or_insert_with(|| push(&mut uniforms, vc));
                    let ps_off = *ps_at.entry(pc).or_insert_with(|| push(&mut uniforms, pc));
                    let vp = d.viewport;
                    let alpha = if rs(&d.states, 15) != 0 { [rs(&d.states, 25) as f32, rs(&d.states, 24) as f32 / 255.0] } else { [0.0, 0.0] };
                    let frame = [1.0 / vp.w.max(1) as f32, 1.0 / vp.h.max(1) as f32, 0.0, 0.0, alpha[0], alpha[1], 0.0, 0.0];
                    let frame_off = *frame_at.entry(frame.map(f32::to_bits)).or_insert_with(|| push(&mut uniforms, &frame.map(f32::to_le_bytes).concat()));
                    self.perf[8] += t_u.elapsed().as_secs_f64();
                    let t_p = std::time::Instant::now();
                    let p = self.prepare(d, [vs_off, ps_off, frame_off]);
                    self.perf[9] += t_p.elapsed().as_secs_f64();
                    if let Err(e) = &p {
                        self.report(format!("draw skipped: {e}"));
                    }
                    prepared.push(p.ok());
                    block += 1;
                }
                _ => prepared.push(None),
            }
        }
        let t_prepared = std::time::Instant::now();
        // room for a whole binding past the last block
        uniforms.resize(uniforms.len().next_multiple_of(256) + VC_SIZE.max(PC_SIZE) as usize, 0);
        let need = uniforms.len() as u64;
        if self.uniforms.as_ref().is_none_or(|u| u.2 < need) {
            let capacity = need + need / 2;
            let buffer = self.device.create_buffer(&wgpu::BufferDescriptor { label: Some("constants"), size: capacity, usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST, mapped_at_creation: false });
            let bind = |size: u64| wgpu::BufferBinding { buffer: &buffer, offset: 0, size: NonZeroU64::new(size) };
            let group = self.device.create_bind_group(&wgpu::BindGroupDescriptor {
                label: Some("constants"),
                layout: &self.uniform_layout,
                entries: &[
                    wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::Buffer(bind(VC_SIZE)) },
                    wgpu::BindGroupEntry { binding: 1, resource: wgpu::BindingResource::Buffer(bind(PC_SIZE)) },
                    wgpu::BindGroupEntry { binding: 2, resource: wgpu::BindingResource::Buffer(bind(FRAME_SIZE)) },
                ],
            });
            self.uniforms = Some((buffer, group, capacity));
        }
        let (uniform_buffer, constants, _) = self.uniforms.clone().expect("made above");
        self.queue.write_buffer(&uniform_buffer, 0, &uniforms);
        self.unique_constants = (vs_at.len(), ps_at.len(), uniforms.len());
        self.uniform_bytes = uniforms;

        let mut enc = self.device.create_command_encoder(&Default::default());
        let mut i = 0;
        while i < cmds.len() {
            match &cmds[i] {
                Cmd::Clear { rt, ds, flags, color, z, stencil, viewport, rects } => {
                    // the world's depth is about to go (the game clears it for close-up
                    // 3D before its post effects): the wet pass needs it, so now
                    if flags & 2 != 0
                        && self.scene.is_some_and(|(_, sds)| sds == *ds)
                        && let Some((cam, sds)) = self.scene.take()
                    {
                        self.wet_streets(&mut enc, backbuffer, sds, &cam);
                    }
                    self.clear(&mut enc, rt, *ds, *flags, *color, *z, *stencil, viewport, rects);
                    i += 1;
                }
                Cmd::Stretch { src, src_rect, dst, dst_rect, .. } => {
                    // the 3D scene is done and about to go through the post effects
                    if *src == backbuffer
                        && let Some((cam, ds)) = self.scene.take()
                    {
                        self.wet_streets(&mut enc, backbuffer, ds, &cam);
                    }
                    self.stretch(&mut enc, *src, *src_rect, *dst, *dst_rect);
                    i += 1;
                }
                Cmd::Camera(c) => {
                    self.camera = Some(*c);
                    i += 1;
                }
                Cmd::Portal { corners, cam, ds, mode } => {
                    // (seen from the camera the scene was drawn with: the game's SetCameraData)
                    let seen = self.camera.unwrap_or(*cam);
                    static COMPARED: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
                    if !COMPARED.swap(true, std::sync::atomic::Ordering::Relaxed) {
                        log(&format!("portal: the game's camera {cam:?}, the scene's {seen:?}"));
                    }
                    match self.depth(*ds) {
                        Some((view, format, ..)) if has_stencil(format) => self.portal.mark(&self.device, &mut enc, &view, format, corners, &seen, *mode),
                        other => {
                            static SAID: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
                            if !SAID.swap(true, std::sync::atomic::Ordering::Relaxed) {
                                log(&format!("portal: depth surface {ds:#x} has no stencil ({:?})", other.map(|o| o.1)));
                            }
                        }
                    }
                    i += 1;
                }
                Cmd::Fill { surface, color, .. } => {
                    self.clear(&mut enc, &[*surface, 0, 0, 0], 0, 1, *color, 1.0, 0, &Viewport::default(), &[]);
                    i += 1;
                }
                Cmd::Draw(_) => {
                    // a run of draws into the same targets: one pass
                    let Some(first) = prepared[i].as_ref() else {
                        i += 1;
                        continue;
                    };
                    let target = first.target;
                    let views = first.color_views.clone();
                    let depth_view = first.depth_view.clone();
                    let through_doorway = matches!(&cmds[i], Cmd::Draw(d) if capture::is_portal_draw(rs(&d.states, 196)));
                    if target.0[0] == backbuffer
                        && target.1 != 0
                        && depth_view.is_some()
                        && !through_doorway
                        && let Some(cam) = self.camera
                        && cam.frustum[5] > 50.0
                    {
                        self.scene = Some((cam, target.1)); // the world, not the HUD
                    }
                    let mut end = i;
                    while end < cmds.len() && matches!(&cmds[end], Cmd::Draw(_)) && prepared[end].as_ref().is_none_or(|p| p.target == target && p.depth_view.is_some() == depth_view.is_some()) {
                        end += 1;
                    }
                    for (&rt, _) in target.0.iter().zip(0..) {
                        if let Some(r) = self.rts.get_mut(&rt) {
                            if !r.rendered && DEBUG_RT.load(std::sync::atomic::Ordering::Relaxed) {
                                let (w, h) = (r.width, r.height);
                                log(&format!("frame {}: first drawn into {rt:#x} ({w}x{h})", self.frame));
                            }
                            r.rendered = true;
                        }
                    }
                    let attachments: Vec<Option<wgpu::RenderPassColorAttachment>> = views
                        .iter()
                        .map(|v| Some(wgpu::RenderPassColorAttachment { view: v, depth_slice: None, resolve_target: None, ops: wgpu::Operations { load: wgpu::LoadOp::Load, store: wgpu::StoreOp::Store } }))
                        .collect();
                    let depth_att = depth_view.as_ref().map(|v| wgpu::RenderPassDepthStencilAttachment {
                        view: v,
                        depth_ops: Some(wgpu::Operations { load: wgpu::LoadOp::Load, store: wgpu::StoreOp::Store }),
                        stencil_ops: None,
                    });
                    let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: Some("game pass"),
                        color_attachments: &attachments,
                        depth_stencil_attachment: depth_att,
                        timestamp_writes: None,
                        occlusion_query_set: None,
                        multiview_mask: None,
                    });
                    let mut last: Option<&Prepared> = None;
                    let mut last_view = None;
                    let mut last_scissor = None;
                    let mut last_blend = None;
                    let mut last_stencil = None;
                    for k in i..end {
                        let (Some(p), Cmd::Draw(d)) = (prepared[k].as_ref(), &cmds[k]) else { continue };
                        let (tw, th) = p.size;
                        let vp = d.viewport;
                        let (x, y) = (vp.x.min(tw - 1) as f32, vp.y.min(th - 1) as f32);
                        let (w, h) = ((vp.w as f32).min(tw as f32 - x).max(1.0), (vp.h as f32).min(th as f32 - y).max(1.0));
                        let view = [x, y, w, h, vp.min_z.clamp(0.0, 1.0), vp.max_z.clamp(0.0, 1.0).max(vp.min_z.clamp(0.0, 1.0))];
                        if last_view != Some(view) {
                            pass.set_viewport(view[0], view[1], view[2], view[3], view[4], view[5]);
                            last_view = Some(view);
                        }
                        let scissor = if rs(&d.states, 174) != 0 {
                            let [l, t, r, b] = d.scissor;
                            let (l, t) = (l.clamp(0, tw as i32) as u32, t.clamp(0, th as i32) as u32);
                            let (r, b) = (r.clamp(l as i32, tw as i32) as u32, b.clamp(t as i32, th as i32) as u32);
                            [l, t, r - l, b - t]
                        } else {
                            [0, 0, tw, th]
                        };
                        if last_scissor != Some(scissor) {
                            pass.set_scissor_rect(scissor[0], scissor[1], scissor[2], scissor[3]);
                            last_scissor = Some(scissor);
                        }
                        let bf = rs(&d.states, 193);
                        if last_blend != Some(bf) {
                            pass.set_blend_constant(wgpu::Color { r: ((bf >> 16) & 0xFF) as f64 / 255.0, g: ((bf >> 8) & 0xFF) as f64 / 255.0, b: (bf & 0xFF) as f64 / 255.0, a: (bf >> 24) as f64 / 255.0 });
                            last_blend = Some(bf);
                        }
                        let stencil = if capture::is_portal_draw(rs(&d.states, 196)) { 1 } else { rs(&d.states, 57) };
                        if last_stencil != Some(stencil) {
                            pass.set_stencil_reference(stencil);
                            last_stencil = Some(stencil);
                        }
                        if last.is_none_or(|l| l.pipeline != p.pipeline) {
                            pass.set_pipeline(&p.pipeline);
                        }
                        if last.is_none_or(|l| l.constants != p.constants) {
                            pass.set_bind_group(0, &constants, &p.constants);
                        }
                        if last.is_none_or(|l| l.textures != p.textures) {
                            pass.set_bind_group(1, &p.textures, &[]);
                        }
                        for (slot, buffer, offset) in &p.vertex_buffers {
                            let same = last.is_some_and(|l| l.vertex_buffers.iter().any(|(s, b, o)| s == slot && b == buffer && o == offset));
                            if !same && *offset < buffer.size() {
                                pass.set_vertex_buffer(*slot, buffer.slice(*offset..));
                            }
                        }
                        let same_index = last.is_some_and(|l| l.index == p.index);
                        let count = capture::vertex_count(d.prim, d.prims);
                        let count = if d.prim == 6 { d.prims * 3 } else { count };
                        match &p.index {
                            Some((ib, big)) if d.indexed => {
                                if !same_index {
                                    pass.set_index_buffer(ib.slice(..), if *big { wgpu::IndexFormat::Uint32 } else { wgpu::IndexFormat::Uint16 });
                                }
                                pass.draw_indexed(d.start..d.start + count, d.base_vertex, 0..1);
                            }
                            Some((ib, _)) => {
                                // a non-indexed fan made into a list
                                pass.set_index_buffer(ib.slice(..), wgpu::IndexFormat::Uint32);
                                pass.draw_indexed(0..count, 0, 0..1);
                            }
                            None => pass.draw(d.start..d.start + count, 0..1),
                        }
                        // (a fan's own index buffer is set every time: it's per draw)
                        last = Some(p).filter(|_| !(p.index.is_some() && !d.indexed));
                    }
                    drop(pass);
                    i = end;
                }
            }
        }

        self.perf[1] += t_prepared.elapsed().as_secs_f64();
        // the frame: the back buffer, to our window
        let Some(back) = self.rt(backbuffer).map(|r| r.view.clone()) else {
            self.queue.submit([enc.finish()]);
            return;
        };
        if let Some(path) = shot {
            self.save(&mut enc, backbuffer, path);
        }
        if DUMP_RTS.swap(false, std::sync::atomic::Ordering::Relaxed) {
            // render-target textures: our copy and D3D9's, to compare
            let list: Vec<(u32, u32)> = self.surfaces_of.iter().flat_map(|(&t, ss)| ss.iter().map(move |&s| (t, s))).collect();
            for (tex, surface) in list {
                if self.rts.get(&surface).is_none_or(|r| r.width != 512) {
                    continue;
                }
                let dir = crate::win::game_file("");
                self.save(&mut enc, surface, &dir.join(format!("rt_{surface:x}_ours.png")));
                if let Some(t) = unsafe { d3d::texture(tex) } {
                    let _ = textures::save_png(&dir.join(format!("rt_{surface:x}_d3d9.png")), t.width, t.height, &t.levels[0]);
                }
            }
        }
        match self.surface.get_current_texture() {
            wgpu::CurrentSurfaceTexture::Success(t) | wgpu::CurrentSurfaceTexture::Suboptimal(t) => {
                let view = t.texture.create_view(&Default::default());
                let (image, bloom) = self.post_frame(&mut enc, backbuffer).unwrap_or_else(|| (back.clone(), self.post.black.clone()));
                {
                    let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                        label: Some("present"),
                        color_attachments: &[Some(wgpu::RenderPassColorAttachment { view: &view, depth_slice: None, resolve_target: None, ops: wgpu::Operations { load: wgpu::LoadOp::Clear(wgpu::Color::BLACK), store: wgpu::StoreOp::Store } })],
                        depth_stencil_attachment: None,
                        timestamp_writes: None,
                        occlusion_query_set: None,
                        multiview_mask: None,
                    });
                    let format = self.config.format;
                    self.present_pass(&mut pass, &image, &bloom, format);
                }
                let t_present = std::time::Instant::now();
                self.queue.submit([enc.finish()]);
                self.queue.present(t);
                self.perf[2] += t_present.elapsed().as_secs_f64();
                self.perf[0] += t_start.elapsed().as_secs_f64();
                self.last_ms = self.last_ms * 0.95 + t_start.elapsed().as_secs_f32() * 1000.0 * 0.05;
            }
            _ => {
                self.queue.submit([enc.finish()]);
                self.surface.configure(&self.device, &self.config);
            }
        }
    }

    #[allow(clippy::too_many_arguments)]
    fn clear(&mut self, enc: &mut wgpu::CommandEncoder, rt: &[u32; 4], ds: u32, flags: u32, color: u32, z: f32, stencil: u32, _vp: &Viewport, _rects: &[[i32; 4]]) {
        let c = wgpu::Color { r: ((color >> 16) & 0xFF) as f64 / 255.0, g: ((color >> 8) & 0xFF) as f64 / 255.0, b: (color & 0xFF) as f64 / 255.0, a: (color >> 24) as f64 / 255.0 };
        let views: Vec<(wgpu::TextureView, bool)> = if flags & 1 != 0 { rt.iter().filter(|&&r| r != 0).filter_map(|&r| self.rt(r).map(|x| (x.view.clone(), x.opaque))).collect() } else { Vec::new() };
        for &r in rt.iter().filter(|&&r| r != 0) {
            if flags & 1 != 0
                && let Some(x) = self.rts.get_mut(&r)
            {
                x.rendered = true;
            }
        }
        let depth = if flags & 6 != 0 && ds != 0 { self.depth(ds) } else { None };
        if views.is_empty() && depth.is_none() {
            return;
        }
        let attachments: Vec<Option<wgpu::RenderPassColorAttachment>> = views
            .iter()
            .map(|(v, opaque)| Some(wgpu::RenderPassColorAttachment { view: v, depth_slice: None, resolve_target: None, ops: wgpu::Operations { load: wgpu::LoadOp::Clear(if *opaque { wgpu::Color { a: 1.0, ..c } } else { c }), store: wgpu::StoreOp::Store } }))
            .collect();
        // a depth-only clear, or colour and depth of the same size
        let color_size = rt.iter().find(|&&r| r != 0).and_then(|r| self.rts.get(r)).map(|r| (r.width, r.height));
        let depth = depth.filter(|d| views.is_empty() || color_size == Some((d.2, d.3)));
        let depth_att = depth.as_ref().map(|(v, format, _, _)| wgpu::RenderPassDepthStencilAttachment {
            view: v,
            depth_ops: Some(wgpu::Operations { load: if flags & 2 != 0 { wgpu::LoadOp::Clear(z.clamp(0.0, 1.0)) } else { wgpu::LoadOp::Load }, store: wgpu::StoreOp::Store }),
            stencil_ops: has_stencil(*format).then_some(wgpu::Operations { load: if flags & 4 != 0 { wgpu::LoadOp::Clear(stencil) } else { wgpu::LoadOp::Load }, store: wgpu::StoreOp::Store }),
        });
        let _ = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("clear"),
            color_attachments: &attachments,
            depth_stencil_attachment: depth_att,
            timestamp_writes: None,
            occlusion_query_set: None,
            multiview_mask: None,
        });
    }

    fn stretch(&mut self, enc: &mut wgpu::CommandEncoder, src: u32, src_rect: Option<[i32; 4]>, dst: u32, dst_rect: Option<[i32; 4]>) {
        let dst_opaque = self.rt(dst).is_some_and(|r| r.opaque);
        let (Some(s), Some(d)) = (self.rt(src).map(|r| (r.view.clone(), r.width, r.height)), self.rt(dst).map(|r| (r.view.clone(), r.format, r.width, r.height))) else {
            self.report("StretchRect between surfaces the replay doesn't have".into());
            return;
        };
        if let Some(r) = self.rts.get_mut(&dst) {
            if DEBUG_RT.load(std::sync::atomic::Ordering::Relaxed) && r.width == 512 {
                log(&format!("frame {}: stretch {src:#x} -> {dst:#x} (was rendered: {})", self.frame, r.rendered));
            }
            r.rendered = true;
        }
        let (sv, sw, sh) = s;
        let (dv, df, dw, dh) = d;
        let uv = match src_rect {
            Some([l, t, r, b]) => [l as f32 / sw as f32, t as f32 / sh as f32, r as f32 / sw as f32, b as f32 / sh as f32],
            None => [0.0, 0.0, 1.0, 1.0],
        };
        let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("stretch"),
            color_attachments: &[Some(wgpu::RenderPassColorAttachment { view: &dv, depth_slice: None, resolve_target: None, ops: wgpu::Operations { load: wgpu::LoadOp::Load, store: wgpu::StoreOp::Store } })],
            depth_stencil_attachment: None,
            timestamp_writes: None,
            occlusion_query_set: None,
            multiview_mask: None,
        });
        if let Some([l, t, r, b]) = dst_rect {
            let (l, t) = (l.clamp(0, dw as i32) as f32, t.clamp(0, dh as i32) as f32);
            let (r, b) = (r.clamp(l as i32, dw as i32) as f32, b.clamp(t as i32, dh as i32) as f32);
            pass.set_viewport(l, t, (r - l).max(1.0), (b - t).max(1.0), 0.0, 1.0);
        }
        self.blit_masked(&mut pass, &sv, uv, df, !dst_opaque);
    }

    fn save(&mut self, enc: &mut wgpu::CommandEncoder, backbuffer: u32, path: &std::path::Path) {
        let Some(r) = self.rts.get(&backbuffer) else { return };
        let (w, h) = (r.width, r.height);
        let (mut texture, mut format, src) = (r.texture.clone(), r.format, r.view.clone());
        if SHOT_FINAL.swap(false, std::sync::atomic::Ordering::Relaxed) {
            // what goes to the window: through the picture settings
            texture = self.device.create_texture(&wgpu::TextureDescriptor {
                label: Some("final shot"),
                size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Bgra8Unorm,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
                view_formats: &[],
            });
            format = wgpu::TextureFormat::Bgra8Unorm;
            let view = texture.create_view(&Default::default());
            let (image, bloom) = self.post_frame(enc, backbuffer).unwrap_or_else(|| (src.clone(), self.post.black.clone()));
            let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("final shot"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment { view: &view, depth_slice: None, resolve_target: None, ops: wgpu::Operations { load: wgpu::LoadOp::Clear(wgpu::Color::BLACK), store: wgpu::StoreOp::Store } })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
                multiview_mask: None,
            });
            self.present_pass(&mut pass, &image, &bloom, format);
        }
        let row = (w * 4).div_ceil(256) * 256;
        let buffer = self.device.create_buffer(&wgpu::BufferDescriptor { label: Some("shot"), size: (row * h) as u64, usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ, mapped_at_creation: false });
        enc.copy_texture_to_buffer(
            wgpu::TexelCopyTextureInfo { texture: &texture, mip_level: 0, origin: wgpu::Origin3d::ZERO, aspect: wgpu::TextureAspect::All },
            wgpu::TexelCopyBufferInfo { buffer: &buffer, layout: wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(row), rows_per_image: Some(h) } },
            wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
        );
        let bgra = format == wgpu::TextureFormat::Bgra8Unorm;
        let path = path.to_path_buf();
        let device = self.device.clone();
        // after this frame's work: map and write
        let (tx, rx) = std::sync::mpsc::channel();
        SAVES.with(|s| s.borrow_mut().push((buffer, w, h, row, bgra, path, device, tx, rx)));
    }
}

/// How often cached objects the game has let go of are dropped.
const SWEEP_FRAMES: u64 = 120;

/// Zeros after vertex data, for elements reaching past the last vertex's stride.
const VERTEX_PAD: usize = 64;

/// Debugging: log render targets as they're first drawn into.
pub static DEBUG_RT: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

/// Debugging: save every 512x512 render-target texture, ours and D3D9's.
pub static DUMP_RTS: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

/// The next new shot is of what goes to the window (the picture settings applied).
pub static SHOT_FINAL: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

/// Debugging: log the sampler states draws use.
pub static SAMPLER_REPORT: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

/// Debugging: log draw N (1-based) of the next frame in full.
pub static DUMP_DRAW: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);

/// Debugging: draw both faces of everything.
pub static NO_CULL: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

/// Replay only this many commands of each frame (0: all).
pub static LIMIT: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);

/// Log the next frame's structure.
pub static DESCRIBE: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

type Save = (wgpu::Buffer, u32, u32, u32, bool, std::path::PathBuf, wgpu::Device, std::sync::mpsc::Sender<()>, std::sync::mpsc::Receiver<()>);

thread_local! {
    static SAVES: std::cell::RefCell<Vec<Save>> = const { std::cell::RefCell::new(Vec::new()) };
}

/// Writes the screenshots requested this frame (after the frame is submitted).
fn finish_saves() {
    SAVES.with(|s| {
        for (buffer, w, h, row, bgra, path, device, _tx, _rx) in s.borrow_mut().drain(..) {
            buffer.map_async(wgpu::MapMode::Read, .., |_| {});
            let _ = device.poll(wgpu::PollType::wait_indefinitely());
            let Ok(data) = buffer.slice(..).get_mapped_range() else { continue };
            let mut rgba = Vec::with_capacity((w * h * 4) as usize);
            for y in 0..h as usize {
                for px in data[y * row as usize..][..w as usize * 4].chunks_exact(4) {
                    if bgra {
                        rgba.extend([px[2], px[1], px[0], 255]);
                    } else {
                        rgba.extend([px[0], px[1], px[2], 255]);
                    }
                }
            }
            drop(data);
            let r = crate::control::write_png(&path, w, h, &rgba);
            log(&match r {
                Ok(()) => format!("new shot {}", path.display()),
                Err(e) => format!("new shot failed: {e}"),
            });
        }
    });
}

struct Prepared {
    pipeline: wgpu::RenderPipeline,
    textures: wgpu::BindGroup,
    vertex_buffers: Vec<(u32, wgpu::Buffer, u64)>,
    index: Option<(wgpu::Buffer, bool)>,
    /// Offsets of its vertex shader, pixel shader and frame constants.
    constants: [u32; 3],
    color_views: Vec<wgpu::TextureView>,
    depth_view: Option<wgpu::TextureView>,
    target: ([u32; 4], u32),
    size: (u32, u32),
}

// ---- on / off, and the frame hook -------------------------------------------------

pub struct Toggle {
    replay: Option<Result<Worker, String>>,
    on: bool,
    was_down: bool,
    shot: Option<std::path::PathBuf>,
    custom: Option<Custom>,
    /// Show our window again (hidden for a device reset).
    reshow: bool,
    /// Window resizes -> the game's resolution; capture pauses around the change.
    resizer: crate::resize::Resizer,
    paused: bool,
    was_exclusive: bool,
    /// Presents in a row with the device fine.
    device_ok_frames: u32,
}

/// The menu's side: the settings file, what was last applied, the status file.
struct Custom {
    watcher: settings::Watcher,
    settings: Option<Settings>,
    frames: u32,
    status: settings::Status,
    /// The rain demo: when the rain starts.
    demo_rain_at: Option<std::time::Instant>,
}

const OVERCAST: i32 = 1;
const RAIN: i32 = 2;
const WEATHER: usize = 0x00CE_756C;

fn weather_now() -> i32 {
    unsafe { *(WEATHER as *const i16) as i32 }
}

/// What the script function WeatherSet does (0x5360E0), on the game's thread. Only with
/// a player (in a game, not the menus).
fn set_weather(w: i32) {
    if crate::win::read_u32(0x00C1_AEA8).is_none_or(|p| p == 0) {
        return;
    }
    type SetWeather = unsafe extern "C" fn(u32, u32);
    let set: SetWeather = unsafe { std::mem::transmute(0x0053_60E0usize) };
    unsafe { set(w.clamp(0, 4) as u32, 0) };
    log(&format!("weather {w}"));
}

// ---- the render thread -----------------------------------------------------------
//
// The game thread only records a frame (capture.rs) and hands it over; the replay
// runs on a thread of its own while the game goes on with its next frame. At most one
// frame waits: handing over the next blocks until the last one is taken. The device is
// created D3DCREATE_MULTITHREADED, as the replay still reads the game's resources.

enum Msg {
    Frame { cmds: Vec<Cmd>, fixed: u32, held: capture::Held, back: u32, shot: Option<std::path::PathBuf> },
    Configure(Settings, bool),
    /// Let go of every D3D object (the device is being reset), then say so.
    ReleaseAll(std::sync::mpsc::Sender<usize>),
}

/// What the render thread tells the game thread.
#[derive(Default)]
struct Shared {
    ms: std::sync::Mutex<f32>,
    replaced: std::sync::atomic::AtomicUsize,
    wetness: std::sync::atomic::AtomicU32, // f32 bits
    failed: std::sync::Mutex<Option<String>>,
}

struct Worker {
    tx: std::sync::mpsc::SyncSender<Msg>,
    shared: Arc<Shared>,
    window: usize,
    gpu: String,
}

/// The replay moves to the render thread once, whole (its window stays the game
/// thread's: it's only resized and shown asynchronously from there).
struct SendReplay(Replay);
unsafe impl Send for SendReplay {}

impl Worker {
    fn start(r: Replay) -> Result<Worker, String> {
        let (tx, rx) = std::sync::mpsc::sync_channel::<Msg>(0);
        let shared = Arc::new(Shared::default());
        let (window, gpu) = (r.window as usize, r.gpu.clone());
        let sh = shared.clone();
        let boxed = SendReplay(r);
        std::thread::Builder::new()
            .name("bullygfx render".into())
            .spawn(move || {
                let mut r = boxed;
                let run = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                    for msg in rx {
                        match msg {
                            Msg::Frame { cmds, fixed, held, back, shot } => {
                                r.0.replay(cmds, fixed, back, shot.as_deref());
                                finish_saves();
                                drop(held);
                                if let Ok(mut ms) = sh.ms.lock() {
                                    *ms = r.0.frame_ms();
                                }
                                sh.replaced.store(r.0.replaced_count(), std::sync::atomic::Ordering::Relaxed);
                                sh.wetness.store(r.0.wetness().to_bits(), std::sync::atomic::Ordering::Relaxed);
                            }
                            Msg::Configure(s, first) => r.0.configure(&s, first),
                            Msg::ReleaseAll(done) => {
                                let n = r.0.held_count();
                                r.0.release_all();
                                // nothing of ours in flight on the GPU while D3D9 resets
                                let _ = r.0.device.poll(wgpu::PollType::wait_indefinitely());
                                let _ = done.send(n);
                            }
                        }
                    }
                }));
                if run.is_err() {
                    capture::ENABLED.store(false, std::sync::atomic::Ordering::SeqCst);
                    capture::EXCLUSIVE.store(false, std::sync::atomic::Ordering::SeqCst);
                    unsafe { ShowWindowAsync(window as *mut c_void, SW_HIDE) };
                    if let Ok(mut f) = sh.failed.lock() {
                        *f = Some("the render thread failed (see bullygfx_panic.txt)".into());
                    }
                    log("the new renderer failed (see bullygfx_panic.txt): back to D3D9");
                }
            })
            .map_err(|e| format!("render thread: {e}"))?;
        Ok(Worker { tx, shared, window, gpu })
    }

    fn show(&self, on: bool) {
        unsafe { ShowWindowAsync(self.window as *mut c_void, if on { SW_SHOWNA } else { SW_HIDE }) };
    }

    /// Our window kept above the game's render window (its sibling): D3D9 raises the window
    /// it presents to (seen in the 64-bit game), and its picture, nothing drawn while we
    /// draw, then covers ours: the screen black.
    fn keep_on_top(&self) {
        #[link(name = "user32")]
        unsafe extern "system" {
            fn GetParent(h: *mut c_void) -> *mut c_void;
            fn GetWindow(h: *mut c_void, cmd: u32) -> *mut c_void;
        }
        const GW_CHILD: u32 = 5;
        const SWP_NOSIZE: u32 = 0x1;
        const SWP_NOMOVE: u32 = 0x2;
        let window = self.window as *mut c_void;
        unsafe {
            let parent = GetParent(window);
            if !parent.is_null() && GetWindow(parent, GW_CHILD) != window {
                SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
            }
        }
    }

    fn failed(&self) -> Option<String> {
        self.shared.failed.lock().ok().and_then(|f| f.clone())
    }
}

impl Toggle {
    pub const fn new() -> Toggle {
        Toggle { replay: None, on: false, was_down: false, shot: None, custom: None, reshow: false, resizer: crate::resize::Resizer::new(), paused: false, was_exclusive: false, device_ok_frames: 100 }
    }

    /// After a panic: the replay is gone, D3D9 draws.
    pub fn fail(&mut self) {
        self.on = false;
        capture::ENABLED.store(false, std::sync::atomic::Ordering::SeqCst);
        capture::EXCLUSIVE.store(false, std::sync::atomic::Ordering::SeqCst);
        if let Some(Ok(w)) = &self.replay {
            w.show(false);
        }
        let why = match &self.replay {
            Some(Ok(w)) => w.failed().unwrap_or_else(|| "failed".into()),
            _ => "failed".into(),
        };
        self.replay = Some(Err(why));
    }

    /// The device is about to be reset.
    pub fn before_reset_check(device: u32) {
        // refcounts of the back buffer and depth surface (AddRef/Release: the count after
        // our AddRef, minus that one)
        let count = |o: u32| unsafe {
            if o == 0 {
                return 0;
            }
            let n = add_ref(o);
            release(o);
            n - 1
        };
        let mut back = 0usize;
        let mut ds = 0usize;
        type GetBackBuffer = unsafe extern "system" fn(usize, u32, u32, u32, *mut usize) -> i32;
        type GetDs = unsafe extern "system" fn(usize, *mut usize) -> i32;
        unsafe {
            std::mem::transmute::<usize, GetBackBuffer>(method(device, 18))(device as usize, 0, 0, 0, &mut back);
            std::mem::transmute::<usize, GetDs>(method(device, 40))(device as usize, &mut ds);
        }
        let (back, ds) = (crate::win::obj32(back), crate::win::obj32(ds));
        let (b, d) = (count(back), count(ds));
        unsafe {
            release(back);
            release(ds);
        }
        log(&format!("  before reset: back buffer refs {} depth refs {} (each includes our Get)", b, d));
    }

    pub fn before_reset(&mut self) {
        {
            static T0: std::sync::OnceLock<std::time::Instant> = std::sync::OnceLock::new();
            let t = T0.get_or_init(std::time::Instant::now).elapsed().as_secs_f32();
            let r = |a: u32| crate::win::read_u32(a).unwrap_or(0);
            log(&format!("device reset at {t:.2}s: mode {}x{} pending {}x{} aa {}", r(0xBCBC60), r(0xBCBC64), r(0xBCBC78), r(0xBCBC7C), r(0xBCBC80)));
        }
        let taken = capture::take();
        log(&format!("  capture held {} refs, released", taken.2.len()));
        drop(taken);
        if let Some(Ok(w)) = &self.replay {
            // our window out of the way of the reset (shown again at the next frame)
            unsafe { ShowWindow(w.window as *mut c_void, SW_HIDE) };
            self.reshow = self.on;
            let (done, wait) = std::sync::mpsc::channel();
            if w.tx.send(Msg::ReleaseAll(done)).is_ok() {
                if let Ok(n) = wait.recv() {
                    log(&format!("  render thread released {n} refs"));
                }
            }
        }
    }

    pub fn set(&mut self, on: bool) {
        self.on = on;
        if on && self.replay.is_none() {
            let r = Replay::new().and_then(|mut r| {
                if let Some(s) = self.custom.as_ref().and_then(|c| c.settings.as_ref()) {
                    r.configure(s, true);
                }
                Worker::start(r)
            });
            if let Err(e) = &r {
                log(&format!("replay renderer failed: {e}"));
            }
            self.replay = Some(r);
        }
        if let Some(Ok(w)) = &self.replay {
            w.show(on);
        }
        // (the Set* calls weren't followed while off)
        capture::SHADOW_STALE.store(true, std::sync::atomic::Ordering::SeqCst);
        capture::ENABLED.store(on && matches!(self.replay, Some(Ok(_))), std::sync::atomic::Ordering::Relaxed);
        if !on {
            capture::EXCLUSIVE.store(false, std::sync::atomic::Ordering::Relaxed);
        }
        log(&format!("replay renderer {}", if on { "on" } else { "off" }));
    }

    /// Follows the menu's settings file and tells it how things are.
    fn customiser(&mut self) {
        let c = self.custom.get_or_insert_with(|| Custom { watcher: Default::default(), settings: None, frames: 0, status: Default::default(), demo_rain_at: None });
        if let Some(new) = c.watcher.poll() {
            let first = c.settings.is_none();
            let renderer_changed = c.settings.as_ref().is_none_or(|old| old.renderer != new.renderer);
            let before = c.settings.replace(new.clone());
            crate::camera::set(new.fov, new.hor_plus);
            if let Some(old) = &before {
                if new.weather_seq != old.weather_seq {
                    set_weather(new.weather);
                }
                if new.demo_seq != old.demo_seq {
                    // the rain demo: overcast now, the rain a few seconds later
                    set_weather(OVERCAST);
                    c.demo_rain_at = Some(std::time::Instant::now() + std::time::Duration::from_secs(6));
                }
            }
            if renderer_changed && (new.renderer || !first) && new.renderer != self.on {
                self.set(new.renderer);
                capture::EXCLUSIVE.store(self.on, std::sync::atomic::Ordering::Relaxed);
            }
            if let Some(Ok(w)) = &self.replay {
                let _ = w.tx.send(Msg::Configure(new, first));
            }
        }
        let c = self.custom.as_mut().unwrap();
        if c.demo_rain_at.is_some_and(|t| std::time::Instant::now() >= t) {
            c.demo_rain_at = None;
            set_weather(RAIN);
        }
        c.frames += 1;
        if c.frames % 30 == 0 {
            let worker = match &self.replay {
                Some(Ok(w)) => Some(w),
                _ => None,
            };
            let status = settings::Status {
                state: match &self.replay {
                    Some(Err(e)) => format!("failed: {e}"),
                    _ if self.on => "on".into(),
                    _ => "off".into(),
                },
                gpu: worker.map(|w| w.gpu.clone()).unwrap_or_default(),
                ms: worker.and_then(|w| w.shared.ms.lock().ok().map(|m| *m)).unwrap_or(0.0),
                named: textures::named_count(),
                replaced: worker.map(|w| w.shared.replaced.load(std::sync::atomic::Ordering::Relaxed)).unwrap_or(0),
                exported: textures::EXPORT_STATUS.lock().map(|s| s.clone()).unwrap_or_default(),
                weather: weather_now(),
                wetness: worker.map(|w| f32::from_bits(w.shared.wetness.load(std::sync::atomic::Ordering::Relaxed))).unwrap_or(0.0),
            };
            if status != c.status {
                settings::write_status(&status);
                c.status = status;
            }
        }
    }

    pub fn request_shot(&mut self, path: std::path::PathBuf) {
        self.shot = Some(path);
    }

    /// At the game's Present: `device` is the real IDirect3DDevice9.
    pub fn present(&mut self, device: u32) {
        // the device's state: while it's lost or waiting for a reset, the new renderer
        // steps aside (the reset fails if we record in the frames before it), back a
        // few frames after it's fine again
        let device_ok = {
            static LAST: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
            type Tcl = unsafe extern "system" fn(usize) -> i32;
            let hr = unsafe { std::mem::transmute::<usize, Tcl>(method(device, 3))(device as usize) } as u32;
            if LAST.swap(hr, std::sync::atomic::Ordering::Relaxed) != hr {
                log(&format!("device state {hr:#010x}"));
            }
            hr == 0
        };
        if !device_ok {
            self.device_ok_frames = 0;
        } else {
            self.device_ok_frames = self.device_ok_frames.saturating_add(1);
        }
        let down = unsafe { GetAsyncKeyState(TOGGLE_KEY) } as u16 & 0x8000 != 0 && game_in_front();
        if down && !self.was_down {
            // from the keyboard: the new renderer alone (D3D9 draws nothing)
            self.set(!self.on);
            capture::EXCLUSIVE.store(self.on, std::sync::atomic::Ordering::Relaxed);
        }
        self.was_down = down;
        self.customiser();
        crate::modfs::tick();
        // a window resize in progress: the new renderer steps aside while the game resets
        let pause = self.resizer.tick() | (self.device_ok_frames < 5);
        if pause != self.paused {
            self.paused = pause;
            use std::sync::atomic::Ordering::SeqCst;
            if pause {
                self.was_exclusive = capture::EXCLUSIVE.load(SeqCst);
                capture::ENABLED.store(false, SeqCst);
                capture::EXCLUSIVE.store(false, SeqCst);
                if let Some(Ok(w)) = &self.replay {
                    w.show(false);
                }
            } else if self.on && matches!(self.replay, Some(Ok(_))) {
                capture::SHADOW_STALE.store(true, SeqCst);
                capture::ENABLED.store(true, SeqCst);
                capture::EXCLUSIVE.store(self.was_exclusive, SeqCst);
                self.reshow = true;
            }
        }
        if self.paused {
            drop(capture::take());
            return;
        }
        let (cmds, fixed, held) = capture::take();
        let shot = self.shot.take();
        if !self.on {
            if shot.is_some() {
                log("new shot: the replay renderer isn't on");
            }
            return;
        }
        let Some(Ok(w)) = &self.replay else { return };
        if std::mem::take(&mut self.reshow) {
            w.show(true);
        }
        w.keep_on_top();
        if w.failed().is_some() {
            self.fail();
            return;
        }
        // the surface the game presents
        let mut back = 0usize;
        type GetBackBuffer = unsafe extern "system" fn(usize, u32, u32, u32, *mut usize) -> i32;
        unsafe {
            std::mem::transmute::<usize, GetBackBuffer>(method(device, 18))(device as usize, 0, 0, 0, &mut back);
        }
        let back = crate::win::obj32(back);
        unsafe { release(back) };
        if w.tx.send(Msg::Frame { cmds, fixed, held, back, shot }).is_err() {
            self.fail();
        }
    }
}
