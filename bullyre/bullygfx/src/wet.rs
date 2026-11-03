//! Wet streets: after the game has drawn its 3D scene into the back buffer (and before
//! its post effects copy it), surfaces facing up get darker and reflective, with
//! screen-space reflections ray-marched through the depth buffer, puddles and
//! animated ripples. On when it rains (weather 2), or always, from the customiser.
//!
//! The camera comes from the game's SetCameraData (Gamebryo: location, direction, up,
//! right, frustum as tangents at distance 1, near, far), so view-space positions are
//! rebuilt from the depth buffer: z from D3D's perspective depth, x/y from the frustum.

use crate::capture::Camera;

const SHADER: &str = r#"
@group(0) @binding(0) var scene: texture_2d<f32>;
@group(0) @binding(1) var depth: texture_depth_2d;
@group(0) @binding(2) var samp: sampler;
struct P {
    proj: vec4<f32>,  // tan half-width, tan half-height, near, far
    right: vec4<f32>, // camera basis in the world; right.w = time
    up: vec4<f32>,    // up.w = strength
    dir: vec4<f32>,
    eye: vec4<f32>,
    size: vec4<f32>,  // width, height, 1/width, 1/height
};
@group(0) @binding(3) var<uniform> p: P;

@vertex fn vs(@builtin(vertex_index) i: u32) -> @builtin(position) vec4<f32> {
    let c = vec2<f32>(f32((i << 1u) & 2u), f32(i & 2u));
    return vec4<f32>(c * 2.0 - 1.0, 0.0, 1.0);
}

fn view_z(d: f32) -> f32 {
    let n = p.proj.z;
    let f = p.proj.w;
    return f * n / (f - d * (f - n));
}
fn view_pos(uv: vec2<f32>, d: f32) -> vec3<f32> {
    let z = view_z(d);
    let ndc = vec2<f32>(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    return vec3<f32>(ndc.x * z * p.proj.x, ndc.y * z * p.proj.y, z);
}
fn project(v: vec3<f32>) -> vec2<f32> {
    let ndc = vec2<f32>(v.x / (v.z * p.proj.x), v.y / (v.z * p.proj.y));
    return vec2<f32>(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}
fn depth_at(uv: vec2<f32>) -> f32 {
    let q = vec2<i32>(clamp(uv, vec2<f32>(0.0), vec2<f32>(0.9999)) * p.size.xy);
    return textureLoad(depth, q, 0);
}
fn hash(q: vec2<f32>) -> f32 {
    return fract(sin(dot(q, vec2<f32>(127.1, 311.7))) * 43758.5453);
}
// 2D simplex noise (Ashima Arts / Stefan Gustavson, MIT), -1..1: round shapes, no grid
fn mod289_3(x: vec3<f32>) -> vec3<f32> { return x - floor(x * (1.0 / 289.0)) * 289.0; }
fn mod289_2(x: vec2<f32>) -> vec2<f32> { return x - floor(x * (1.0 / 289.0)) * 289.0; }
fn permute(x: vec3<f32>) -> vec3<f32> { return mod289_3(((x * 34.0) + 1.0) * x); }
fn snoise(v: vec2<f32>) -> f32 {
    let C = vec4<f32>(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
    var i = floor(v + dot(v, C.yy));
    let x0 = v - i + dot(i, C.xx);
    var i1 = vec2<f32>(0.0, 1.0);
    if x0.x > x0.y {
        i1 = vec2<f32>(1.0, 0.0);
    }
    var x12 = x0.xyxy + C.xxzz;
    x12 = vec4<f32>(x12.xy - i1, x12.zw);
    i = mod289_2(i);
    let pm = permute(permute(i.y + vec3<f32>(0.0, i1.y, 1.0)) + i.x + vec3<f32>(0.0, i1.x, 1.0));
    var m = max(vec3<f32>(0.5) - vec3<f32>(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), vec3<f32>(0.0));
    m = m * m;
    m = m * m;
    let x = 2.0 * fract(pm * C.www) - 1.0;
    let h = abs(x) - 0.5;
    let ox = floor(x + 0.5);
    let a0 = x - ox;
    m = m * (1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h));
    let g = vec3<f32>(a0.x * x0.x + h.x * x0.y, a0.y * x12.x + h.y * x12.y, a0.z * x12.z + h.z * x12.w);
    return 130.0 * dot(m, g);
}
fn noise(q: vec2<f32>) -> f32 {
    let i = floor(q);
    let f = fract(q);
    let u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2<f32>(1.0, 0.0)), u.x), mix(hash(i + vec2<f32>(0.0, 1.0)), hash(i + vec2<f32>(1.0, 1.0)), u.x), u.y);
}

