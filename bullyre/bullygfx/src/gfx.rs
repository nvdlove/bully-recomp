//! The new renderer: wgpu (Vulkan, D3D12 as the fallback), drawing into its own
//! window laid over the game's picture (a child of the game window that lets input
//! through). F8 or the control script shows or hides it. It draws the world meshes the
//! game drew this frame, with their real transforms, camera and base textures, lit
//! simply.

use std::collections::HashMap;
use std::ffi::c_void;
use std::num::NonZeroIsize;

use wgpu::util::DeviceExt;

use crate::d3d;
use crate::scene::{Draw, Matrix};
use crate::win::*;

const TOGGLE_KEY: i32 = 0x77; // F8
/// The game's window handle (BullyMain keeps it here).
const GAME_HWND: u32 = 0x00BD_77FC;
const DEPTH: wgpu::TextureFormat = wgpu::TextureFormat::Depth32Float;

const SHADER: &str = r#"
struct Inst {
    world: mat4x4<f32>, view: mat4x4<f32>, proj: mat4x4<f32>, bones: vec4<u32>, alpha: vec4<f32>,
    ambient: vec4<f32>, diffuse: vec4<f32>, emissive: vec4<f32>, // material; ambient.w = 1: unlit
};
// The game's time of day: ambient for the world and for characters, sun colour and
// direction (towards the sun).
struct Light { ambient: vec4<f32>, ambient_peds: vec4<f32>, sun: vec4<f32>, sun_dir: vec4<f32>, mode: vec4<u32> };
@group(0) @binding(0) var<storage, read> insts: array<Inst>;
@group(0) @binding(1) var<storage, read> bones: array<vec4<f32>>;
@group(0) @binding(2) var<uniform> light: Light;
@group(1) @binding(0) var base: texture_2d<f32>;
@group(1) @binding(1) var smp: sampler;

struct Out {
    @builtin(position) pos: vec4<f32>, @location(0) n: vec3<f32>, @location(1) uv: vec2<f32>,
    @location(2) @interpolate(flat) cut: f32, @location(3) color: vec4<f32>, @location(4) @interpolate(flat) inst: u32,
    @location(5) @interpolate(flat) skinned: u32,
};

// The matrices are Direct3D's (row-major, row vectors) as they are, so here they read
// transposed and multiply on the left.
@vertex fn vs(@location(0) p: vec3<f32>, @location(1) n: vec3<f32>, @location(2) uv: vec2<f32>, @location(5) color: vec4<f32>, @builtin(instance_index) i: u32) -> Out {
    let it = insts[i];
    var o: Out;
    o.pos = it.proj * (it.view * (it.world * vec4(p, 1.0)));
    o.n = (it.world * vec4(n, 0.0)).xyz;
    o.uv = uv;
    o.cut = it.alpha.x;
    o.color = color;
    o.inst = i;
    o.skinned = 0u;
    return o;
}

// Skinned: bone k is rows bones[base + 3k .. 3k + 2] of a 4x3 skin-to-world matrix.
fn bone(base: u32, k: u32, p: vec4<f32>) -> vec3<f32> {
    let r = base + 3u * k;
    return vec3(dot(bones[r], p), dot(bones[r + 1u], p), dot(bones[r + 2u], p));
}

@vertex fn vs_skin(@location(0) p: vec3<f32>, @location(1) n: vec3<f32>, @location(2) uv: vec2<f32>,
                   @location(3) w: vec3<f32>, @location(4) idx: vec4<u32>, @location(5) color: vec4<f32>, @builtin(instance_index) i: u32) -> Out {
    let it = insts[i];
    let b = it.bones.x;
    // bones.y = 1: the indices are D3DCOLOR as a shader sees it (bytes 2, 1, 0, 3)
    var ix = idx;
    if it.bones.y == 1u {
        ix = idx.zyxw;
    }
    let w4 = vec4(w, 1.0 - w.x - w.y - w.z);
    let pos = vec4(p, 1.0);
    let nrm = vec4(n, 0.0);
    var world = vec3(0.0);
    var normal = vec3(0.0);
    for (var k = 0u; k < 4u; k++) {
        if w4[k] != 0.0 {
            world += w4[k] * bone(b, ix[k], pos);
            normal += w4[k] * bone(b, ix[k], nrm);
        }
    }
    var o: Out;
    o.pos = it.proj * (it.view * vec4(world, 1.0));
    o.n = normal;
    o.uv = uv;
    o.cut = it.alpha.x;
    o.color = color;
    o.inst = i;
    o.skinned = 1u;
    return o;
}

