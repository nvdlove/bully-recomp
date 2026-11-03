//! Seamless doors' doorways (bullyre_x64's SeamlessDoors.cpp): another area drawn through an
//! open door. The game sends the doorway's corners and the camera they're seen from
//! (bullygfx_portal), then draws the other side from a camera moved through the door, with
//! render state 196 set to PORTAL_DRAWS (replay.rs keeps those draws to stencil 1; PORTAL_AROUND_DRAWS
//! to stencil 1 or 2).
//!
//! Here the doorway is marked where it is seen (PORTAL_IN): the stencil cleared to 0, set to 1
//! where the doorway passes the depth test against the scene so far (what stands in front of it,
//! and a shut door, keep it hidden), and the depth there pushed to the far plane, so the other
//! side is drawn there as if nothing had been.
//!
//! PORTAL_AROUND, once the player is through a door and the camera, behind them, isn't yet: the
//! scene drawn is the new area's, and the area the camera is still in goes around the doorway.
//! The stencil is cleared to 0, made 2 over the whole doorway (its depth there the nearer of the
//! scene's and the doorway's), then 1 everywhere else with the depth there pushed to the far
//! plane. The camera's area is drawn with PORTAL_AROUND_DRAWS (where the stencil is 1 or 2,
//! depth-tested: around the doorway, and in it only what's in front of it), its sky with
//! PORTAL_DRAWS (only around). The doorway's corners and the camera are then the camera's area's
//! (the camera the next draws are made with: the depth is theirs).

use crate::capture::Camera;

/// The modes (bullygfx_portal2).
pub const PORTAL_IN: u32 = 0;
pub const PORTAL_AROUND: u32 = 1;

const SHADER: &str = r#"
struct P {
    corners: array<vec4<f32>, 4>,
    eye: vec4<f32>,
    dir: vec4<f32>,
    up: vec4<f32>,
    right: vec4<f32>,
    lrtb: vec4<f32>,   // the frustum's left, right, top, bottom (tangents at distance 1)
    nf: vec4<f32>,     // near, far
};
@group(0) @binding(0) var<uniform> p: P;

@vertex fn full(@builtin(vertex_index) i: u32) -> @builtin(position) vec4<f32> {
    let c = vec2<f32>(f32((i << 1u) & 2u), f32(i & 2u));
    return vec4<f32>(c * 2.0 - 1.0, 0.0, 1.0);
}

// the whole screen at the far plane
@vertex fn full_far(@builtin(vertex_index) i: u32) -> @builtin(position) vec4<f32> {
    let c = vec2<f32>(f32((i << 1u) & 2u), f32(i & 2u));
    return vec4<f32>(c * 2.0 - 1.0, 1.0, 1.0);
}

// a corner in clip space, as D3D projects it for the game's camera
fn clip(i: u32) -> vec4<f32> {
    var order = array<u32, 6>(0u, 1u, 2u, 0u, 2u, 3u);
    let w = p.corners[order[i]].xyz - p.eye.xyz;
    let v = vec3<f32>(dot(w, p.right.xyz), dot(w, p.up.xyz), dot(w, p.dir.xyz));
    let l = p.lrtb.x;
    let r = p.lrtb.y;
    let t = p.lrtb.z;
    let b = p.lrtb.w;
    let n = p.nf.x;
    let f = p.nf.y;
    return vec4<f32>((2.0 * v.x - (r + l) * v.z) / (r - l), (2.0 * v.y - (t + b) * v.z) / (t - b), f * (v.z - n) / (f - n), v.z);
}

@vertex fn quad(@builtin(vertex_index) i: u32) -> @builtin(position) vec4<f32> {
    return clip(i);
}

// (PORTAL_AROUND) the same, not cut by the near plane: the camera may be right by the doorway,
// which then fills the view (its depth there no nearer than the near plane)
@vertex fn quad_near(@builtin(vertex_index) i: u32) -> @builtin(position) vec4<f32> {
    let c = clip(i);
    return vec4<f32>(c.xy, max(c.z, 0.0), c.w);
}

// the same at the far plane (depth 1)
@vertex fn quad_far(@builtin(vertex_index) i: u32) -> @builtin(position) vec4<f32> {
    let c = clip(i);
    return vec4<f32>(c.xy, c.w, c.w);
}
"#;

pub struct Portal {
    layout: wgpu::BindGroupLayout,
    /// By depth format: the stencil cleared, the doorway marked, its depth pushed back; then
    /// PORTAL_AROUND's: the doorway marked 2 (its depth the nearer), the rest 1 at the far plane.
    pipelines: std::collections::HashMap<wgpu::TextureFormat, [wgpu::RenderPipeline; 5]>,
}