@fragment fn fs(@builtin(position) fc: vec4<f32>) -> @location(0) vec4<f32> {
    let px = vec2<i32>(fc.xy);
    let base = textureLoad(scene, px, 0);
    let d = textureLoad(depth, px, 0);
    let uv = fc.xy * p.size.zw;
    let pos = view_pos(uv, d);
    // the surface's normal, from neighbouring pixels (derivatives before any early out)
    var n = normalize(cross(dpdy(pos), dpdx(pos)));
    if d >= 0.99999 {
        return base; // sky
    }
    if dot(n, pos) > 0.0 {
        n = -n;
    }
    let up_view = normalize(vec3<f32>(p.right.z, p.up.z, p.dir.z)); // the world's up (z) in view space
    let flat_ = smoothstep(0.72, 0.94, dot(n, up_view));
    let world = p.eye.xyz + pos.x * p.right.xyz + pos.y * p.up.xyz + pos.z * p.dir.xyz;
    // how long it has rained: the ground darkens first, then puddles form in spots and
    // grow (the threshold drops); after the rain it runs backwards
    let level = p.eye.w;
    let warp = vec2<f32>(snoise(world.xy * 0.045), snoise(world.xy * 0.045 + 17.3)) * 3.5;
    let field = (snoise((world.xy + warp) * 0.11) * 0.65 + snoise((world.xy + warp) * 0.42 + 5.1) * 0.35) * 0.5 + 0.5;
    let th = mix(0.97, 0.52, smoothstep(0.15, 1.0, level));
    let puddle = smoothstep(th, th + 0.1, field);
    let fade = 1.0 - smoothstep(60.0, 140.0, pos.z);
    let wet = flat_ * p.up.w * smoothstep(0.0, 0.35, level) * mix(0.5, 1.0, puddle) * fade;
    if p.dir.w > 0.5 {
        // debug: red = facing up, green = the world's up in view space (y), blue = depth
        return vec4<f32>(flat_, up_view.y * 0.5 + 0.5, fract(pos.z / 20.0), 1.0);
    }
    if wet <= 0.002 {
        return base;
    }
    // ripples: rain drops rippling the puddles
    let t = p.right.w;
    let rq = world.xy * 7.0;
    let rip = vec2<f32>(snoise(rq + vec2<f32>(t * 1.7, t * 0.9)), snoise(rq + vec2<f32>(-t * 1.1, t * 1.5) + 31.0)) * 0.5;
    let side = normalize(cross(up_view, vec3<f32>(0.0, 0.0, 1.0)) + vec3<f32>(1e-4, 0.0, 0.0));
    let fwd = normalize(cross(side, up_view));
    // puddles: calm, a few rain rings; damp ground: rough
    let n2 = normalize(n + (side * rip.x + fwd * rip.y) * mix(0.28, 0.035, puddle));
    // reflect the view ray and march it through the depth buffer
    let v = normalize(pos);
    let r = reflect(v, n2);
    var q = pos;
    var step_ = 0.08 + pos.z * 0.012;
    var hit = false;
    var huv = vec2<f32>(0.0);
    for (var i = 0; i < 48; i = i + 1) {
        q = q + r * step_;
        step_ = step_ * 1.07;
        if q.z <= p.proj.z {
            break;
        }
        let s = project(q);
        if s.x < 0.0 || s.x > 1.0 || s.y < 0.0 || s.y > 1.0 {
            break;
        }
        let sz = view_z(depth_at(s));
        if q.z > sz && q.z - sz < max(0.35, step_ * 2.5) {
            hit = true;
            huv = s;
            break;
        }
    }
    // missed: the sky (the top of the frame above this point), dimmed, with the
    // ground's own colour mixed in so wide open puddles don't go flat
    let sky = (textureSampleLevel(scene, samp, vec2<f32>(uv.x, 0.06), 0.0).rgb + textureSampleLevel(scene, samp, vec2<f32>(1.0 - uv.x, 0.12), 0.0).rgb) * 0.5;
    var refl = mix(sky * 0.6, base.rgb * 0.5, 0.35);
    if hit {
        let edge = min(min(huv.x, 1.0 - huv.x), min(huv.y, 1.0 - huv.y));
        refl = mix(refl, textureSampleLevel(scene, samp, huv, 0.0).rgb, smoothstep(0.0, 0.08, edge));
    }
    let fres = 0.04 + 0.96 * pow(1.0 - max(dot(-v, n2), 0.0), 5.0);
    // puddles are near mirrors; damp ground only glints at grazing angles
    let reflectance = mix(0.12 + 0.6 * fres, 0.72 + 0.28 * fres, puddle);
    let k = clamp(wet * reflectance, 0.0, 0.95);
    // darker when wet: less so when the scene is already dark (night)
    let lum = dot(base.rgb, vec3<f32>(0.2126, 0.7152, 0.0722));
    let darker = base.rgb * mix(1.0, mix(0.6, 0.45, puddle), wet * smoothstep(0.03, 0.25, lum));
    return vec4<f32>(mix(darker, refl * 1.05, k), base.a);
}
"#;

/// Testing: show the wet mask instead.
pub static DEBUG: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);

pub struct Wet {
    layout: wgpu::BindGroupLayout,
    pipelines: std::collections::HashMap<wgpu::TextureFormat, wgpu::RenderPipeline>,
    sampler: wgpu::Sampler,
    /// The back buffer as it was (the pass reads it while writing the real one).
    copy: Option<(wgpu::Texture, wgpu::TextureView, u32, u32, wgpu::TextureFormat)>,
    start: std::time::Instant,
}