@fragment fn fs(v: Out) -> @location(0) vec4<f32> {
    let c = textureSample(base, smp, v.uv);
    if c.a <= v.cut {
        discard; // the material's alpha test (cut-out foliage, fences); -1 = none
    }
    let m = insts[v.inst];
    if m.ambient.w > 0.5 {
        return vec4(c.rgb * v.color.rgb, 1.0); // unlit (the sky)
    }
    // Gamebryo's lighting: emissive + ambient * ambient light + diffuse * sun * N.L,
    // the vertex colour carrying the baked light
    var amb = light.ambient.rgb;
    if v.skinned == 1u {
        amb = light.ambient_peds.rgb;
    }
    let ndl = max(dot(normalize(v.n), light.sun_dir.xyz), 0.0);
    let lit = m.emissive.rgb + m.ambient.rgb * amb + m.diffuse.rgb * light.sun.rgb * ndl;
    switch light.mode.x {
        case 1u: { return vec4(c.rgb * v.color.rgb * 2.0, 1.0); }
        case 2u: { return vec4(c.rgb * v.color.rgb, 1.0); }
        case 3u: { return vec4(c.rgb, 1.0); }
        default: { return vec4(c.rgb * v.color.rgb * lit, 1.0); }
    }
}
"#;

#[repr(C)]
#[derive(Clone, Copy)]
struct Inst {
    world: Matrix,
    view: Matrix,
    proj: Matrix,
    bones: [u32; 4], // first register in the bone buffer, index byte order
    alpha: [f32; 4], // alpha below which to discard (-1: none)
    ambient: [f32; 4], // w = 1: unlit
    diffuse: [f32; 4],
    emissive: [f32; 4],
}

#[repr(C)]
#[derive(Clone, Copy)]
struct Light {
    ambient: [f32; 4],
    ambient_peds: [f32; 4],
    sun: [f32; 4],
    sun_dir: [f32; 4],
    mode: [u32; 4],
}

/// The game's lighting for this moment (the timecycle blends it every frame): floats,
/// ambient for props/world at +0x0C, characters +0x18, sun colour +0x24, and the
/// direction towards the sun at +0xE4.
const CURRENT_LIGHT: u32 = 0x020C_47A0;

fn game_light() -> Light {
    let f = |o: u32| f32::from_bits(crate::win::read_u32(CURRENT_LIGHT + o).unwrap_or(0));
    let rgb = |o: u32| [f(o), f(o + 4), f(o + 8), 1.0];
    Light { ambient: rgb(0x0C), ambient_peds: rgb(0x18), sun: rgb(0x24), sun_dir: [f(0xE4), f(0xE8), f(0xEC), 0.0], mode: [0; 4] }
}

struct GpuBuffer {
    buffer: wgpu::Buffer,
    sig: u64,
}