impl Portal {
    pub fn new(device: &wgpu::Device) -> Portal {
        let layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("portal"),
            entries: &[wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::VERTEX,
                ty: wgpu::BindingType::Buffer { ty: wgpu::BufferBindingType::Uniform, has_dynamic_offset: false, min_binding_size: None },
                count: None,
            }],
        });
        Portal { layout, pipelines: Default::default() }
    }

    fn make(&self, device: &wgpu::Device, format: wgpu::TextureFormat) -> [wgpu::RenderPipeline; 5] {
        let module = device.create_shader_module(wgpu::ShaderModuleDescriptor { label: Some("portal"), source: wgpu::ShaderSource::Wgsl(SHADER.into()) });
        let layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor { label: None, bind_group_layouts: &[Some(&self.layout)], immediate_size: 0 });
        let stencil_ops = |compare, pass, depth_fail, write| {
            let face = wgpu::StencilFaceState { compare, fail_op: wgpu::StencilOperation::Keep, depth_fail_op: depth_fail, pass_op: pass };
            wgpu::StencilState { front: face, back: face, read_mask: 0xFF, write_mask: write }
        };
        let stencil = |compare, pass, write| stencil_ops(compare, pass, wgpu::StencilOperation::Keep, write);
        let pipeline = |entry: &str, depth_compare, depth_write, stencil| {
            device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("portal"),
                layout: Some(&layout),
                vertex: wgpu::VertexState { module: &module, entry_point: Some(entry), compilation_options: Default::default(), buffers: &[] },
                primitive: Default::default(),
                depth_stencil: Some(wgpu::DepthStencilState {
                    format,
                    depth_write_enabled: Some(depth_write),
                    depth_compare: Some(depth_compare),
                    stencil,
                    bias: Default::default(),
                }),
                multisample: Default::default(),
                fragment: None,
                multiview_mask: None,
                cache: None,
            })
        };
        use wgpu::{CompareFunction as C, StencilOperation as S};
        [
            pipeline("full", C::Always, false, stencil(C::Always, S::Replace, 0xFF)),
            pipeline("quad", C::LessEqual, false, stencil(C::Always, S::Replace, 0xFF)),
            pipeline("quad_far", C::Always, true, stencil(C::Equal, S::Keep, 0)),
            // (PORTAL_AROUND) the whole doorway 2, its depth where it's nearer; then the 0s made 1 at the far plane
            pipeline("quad_near", C::LessEqual, true, stencil_ops(C::Always, S::Replace, S::Replace, 0xFF)),
            pipeline("full_far", C::Always, true, stencil(C::NotEqual, S::IncrementClamp, 0xFF)),
        ]
    }

    /// The doorway marked in `depth` (the scene's depth-stencil attachment view), as `mode` has it.
    pub fn mark(&mut self, device: &wgpu::Device, enc: &mut wgpu::CommandEncoder, depth: &wgpu::TextureView, format: wgpu::TextureFormat, corners: &[[f32; 3]; 4], cam: &Camera, mode: u32) {
        let frustum = &cam.frustum;
        if !self.pipelines.contains_key(&format) {
            let made = self.make(device, format);
            self.pipelines.insert(format, made);
        }
        let mut params = [0f32; 4 * 4 + 4 * 4 + 8];
        for (i, c) in corners.iter().enumerate() {
            params[i * 4..i * 4 + 3].copy_from_slice(c);
            params[i * 4 + 3] = 1.0;
        }
        for (i, v) in [cam.eye, cam.dir, cam.up, cam.right].iter().enumerate() {
            params[16 + i * 4..16 + i * 4 + 3].copy_from_slice(v);
        }
        params[32..36].copy_from_slice(&frustum[0..4]);
        params[36] = frustum[4];
        params[37] = frustum[5];
        use wgpu::util::DeviceExt;
        let buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor { label: Some("portal"), contents: &params.map(f32::to_le_bytes).concat(), usage: wgpu::BufferUsages::UNIFORM });
        let bind = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: None,
            layout: &self.layout,
            entries: &[wgpu::BindGroupEntry { binding: 0, resource: buffer.as_entire_binding() }],
        });
        let [clear, quad, far, around_quad, around_rest] = &self.pipelines[&format];
        let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
            label: Some("portal"),
            color_attachments: &[],
            depth_stencil_attachment: Some(wgpu::RenderPassDepthStencilAttachment {
                view: depth,
                depth_ops: Some(wgpu::Operations { load: wgpu::LoadOp::Load, store: wgpu::StoreOp::Store }),
                stencil_ops: Some(wgpu::Operations { load: wgpu::LoadOp::Load, store: wgpu::StoreOp::Store }),
            }),
            timestamp_writes: None,
            occlusion_query_set: None,
            multiview_mask: None,
        });
        pass.set_bind_group(0, &bind, &[]);
        pass.set_pipeline(clear);
        pass.set_stencil_reference(0);
        pass.draw(0..3, 0..1);
        if mode == PORTAL_AROUND {
            pass.set_pipeline(around_quad);
            pass.set_stencil_reference(2);
            pass.draw(0..6, 0..1);
            pass.set_pipeline(around_rest);
            pass.draw(0..3, 0..1);
            return;
        }
        pass.set_pipeline(quad);
        pass.set_stencil_reference(1);
        pass.draw(0..6, 0..1);
        pass.set_pipeline(far);
        pass.draw(0..6, 0..1);
    }
}
