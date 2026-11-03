//! The finished frame's passes before the picture settings: FXAA (the edges smoothed: the
//! game has no anti-aliasing) and bloom (the bright parts, blurred over a chain of halved
//! levels and added back by the present pass). Both read the replay's copy of the back
//! buffer; the settings `aa` and `bloom` turn them off.

use std::collections::HashMap;

use wgpu::util::DeviceExt;

const VS: &str = r#"
struct Out { @builtin(position) pos: vec4<f32>, @location(0) uv: vec2<f32> };
@vertex fn vs(@builtin(vertex_index) i: u32) -> Out {
    let c = vec2<f32>(f32(i & 1u), f32(i >> 1u));
    var o: Out;
    o.pos = vec4<f32>(c.x * 2.0 - 1.0, 1.0 - c.y * 2.0, 0.0, 1.0);
    o.uv = c;
    return o;
}
@group(0) @binding(0) var src: texture_2d<f32>;
@group(0) @binding(1) var smp: sampler;
struct P { texel: vec4<f32>, params: vec4<f32> };
@group(0) @binding(2) var<uniform> p: P;
fn tap(uv: vec2<f32>) -> vec3<f32> {
    return textureSampleLevel(src, smp, uv, 0.0).rgb;
}
"#;

/// FXAA (after Timothy Lottes' 3.11, quality preset): the luma contrast finds an edge, its
/// direction and its ends along it; the pixel is blended across by how far it sits from
/// the nearer end, and by the sub-pixel contrast.
const FXAA: &str = r#"
fn luma(c: vec3<f32>) -> f32 {
    return dot(c, vec3<f32>(0.299, 0.587, 0.114));
}
@fragment fn fxaa(v: Out) -> @location(0) vec4<f32> {
    let t = p.texel.xy;
    let uv = v.uv;
    let rgb = tap(uv);
    let lM = luma(rgb);
    let lN = luma(tap(uv + vec2<f32>(0.0, -t.y)));
    let lS = luma(tap(uv + vec2<f32>(0.0, t.y)));
    let lE = luma(tap(uv + vec2<f32>(t.x, 0.0)));
    let lW = luma(tap(uv + vec2<f32>(-t.x, 0.0)));
    let hi = max(lM, max(max(lN, lS), max(lE, lW)));
    let lo = min(lM, min(min(lN, lS), min(lE, lW)));
    let range = hi - lo;
    if range < max(0.0312, hi * 0.125) {
        return vec4<f32>(rgb, 1.0);
    }
    let lNW = luma(tap(uv + vec2<f32>(-t.x, -t.y)));
    let lNE = luma(tap(uv + vec2<f32>(t.x, -t.y)));
    let lSW = luma(tap(uv + vec2<f32>(-t.x, t.y)));
    let lSE = luma(tap(uv + vec2<f32>(t.x, t.y)));
    let average = (2.0 * (lN + lS + lE + lW) + lNW + lNE + lSW + lSE) / 12.0;
    let sub = smoothstep(0.0, 1.0, clamp(abs(average - lM) / range, 0.0, 1.0));
    let sub_amount = sub * sub * 0.75;
    let horizontal = abs(lNW + lNE - 2.0 * lN) + 2.0 * abs(lW + lE - 2.0 * lM) + abs(lSW + lSE - 2.0 * lS)
        >= abs(lNW + lSW - 2.0 * lW) + 2.0 * abs(lN + lS - 2.0 * lM) + abs(lNE + lSE - 2.0 * lE);
    var across = select(t.x, t.y, horizontal);
    let l1 = select(lW, lN, horizontal);
    let l2 = select(lE, lS, horizontal);
    let g1 = abs(l1 - lM);
    let g2 = abs(l2 - lM);
    var base = 0.5 * (l2 + lM);
    var gradient = g2 * 0.25;
    if g1 >= g2 {
        across = -across;
        base = 0.5 * (l1 + lM);
        gradient = g1 * 0.25;
    }
    var mid = uv;
    if horizontal {
        mid.y = mid.y + across * 0.5;
    } else {
        mid.x = mid.x + across * 0.5;
    }
    let along = select(vec2<f32>(0.0, t.y), vec2<f32>(t.x, 0.0), horizontal);
    var uv1 = mid - along;
    var uv2 = mid + along;
    var e1 = luma(tap(uv1)) - base;
    var e2 = luma(tap(uv2)) - base;
    var done1 = abs(e1) >= gradient;
    var done2 = abs(e2) >= gradient;
    var steps = array<f32, 10>(1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);
    for (var i = 0; i < 10; i = i + 1) {
        if done1 && done2 {
            break;
        }
        if !done1 {
            uv1 = uv1 - along * steps[i];
            e1 = luma(tap(uv1)) - base;
            done1 = abs(e1) >= gradient;
        }
        if !done2 {
            uv2 = uv2 + along * steps[i];
            e2 = luma(tap(uv2)) - base;
            done2 = abs(e2) >= gradient;
        }
    }
    let d1 = select(uv.y - uv1.y, uv.x - uv1.x, horizontal);
    let d2 = select(uv2.y - uv.y, uv2.x - uv.x, horizontal);
    let nearer1 = d1 < d2;
    let shift = -min(d1, d2) / max(d1 + d2, 1e-6) + 0.5;
    let darker = lM < base;
    let right = select((e2 < 0.0) != darker, (e1 < 0.0) != darker, nearer1);
    let blend = max(select(0.0, shift, right), sub_amount);
    var moved = uv;
    if horizontal {
        moved.y = moved.y + blend * across;
    } else {
        moved.x = moved.x + blend * across;
    }
    return vec4<f32>(tap(moved), 1.0);
}
"#;