pub struct Gfx {
    window: *mut c_void,
    parent: *mut c_void,
    size: (u32, u32),
    surface: wgpu::Surface<'static>,
    device: wgpu::Device,
    queue: wgpu::Queue,
    config: wgpu::SurfaceConfiguration,
    depth: wgpu::TextureView,
    /// [static, skinned] x [triangle list, triangle strip] x [depth write, none]
    pipelines: [[[wgpu::RenderPipeline; 2]; 2]; 2],
    layout: wgpu::BindGroupLayout,
    vertices: HashMap<(u32, u32, u32, u32), GpuBuffer>, // (vb, offset, stride, decl): skinned ones have their own layout
    indices: HashMap<u32, GpuBuffer>,
    layouts: HashMap<u32, d3d::Layout>,
    texture_layout: wgpu::BindGroupLayout,
    sampler: wgpu::Sampler,
    /// The game's textures on the GPU, by IDirect3DTexture9; `None` if unreadable.
    textures: HashMap<u32, Option<GpuTexture>>,
    white: wgpu::BindGroup,
    frame: u64,
    reported: std::collections::HashSet<u32>,
    pub skin_swap: bool,
    pub light_mode: u32,
    /// Which texture stage holds the base colour.
    pub base_stage: usize,
}

struct GpuTexture {
    bind: wgpu::BindGroup,
    checked: u64, // frame it was last checked against the game's
    sig: u64,
}

/// A cheap fingerprint of a buffer's contents: size and a few spots, to notice when
/// the game frees one and a new one takes its address.
/// Gamebryo NiAlphaProperty flags: bit 9 alpha test (reference given), bit 0 blending
/// (until there's a blended pass: cut where it's mostly clear).
fn alpha_cut((flags, reference): (u16, u8)) -> f32 {
    if flags & 0x200 != 0 {
        reference as f32 / 255.0
    } else if flags & 1 != 0 {
        0.25
    } else {
        -1.0
    }
}

fn as_bytes<T: Copy>(v: &[T]) -> &[u8] {
    unsafe { std::slice::from_raw_parts(v.as_ptr() as *const u8, std::mem::size_of_val(v)) }
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
        return HTTRANSPARENT; // clicks go to the game
    }
    unsafe { DefWindowProcW(h, m, w, l) }
}

fn client_size(h: *mut c_void) -> (u32, u32) {
    let mut r = RECT::default();
    unsafe { GetClientRect(h, &mut r) };
    (r.right.max(1) as u32, r.bottom.max(1) as u32)
}