impl Wet {
    pub fn new(device: &wgpu::Device) -> Wet {
        let layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("wet"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture { sample_type: wgpu::TextureSampleType::Float { filterable: true }, view_dimension: wgpu::TextureViewDimension::D2, multisampled: false },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture { sample_type: wgpu::TextureSampleType::Depth, view_dimension: wgpu::TextureViewDimension::D2, multisampled: false },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry { binding: 2, visibility: wgpu::ShaderStages::FRAGMENT, ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering), count: None },
                wgpu::BindGroupLayoutEntry {
                    binding: 3,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Uniform, has_dynamic_offset: false, min_binding_size: None },
                    count: None,
                },
            ],
        });
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("wet"),
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });
        Wet { layout, pipelines: Default::default(), sampler, copy: None, start: std::time::Instant::now() }
    }

    /// Makes `target` (the back buffer: its texture and view) wet, from `depth` (a
    /// depth-only view of the scene's depth buffer, the same size) and the camera.
    #[allow(clippy::too_many_arguments)]
    pub fn pass(
        &mut self,
        device: &wgpu::Device,
        enc: &mut wgpu::CommandEncoder,
        target: (&wgpu::Texture, &wgpu::TextureView, u32, u32, wgpu::TextureFormat),
        depth: &wgpu::TextureView,
        cam: &Camera,
        strength: f32,
        level: f32,
    ) {
        let (texture, view, w, h, format) = target;
        if self.copy.as_ref().is_none_or(|c| (c.2, c.3, c.4) != (w, h, format)) {
            let t = device.create_texture(&wgpu::TextureDescriptor {
                label: Some("wet source"),
                size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format,
                usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
                view_formats: &[],
            });
            let v = t.create_view(&Default::default());
            self.copy = Some((t, v, w, h, format));
        }
        let (copy_tex, copy_view, ..) = self.copy.as_ref().unwrap();
        enc.copy_texture_to_texture(texture.as_image_copy(), copy_tex.as_image_copy(), wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 });

        if !self.pipelines.contains_key(&format) {
            let module = device.create_shader_module(wgpu::ShaderModuleDescriptor { label: Some("wet"), source: wgpu::ShaderSource::Wgsl(SHADER.into()) });
            let layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor { label: None, bind_group_layouts: &[Some(&self.layout)], immediate_size: 0 });
            let p = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("wet"),
                layout: Some(&layout),
                vertex: wgpu::VertexState { module: &module, entry_point: Some("vs"), compilation_options: Default::default(), buffers: &[] },
                primitive: Default::default(),
                depth_stencil: None,
                multisample: Default::default(),
                fragment: Some(wgpu::FragmentState { module: &module, entry_point: Some("fs"), compilation_options: Default::default(), targets: &[Some(format.into())] }),
                multiview_mask: None,
                cache: None,
            });
            self.pipelines.insert(format, p);
        }
        let f = cam.frustum;
        let time = self.start.elapsed().as_secs_f32() % 1000.0;
        #[rustfmt::skip]
        let params: [f32; 24] = [
            f[1], f[2], f[4], f[5],
            cam.right[0], cam.right[1], cam.right[2], time,
            cam.up[0], cam.up[1], cam.up[2], strength,
            cam.dir[0], cam.dir[1], cam.dir[2], if DEBUG.load(std::sync::atomic::Ordering::Relaxed) { 1.0 } else { 0.0 },
            cam.eye[0], cam.eye[1], cam.eye[2], level,
            w as f32, h as f32, 1.0 / w as f32, 1.0 / h as f32,
        ];
        use wgpu::util::DeviceExt;
        let buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("wet params"), contents: &params.map(f32::to_le_bytes).concat(), usage: wgpu::BufferUsages::UNIFORM });
        let bind = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: None,
            layout: &self.layout,
            entries: &[
                wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::TextureView(copy_view) },
                wgpu::BindGroupEntry { binding: 1, resource: wgpu::BindingResource::TextureView(depth) },
                wgpu::BindGroupEntry { binding: 2, resource: wgpu::BindingResource::Sampler(&self.sampler) },
                wgpu::BindGroupEntry { binding: 3, resource: buffer.as_entire_binding() },
            ],
        });
        let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("wet"),
            color_attachments: &[Some(wgpu::RenderPassColorAttachment { view, depth_slice: None, resolve_target: None, ops: wgpu::Operations { load: wgpu::LoadOp::Load, store: wgpu::StoreOp::Store } })],
            depth_stencil_attachment: None,
            timestamp_writes: None,
            occlusion_query_set: None,
            multiview_mask: None,
        });
        pass.set_pipeline(&self.pipelines[&format]);
        pass.set_bind_group(0, &bind, &[]);
        pass.draw(0..3, 0..1);
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn shader_validates() {
        let module = naga::front::wgsl::parse_str(super::SHADER).expect("parse");
        naga::valid::Validator::new(naga::valid::ValidationFlags::all(), naga::valid::Capabilities::all()).validate(&module).expect("validate");
    }
}