/// Bloom: a 13-tap downsample (Jimenez, 2014) to each halved level, the first one keeping
/// only what's above the threshold (with a soft knee); then a 3x3 tent upsample of each
/// level added into the next bigger one.
const BLOOM: &str = r#"
fn down13(uv: vec2<f32>) -> vec3<f32> {
    let t = p.texel.xy;
    let a = tap(uv + t * vec2<f32>(-2.0, -2.0));
    let b = tap(uv + t * vec2<f32>(0.0, -2.0));
    let c = tap(uv + t * vec2<f32>(2.0, -2.0));
    let d = tap(uv + t * vec2<f32>(-1.0, -1.0));
    let e = tap(uv + t * vec2<f32>(1.0, -1.0));
    let f = tap(uv + t * vec2<f32>(-2.0, 0.0));
    let g = tap(uv);
    let h = tap(uv + t * vec2<f32>(2.0, 0.0));
    let i = tap(uv + t * vec2<f32>(-1.0, 1.0));
    let j = tap(uv + t * vec2<f32>(1.0, 1.0));
    let k = tap(uv + t * vec2<f32>(-2.0, 2.0));
    let l = tap(uv + t * vec2<f32>(0.0, 2.0));
    let m = tap(uv + t * vec2<f32>(2.0, 2.0));
    return (d + e + i + j) * 0.125 + (a + b + f + g + b + c + g + h + f + g + k + l + g + h + l + m) * 0.03125;
}
@fragment fn prefilter(v: Out) -> @location(0) vec4<f32> {
    let c = down13(v.uv);
    let bright = max(c.r, max(c.g, c.b));
    let threshold = p.params.x;
    let knee = threshold * 0.5 + 1e-4;
    var soft = clamp(bright - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    let w = max(soft, bright - threshold) / max(bright, 1e-4);
    return vec4<f32>(c * w, 1.0);
}
@fragment fn down(v: Out) -> @location(0) vec4<f32> {
    return vec4<f32>(down13(v.uv), 1.0);
}
@fragment fn up(v: Out) -> @location(0) vec4<f32> {
    let t = p.texel.xy;
    let s = tap(v.uv + vec2<f32>(-t.x, -t.y)) + tap(v.uv + vec2<f32>(t.x, -t.y)) + tap(v.uv + vec2<f32>(-t.x, t.y)) + tap(v.uv + vec2<f32>(t.x, t.y))
        + (tap(v.uv + vec2<f32>(0.0, -t.y)) + tap(v.uv + vec2<f32>(0.0, t.y)) + tap(v.uv + vec2<f32>(-t.x, 0.0)) + tap(v.uv + vec2<f32>(t.x, 0.0))) * 2.0
        + tap(v.uv) * 4.0;
    return vec4<f32>(s / 16.0, 1.0);
}
"#;

const LEVELS: usize = 5;
const BLOOM_FORMAT: wgpu::TextureFormat = wgpu::TextureFormat::Rgba16Float;

struct Targets {
    size: (u32, u32),
    format: wgpu::TextureFormat,
    aa: wgpu::TextureView,
    levels: Vec<(wgpu::TextureView, u32, u32)>,
}

pub struct Post {
    layout: wgpu::BindGroupLayout,
    pipeline_layout: wgpu::PipelineLayout,
    fxaa: wgpu::ShaderModule,
    bloom: wgpu::ShaderModule,
    pipelines: HashMap<(&'static str, wgpu::TextureFormat, bool), wgpu::RenderPipeline>,
    pub sampler: wgpu::Sampler,
    targets: Option<Targets>,
    /// Bound as the bloom when it's off.
    pub black: wgpu::TextureView,
}

impl Post {
    pub fn new(device: &wgpu::Device, queue: &wgpu::Queue) -> Post {
        let layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("post"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture { sample_type: wgpu::TextureSampleType::Float { filterable: true }, view_dimension: wgpu::TextureViewDimension::D2, multisampled: false },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry { binding: 1, visibility: wgpu::ShaderStages::FRAGMENT, ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering), count: None },
                wgpu::BindGroupLayoutEntry {
                    binding: 2,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Uniform, has_dynamic_offset: false, min_binding_size: None },
                    count: None,
                },
            ],
        });
        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor { label: Some("post"), bind_group_layouts: &[Some(&layout)], immediate_size: 0 });
        let module = |label: &str, body: &str| device.create_shader_module(wgpu::ShaderModuleDescriptor { label: Some(label), source: wgpu::ShaderSource::Wgsl(format!("{VS}{body}").into()) });
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("post"),
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });
        let black = device
            .create_texture_with_data(
                queue,
                &wgpu::TextureDescriptor {
                    label: Some("no bloom"),
                    size: wgpu::Extent3d { width: 1, height: 1, depth_or_array_layers: 1 },
                    mip_level_count: 1,
                    sample_count: 1,
                    dimension: wgpu::TextureDimension::D2,
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    usage: wgpu::TextureUsages::TEXTURE_BINDING,
                    view_formats: &[],
                },
                wgpu::util::TextureDataOrder::LayerMajor,
                &[0, 0, 0, 255],
            )
            .create_view(&Default::default());
        Post { fxaa: module("fxaa", FXAA), bloom: module("bloom", BLOOM), layout, pipeline_layout, pipelines: HashMap::new(), sampler, targets: None, black }
    }

    fn target(device: &wgpu::Device, label: &str, w: u32, h: u32, format: wgpu::TextureFormat) -> wgpu::TextureView {
        device
            .create_texture(&wgpu::TextureDescriptor {
                label: Some(label),
                size: wgpu::Extent3d { width: w.max(1), height: h.max(1), depth_or_array_layers: 1 },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::TEXTURE_BINDING,
                view_formats: &[],
            })
            .create_view(&Default::default())
    }

    fn pipeline(&mut self, device: &wgpu::Device, entry: &'static str, format: wgpu::TextureFormat, additive: bool) -> wgpu::RenderPipeline {
        if let Some(p) = self.pipelines.get(&(entry, format, additive)) {
            return p.clone();
        }
        let module = if entry == "fxaa" { &self.fxaa } else { &self.bloom };
        let blend = additive.then_some(wgpu::BlendState {
            color: wgpu::BlendComponent { src_factor: wgpu::BlendFactor::One, dst_factor: wgpu::BlendFactor::One, operation: wgpu::BlendOperation::Add },
            alpha: wgpu::BlendComponent::REPLACE,
        });
        let p = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some(entry),
            layout: Some(&self.pipeline_layout),
            vertex: wgpu::VertexState { module, entry_point: Some("vs"), compilation_options: Default::default(), buffers: &[] },
            primitive: wgpu::PrimitiveState { topology: wgpu::PrimitiveTopology::TriangleStrip, ..Default::default() },
            depth_stencil: None,
            multisample: Default::default(),
            fragment: Some(wgpu::FragmentState {
                module,
                entry_point: Some(entry),
                compilation_options: Default::default(),
                targets: &[Some(wgpu::ColorTargetState { format, blend, write_mask: wgpu::ColorWrites::ALL })],
            }),
            multiview_mask: None,
            cache: None,
        });
        self.pipelines.insert((entry, format, additive), p.clone());
        p
    }

    #[allow(clippy::too_many_arguments)]
    fn pass(&mut self, device: &wgpu::Device, enc: &mut wgpu::CommandEncoder, entry: &'static str, src: &wgpu::TextureView, src_size: (u32, u32), dst: &wgpu::TextureView, format: wgpu::TextureFormat, additive: bool, params: [f32; 4]) {
        let pipeline = self.pipeline(device, entry, format, additive);
        let data: [f32; 8] = [1.0 / src_size.0.max(1) as f32, 1.0 / src_size.1.max(1) as f32, 0.0, 0.0, params[0], params[1], params[2], params[3]];
        let buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some(entry), contents: &data.map(f32::to_le_bytes).concat(), usage: wgpu::BufferUsages::UNIFORM });
        let bind = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some(entry),
            layout: &self.layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::TextureView(src) },
                wgpu::BindGroupEntry { binding: 1, resource: wgpu::BindingResource::Sampler(&self.sampler) },
                wgpu::BindGroupEntry { binding: 2, resource: buffer.as_entire_binding() },
            ],
        });
        let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some(entry),
            color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                view: dst,
                depth_slice: None,
                resolve_target: None,
                ops: wgpu::Operations { load: if additive { wgpu::LoadOp::Load } else { wgpu::LoadOp::Clear(wgpu::Color::BLACK) }, store: wgpu::StoreOp::Store },
            })],
            depth_stencil_attachment: None,
            timestamp_writes: None,
            occlusion_query_set: None,
            multiview_mask: None,
        });
        pass.set_pipeline(&pipeline);
        pass.set_bind_group(0, &bind, &[]);
        pass.draw(0..4, 0..1);
    }

    /// The frame through FXAA (when `aa`) and its bloom (when `bloom` > 0, above
    /// `threshold`): the image to present and the bloom to add (black when off).
    #[allow(clippy::too_many_arguments)]
    pub fn run(
        &mut self,
        device: &wgpu::Device,
        enc: &mut wgpu::CommandEncoder,
        src: &wgpu::TextureView,
        size: (u32, u32),
        format: wgpu::TextureFormat,
        aa: bool,
        bloom: f32,
        threshold: f32,
    ) -> (wgpu::TextureView, wgpu::TextureView) {
        if !aa && bloom <= 0.0 {
            return (src.clone(), self.black.clone());
        }
        if self.targets.as_ref().is_none_or(|t| t.size != size || t.format != format) {
            let mut levels = Vec::new();
            let (mut w, mut h) = size;
            for i in 0..LEVELS {
                w = (w / 2).max(1);
                h = (h / 2).max(1);
                levels.push((Self::target(device, &format!("bloom {i}"), w, h, BLOOM_FORMAT), w, h));
            }
            self.targets = Some(Targets { size, format, aa: Self::target(device, "fxaa", size.0, size.1, format), levels });
        }
        let t = self.targets.as_ref().unwrap();
        let (aa_view, levels) = (t.aa.clone(), t.levels.clone());
        let image = if aa {
            self.pass(device, enc, "fxaa", src, size, &aa_view, format, false, [0.0; 4]);
            aa_view
        } else {
            src.clone()
        };
        if bloom <= 0.0 {
            return (image, self.black.clone());
        }
        // down: the image's bright parts to the first level, then each to the next
        self.pass(device, enc, "prefilter", &image, size, &levels[0].0, BLOOM_FORMAT, false, [threshold, 0.0, 0.0, 0.0]);
        for i in 1..levels.len() {
            let (from, w, h) = levels[i - 1].clone();
            self.pass(device, enc, "down", &from, (w, h), &levels[i].0, BLOOM_FORMAT, false, [0.0; 4]);
        }
        // up: each level added into the bigger one
        for i in (1..levels.len()).rev() {
            let (from, w, h) = levels[i].clone();
            self.pass(device, enc, "up", &from, (w, h), &levels[i - 1].0, BLOOM_FORMAT, true, [0.0; 4]);
        }
        (image, levels[0].0.clone())
    }
}