impl Gfx {
    pub fn new() -> Result<Gfx, String> {
        let parent = crate::win::read_u32(GAME_HWND).ok_or("no game window")? as usize as *mut c_void;
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

        // Vulkan first: wgpu's D3D12 pipelines fail in a 32-bit process (E_INVALIDARG
        // from CreateGraphicsPipelineState). D3D12 stays as the fallback.
        let mut handle = raw_window_handle::Win32WindowHandle::new(NonZeroIsize::new(window as isize).unwrap());
        handle.hinstance = NonZeroIsize::new(inst as isize);
        let target = || wgpu::SurfaceTargetUnsafe::RawHandle {
            raw_display_handle: Some(raw_window_handle::RawDisplayHandle::Windows(raw_window_handle::WindowsDisplayHandle::new())),
            raw_window_handle: raw_window_handle::RawWindowHandle::Win32(handle),
        };
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
        // the game's textures are mostly DXT1/3/5: keep them compressed on the GPU
        let features = adapter.features() & wgpu::Features::TEXTURE_COMPRESSION_BC;
        let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor { label: Some("bullygfx"), required_features: features, ..Default::default() }))
            .map_err(|e| format!("device: {e}"))?;
        crate::stats::note(&format!("new renderer on {} ({:?})", adapter.get_info().name, adapter.get_info().backend));
        // validation errors are reported, not fatal: the game must keep running
        device.on_uncaptured_error(std::sync::Arc::new(|e: wgpu::Error| {
            static SEEN: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
            if SEEN.fetch_add(1, std::sync::atomic::Ordering::Relaxed) < 5 {
                let mut text = format!("{e:?}");
                text.truncate(1500);
                crate::control::log(&format!("wgpu error: {text}"));
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

        let module = device.create_shader_module(wgpu::ShaderModuleDescriptor { label: Some("world"), source: wgpu::ShaderSource::Wgsl(SHADER.into()) });
        let layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: None,
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::VERTEX | wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Storage { read_only: true }, has_dynamic_offset: false, min_binding_size: None },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::VERTEX,
                    ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Storage { read_only: true }, has_dynamic_offset: false, min_binding_size: None },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 2,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Uniform, has_dynamic_offset: false, min_binding_size: None },
                    count: None,
                },
            ],
        });
        let texture_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("texture"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture { sample_type: wgpu::TextureSampleType::Float { filterable: true }, view_dimension: wgpu::TextureViewDimension::D2, multisampled: false },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry { binding: 1, visibility: wgpu::ShaderStages::FRAGMENT, ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering), count: None },
            ],
        });
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("base"),
            address_mode_u: wgpu::AddressMode::Repeat,
            address_mode_v: wgpu::AddressMode::Repeat,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::MipmapFilterMode::Linear,
            anisotropy_clamp: 8,
            ..Default::default()
        });
        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor { label: None, bind_group_layouts: &[Some(&layout), Some(&texture_layout)], immediate_size: 0 });
        let make = |entry: &str, buffer: wgpu::VertexBufferLayout, strip: bool, z_write: bool| {
            device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some(entry),
                layout: Some(&pipeline_layout),
                vertex: wgpu::VertexState { module: &module, entry_point: Some(entry), compilation_options: Default::default(), buffers: &[Some(buffer)] },
                primitive: if strip {
                    wgpu::PrimitiveState {
                        topology: wgpu::PrimitiveTopology::TriangleStrip,
                        strip_index_format: Some(wgpu::IndexFormat::Uint16),
                        cull_mode: None,
                        ..Default::default()
                    }
                } else {
                    wgpu::PrimitiveState { cull_mode: None, ..Default::default() }
                },
                depth_stencil: Some(wgpu::DepthStencilState {
                    format: DEPTH,
                    depth_write_enabled: Some(z_write),
                    depth_compare: Some(wgpu::CompareFunction::Less),
                    stencil: Default::default(),
                    bias: Default::default(),
                }),
                multisample: Default::default(),
                fragment: Some(wgpu::FragmentState { module: &module, entry_point: Some("fs"), compilation_options: Default::default(), targets: &[Some(format.into())] }),
                multiview_mask: None,
                cache: None,
            })
        };
        let static_layout = wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<d3d::Vertex>() as u64,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &wgpu::vertex_attr_array![0 => Float32x3, 1 => Float32x3, 2 => Float32x2, 5 => Unorm8x4],
        };
        let skin_layout = wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<d3d::SkinVertex>() as u64,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &wgpu::vertex_attr_array![0 => Float32x3, 1 => Float32x3, 2 => Float32x2, 5 => Unorm8x4, 3 => Float32x3, 4 => Uint8x4],
        };
        let pipelines = [("vs", &static_layout), ("vs_skin", &skin_layout)].map(|(entry, layout)| {
            [false, true].map(|strip| [true, false].map(|z_write| make(entry, layout.clone(), strip, z_write)))
        });
        let depth = Self::depth_view(&device, size);
        let white = Self::make_texture(&device, &queue, &texture_layout, &sampler, &d3d::Texture { format: 0, width: 1, height: 1, levels: vec![vec![255; 4]] });
        Ok(Gfx {
            window,
            parent,
            size,
            surface,
            device,
            queue,
            config,
            depth,
            pipelines,
            layout,
            vertices: HashMap::new(),
            indices: HashMap::new(),
            layouts: HashMap::new(),
            texture_layout,
            sampler,
            textures: HashMap::new(),
            white,
            frame: 0,
            reported: Default::default(),
            // the game's shader reads the D3DCOLOR indices as .zyxw, which undoes the
            // colour swizzle: bone index k is byte k
            skin_swap: false,
            light_mode: 0,
            base_stage: 0,
        })
    }

    fn depth_view(device: &wgpu::Device, size: (u32, u32)) -> wgpu::TextureView {
        device
            .create_texture(&wgpu::TextureDescriptor {
                label: Some("depth"),
                size: wgpu::Extent3d { width: size.0, height: size.1, depth_or_array_layers: 1 },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: DEPTH,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
                view_formats: &[],
            })
            .create_view(&Default::default())
    }

    pub fn show(&self, on: bool) {
        unsafe { ShowWindow(self.window, if on { SW_SHOWNA } else { SW_HIDE }) };
    }

    /// Keeps the window over the whole game picture.
    fn follow_parent(&mut self) {
        let size = client_size(self.parent);
        if size != self.size {
            self.size = size;
            unsafe { SetWindowPos(self.window, HWND_TOP, 0, 0, size.0 as i32, size.1 as i32, SWP_NOZORDER | SWP_NOACTIVATE) };
            self.config.width = size.0;
            self.config.height = size.1;
            self.surface.configure(&self.device, &self.config);
            self.depth = Self::depth_view(&self.device, size);
        }
    }

    fn upload(&self, label: &str, bytes: &[u8], usage: wgpu::BufferUsages) -> wgpu::Buffer {
        let mut padded = bytes.to_vec();
        padded.resize(bytes.len().div_ceil(4) * 4, 0);
        self.device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some(label), contents: &padded, usage })
    }

    /// The draw's buffers and texture on the GPU, (re)made when the game's changed.
    fn prepare(&mut self, d: &Draw) -> bool {
        if !self.layouts.contains_key(&d.decl) {
            match unsafe { d3d::layout(d.decl) } {
                Some(l) => {
                    self.layouts.insert(d.decl, l);
                }
                None => return false,
            }
        }
        let layout = self.layouts[&d.decl];
        if d.bones.is_some() && (layout.weights.is_none() || layout.indices.is_none()) || layout.position.is_none() {
            if self.reported.insert(d.decl) {
                crate::control::log(&format!("dropping draws with decl {:#x} (skinned {}): {layout:?} stride {}", d.decl, d.bones.is_some(), d.stride));
            }
        }
        let key = (d.vb, d.vb_offset, d.stride, d.decl);
        let have = self.vertices.get(&key).map(|b| b.sig);
        let Some(fresh) = (unsafe {
            d3d::with_buffer(d.vb, |vb| {
                let sig = signature(vb);
                if have == Some(sig) {
                    return Some(None);
                }
                let (offset, stride) = (d.vb_offset as usize, d.stride as usize);
                let bytes: Option<Vec<u8>> = if d.bones.is_some() {
                    d3d::skin_vertices(vb, offset, stride, &layout).map(|v| as_bytes(&v).to_vec())
                } else {
                    d3d::vertices(vb, offset, stride, &layout).map(|v| as_bytes(&v).to_vec())
                };
                bytes.map(|b| Some((b, sig)))
            })
        })
        .flatten() else {
            if self.reported.insert(d.decl ^ 0x8000_0000) {
                crate::control::log(&format!("couldn't read vertices: decl {:#x} skinned {} stride {} {layout:?}", d.decl, d.bones.is_some(), d.stride));
            }
            return false;
        };
        if let Some((bytes, sig)) = fresh {
            let buffer = self.upload("vertices", &bytes, wgpu::BufferUsages::VERTEX);
            self.vertices.insert(key, GpuBuffer { buffer, sig });
        }
        let have = self.indices.get(&d.ib).map(|b| b.sig);
        let Some(fresh) = (unsafe {
            d3d::with_buffer(d.ib, |ib| {
                let sig = signature(ib);
                (have != Some(sig)).then(|| (ib.to_vec(), sig))
            })
        }) else {
            return false;
        };
        if let Some((ib, sig)) = fresh {
            let buffer = self.upload("indices", &ib, wgpu::BufferUsages::INDEX);
            self.indices.insert(d.ib, GpuBuffer { buffer, sig });
        }
        let tex = self.base_of(d);
        if tex != 0 {
            self.prepare_texture(tex);
        }
        true
    }

    /// The base colour texture: the material's base map, else the chosen stage.
    fn base_of(&self, d: &Draw) -> u32 {
        if d.base != 0 { d.base } else { d.textures[self.base_stage] }
    }

    /// How often a cached texture is compared with the game's again (the game frees
    /// textures and new ones can take the same address).
    const TEXTURE_RECHECK: u64 = 120;

    fn prepare_texture(&mut self, tex: u32) {
        let frame = self.frame;
        if let Some(Some(t)) = self.textures.get(&tex)
            && frame - t.checked < Self::TEXTURE_RECHECK
        {
            return;
        }
        if let Some(None) = self.textures.get(&tex) {
            return;
        }
        let Some(data) = (unsafe { d3d::texture(tex) }) else {
            self.textures.insert(tex, None);
            return;
        };
        let sig = signature(&data.levels[0]) ^ ((data.width as u64) << 32 | data.height as u64) ^ data.format as u64;
        if let Some(Some(t)) = self.textures.get_mut(&tex)
            && t.sig == sig
        {
            t.checked = frame;
            return;
        }
        let bind = Self::make_texture(&self.device, &self.queue, &self.texture_layout, &self.sampler, &data);
        self.textures.insert(tex, Some(GpuTexture { bind, checked: frame, sig }));
    }

    fn make_texture(device: &wgpu::Device, queue: &wgpu::Queue, layout: &wgpu::BindGroupLayout, sampler: &wgpu::Sampler, t: &d3d::Texture) -> wgpu::BindGroup {
        let format = match t.format {
            d3d::FMT_DXT1 => wgpu::TextureFormat::Bc1RgbaUnorm,
            d3d::FMT_DXT3 => wgpu::TextureFormat::Bc2RgbaUnorm,
            d3d::FMT_DXT5 => wgpu::TextureFormat::Bc3RgbaUnorm,
            _ => wgpu::TextureFormat::Rgba8Unorm,
        };
        let size = wgpu::Extent3d { width: t.width, height: t.height, depth_or_array_layers: 1 };
        let texture = device.create_texture_with_data(
            queue,
            &wgpu::TextureDescriptor {
                label: Some("game texture"),
                size,
                mip_level_count: t.levels.len() as u32,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format,
                usage: wgpu::TextureUsages::TEXTURE_BINDING,
                view_formats: &[],
            },
            wgpu::util::TextureDataOrder::MipMajor,
            &t.levels.concat(),
        );
        device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: None,
            layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::TextureView(&texture.create_view(&Default::default())) },
                wgpu::BindGroupEntry { binding: 1, resource: wgpu::BindingResource::Sampler(sampler) },
            ],
        })
    }

    fn world_pass(&self, enc: &mut wgpu::CommandEncoder, target: &wgpu::TextureView, bind: &wgpu::BindGroup, draws: &[Draw]) {
        let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("world"),
            color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                view: target,
                depth_slice: None,
                resolve_target: None,
                ops: wgpu::Operations { load: wgpu::LoadOp::Clear(wgpu::Color { r: 0.42, g: 0.58, b: 0.78, a: 1.0 }), store: wgpu::StoreOp::Store },
            })],
            depth_stencil_attachment: Some(wgpu::RenderPassDepthStencilAttachment {
                view: &self.depth,
                depth_ops: Some(wgpu::Operations { load: wgpu::LoadOp::Clear(1.0), store: wgpu::StoreOp::Store }),
                stencil_ops: None,
            }),
            timestamp_writes: None,
            occlusion_query_set: None,
            multiview_mask: None,
        });
        pass.set_bind_group(0, bind, &[]);
        for (i, d) in draws.iter().enumerate() {
            pass.set_pipeline(&self.pipelines[d.bones.is_some() as usize][d.strip as usize][!d.material.z_write as usize]);
            let texture = match self.textures.get(&self.base_of(d)) {
                Some(Some(t)) => &t.bind,
                _ => &self.white,
            };
            pass.set_bind_group(1, texture, &[]);
            let vb = &self.vertices[&(d.vb, d.vb_offset, d.stride, d.decl)].buffer;
            let ib = &self.indices[&d.ib].buffer;
            pass.set_vertex_buffer(0, vb.slice(..));
            pass.set_index_buffer(ib.slice(..), wgpu::IndexFormat::Uint16);
            let first = d.start_index;
            let count = if d.strip { d.triangles + 2 } else { d.triangles * 3 };
            pass.draw_indexed(first..first + count, d.base_vertex, i as u32..i as u32 + 1);
        }
    }

    /// Draws the frame; with `shot`, also renders it off screen and saves a PNG.
    pub fn render(&mut self, draws: &[Draw], shot: Option<&std::path::Path>) {
        self.follow_parent();
        self.frame += 1;
        let draws: Vec<Draw> = draws.iter().filter(|d| self.prepare(d)).cloned().collect();
        let mut bones: Vec<[f32; 4]> = Vec::new();
        let insts: Vec<Inst> = draws
            .iter()
            .map(|d| {
                let first = bones.len() as u32;
                if let Some(b) = &d.bones {
                    bones.extend_from_slice(b);
                }
                let m = &d.material;
                Inst {
                    world: d.world,
                    view: d.view,
                    proj: d.proj,
                    bones: [first, self.skin_swap as u32, 0, 0],
                    alpha: [alpha_cut(d.alpha), 0., 0., 0.],
                    ambient: [m.ambient[0], m.ambient[1], m.ambient[2], if m.z_write { 0.0 } else { 1.0 }],
                    diffuse: [m.diffuse[0], m.diffuse[1], m.diffuse[2], 0.0],
                    emissive: [m.emissive[0], m.emissive[1], m.emissive[2], 0.0],
                }
            })
            .collect();
        if bones.is_empty() {
            bones.push([0.0; 4]);
        }
        let bone_buffer = self.upload("bones", as_bytes(&bones), wgpu::BufferUsages::STORAGE);
        let mut light = game_light();
        light.mode[0] = self.light_mode;
        let light_buffer = self.upload("light", as_bytes(&[light]), wgpu::BufferUsages::UNIFORM);
        let inst_bytes = if insts.is_empty() {
            vec![0u8; std::mem::size_of::<Inst>()]
        } else {
            unsafe { std::slice::from_raw_parts(insts.as_ptr() as *const u8, insts.len() * std::mem::size_of::<Inst>()) }.to_vec()
        };
        let inst_buffer = self.upload("instances", &inst_bytes, wgpu::BufferUsages::STORAGE);
        let bind = self.device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: None,
            layout: &self.layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: inst_buffer.as_entire_binding() },
                wgpu::BindGroupEntry { binding: 1, resource: bone_buffer.as_entire_binding() },
                wgpu::BindGroupEntry { binding: 2, resource: light_buffer.as_entire_binding() },
            ],
        });
        if let Some(path) = shot {
            let r = self.save_shot(&bind, &draws, path);
            crate::control::log(&match r {
                Ok(()) => format!("new shot {} ({} draws)", path.display(), draws.len()),
                Err(e) => format!("new shot failed: {e}"),
            });
        }
        let frame = match self.surface.get_current_texture() {
            wgpu::CurrentSurfaceTexture::Success(t) | wgpu::CurrentSurfaceTexture::Suboptimal(t) => t,
            _ => {
                self.surface.configure(&self.device, &self.config);
                return;
            }
        };
        let view = frame.texture.create_view(&Default::default());
        let mut enc = self.device.create_command_encoder(&Default::default());
        self.world_pass(&mut enc, &view, &bind, &draws);
        self.queue.submit([enc.finish()]);
        self.queue.present(frame);
    }

    fn save_shot(&self, bind: &wgpu::BindGroup, draws: &[Draw], path: &std::path::Path) -> Result<(), String> {
        let (w, h) = self.size;
        let texture = self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some("shot"),
            size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: self.config.format,
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
            view_formats: &[],
        });
        let row = (w * 4).div_ceil(256) * 256;
        let buffer = self.device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("shot"),
            size: (row * h) as u64,
            usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
            mapped_at_creation: false,
        });
        let mut enc = self.device.create_command_encoder(&Default::default());
        self.world_pass(&mut enc, &texture.create_view(&Default::default()), bind, draws);
        enc.copy_texture_to_buffer(
            wgpu::TexelCopyTextureInfo { texture: &texture, mip_level: 0, origin: wgpu::Origin3d::ZERO, aspect: wgpu::TextureAspect::All },
            wgpu::TexelCopyBufferInfo { buffer: &buffer, layout: wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(row), rows_per_image: Some(h) } },
            wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
        );
        self.queue.submit([enc.finish()]);
        buffer.map_async(wgpu::MapMode::Read, .., |_| {});
        self.device.poll(wgpu::PollType::wait_indefinitely()).map_err(|e| e.to_string())?;
        let data = buffer.slice(..).get_mapped_range().map_err(|e| format!("{e:?}"))?;
        let bgra = matches!(self.config.format, wgpu::TextureFormat::Bgra8Unorm | wgpu::TextureFormat::Bgra8UnormSrgb);
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
        crate::control::write_png(path, w, h, &rgba).map_err(|e| e.to_string())
    }
}

/// F8 (or the control script) and the renderer, on the game's render thread.
pub struct Toggle {
    gfx: Option<Result<Gfx, String>>,
    on: bool,
    was_down: bool,
    shot: Option<std::path::PathBuf>,
}

impl Toggle {
    pub const fn new() -> Toggle {
        Toggle { gfx: None, on: false, was_down: false, shot: None }
    }

    pub fn set(&mut self, on: bool) {
        self.on = on;
        if self.gfx.is_none() {
            let g = Gfx::new();
            if let Err(e) = &g {
                crate::control::log(&format!("new renderer failed: {e}"));
            }
            self.gfx = Some(g);
        }
        if let Some(Ok(g)) = &self.gfx {
            g.show(on);
        }
        crate::control::log(&format!("new renderer {}", if on { "on" } else { "off" }));
    }

    pub fn set_light_mode(&mut self, mode: u32) {
        if let Some(Ok(g)) = &mut self.gfx {
            g.light_mode = mode;
        }
        crate::control::log(&format!("light mode {mode}"));
    }

    pub fn set_skin_swap(&mut self, on: bool) {
        if let Some(Ok(g)) = &mut self.gfx {
            g.skin_swap = on;
        }
        crate::control::log(&format!("skin index swizzle {on}"));
    }

    pub fn set_base_stage(&mut self, stage: usize) {
        if let Some(Ok(g)) = &mut self.gfx {
            g.base_stage = stage.min(crate::scene::STAGES - 1);
        }
        crate::control::log(&format!("base texture stage {stage}"));
    }

    pub fn request_shot(&mut self, path: std::path::PathBuf) {
        self.shot = Some(path);
    }

    /// Once per game frame, before the game shows it.
    pub fn frame(&mut self, draws: &[Draw]) {
        let down = unsafe { GetAsyncKeyState(TOGGLE_KEY) } as u16 & 0x8000 != 0;
        if down && !self.was_down {
            self.set(!self.on);
        }
        self.was_down = down;
        let shot = self.shot.take();
        match &mut self.gfx {
            Some(Ok(g)) if self.on || shot.is_some() => g.render(draws, shot.as_deref()),
            _ if shot.is_some() => crate::control::log("new shot: the new renderer isn't running"),
            _ => {}
        }
    }
}
