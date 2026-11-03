//! The frame as the new renderer sees it, collected from the calls the game makes:
//! the camera (the view and projection SetCameraData sets on the device), and each
//! world mesh draw: Bully's mesh draw (renderer slot 100) gives the world transform,
//! the device state at its DrawIndexedPrimitive gives the buffers.

use std::sync::Mutex;

/// Renderer slots and device methods the capture follows.
pub const BEGIN_FRAME: u32 = 82;
pub const DISPLAY_FRAME: u32 = 84;
pub const BEGIN_TARGET: u32 = 89;
pub const DRAW_MESH: u32 = 100;
pub const DRAW_SKINNED: u32 = 101;
pub const DEV_SET_TRANSFORM: u32 = 44;
pub const DEV_DRAW_INDEXED: u32 = 82;
pub const DEV_SET_DECL: u32 = 87;
pub const DEV_SET_STREAM: u32 = 100;
pub const DEV_SET_INDICES: u32 = 104;
pub const DEV_SET_TEXTURE: u32 = 65;
pub const DEV_SET_VS_CONST: u32 = 94;
pub const DEV_SET_PS_CONST: u32 = 109;
pub const DEV_SET_VS: u32 = 92;
pub const DEV_SET_PS: u32 = 107;
/// DrawIndexedPrimitive's call site inside the skinned draw (0x87E7D0).
const SKINNED_DRAW_SITE: u32 = 0x0087_EA56;
/// The skinning shader's bone palette: `float4x3 mBone[65]` in c0..c194 (its
/// view-projection follows at c195, the camera position at c199).
pub const BONE_REGS: usize = 195;
/// Vertex shader constant registers kept (Direct3D 9 has 256).
const VS_REGS: usize = 256;
/// DrawIndexedPrimitive's call site inside Bully's mesh draw (0x402E90).
const MESH_DRAW_SITE: u32 = 0x0040_309C;

pub const STAGES: usize = 4;
const D3DTS_VIEW: u32 = 2;
const D3DTS_PROJECTION: u32 = 3;
const D3DPT_TRIANGLELIST: u32 = 4;
const D3DPT_TRIANGLESTRIP: u32 = 5;

/// Row-major, row-vector (Direct3D) 4x4.
pub type Matrix = [f32; 16];
pub const IDENTITY: Matrix = [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.];

#[derive(Clone)]
pub struct Draw {
    pub target: u32, // NiRenderTargetGroup it went into
    pub world: Matrix,
    pub view: Matrix,
    pub proj: Matrix,
    /// The textures on stages 0..3 (IDirect3DBaseTexture9), 0 for none.
    pub textures: [u32; STAGES],
    /// The material's base and normal maps (IDirect3DBaseTexture9), 0 for none.
    pub base: u32,
    pub normal: u32,
    /// NiAlphaProperty: flags (bit 0 blend, bit 9 test) and the test reference.
    pub alpha: (u16, u8),
    pub material: Material,
    pub decl: u32,
    pub vb: u32,
    pub vb_offset: u32,
    pub stride: u32,
    pub ib: u32,
    pub base_vertex: i32,
    pub start_index: u32,
    pub triangles: u32,
    /// A triangle strip (NiTriStrips) rather than a list: `triangles + 2` indices.
    pub strip: bool,
    /// Skinned: the bone palette (4x3 skin-to-world matrices as 3 rows each).
    pub bones: Option<std::sync::Arc<Vec<[f32; 4]>>>,
}

struct State {
    target: u32,
    world: Matrix,
    view: Matrix,
    proj: Matrix,
    textures: [u32; STAGES],
    base: u32,
    normal: u32,
    alpha: (u16, u8),
    material: Material,
    decl: u32,
    vb: u32,
    vb_offset: u32,
    stride: u32,
    ib: u32,
    draws: Vec<Draw>,
    /// The vertex shader constants as the device has them now.
    vs: [[f32; 4]; VS_REGS],
    /// Log the next skinned draw in full (for working out its layout).
    dump_skin: bool,
    /// Log a line per skinned draw until the next frame.
    list_skins: u8,
    /// Log the next static mesh draw's shader constants.
    dump_mesh: bool,
    /// The pixel shader constants as the device has them now.
    ps: [[f32; 4]; 32],
    /// The shaders set now (IDirect3DVertexShader9 / IDirect3DPixelShader9).
    vs_shader: u32,
    ps_shader: u32,
    /// Registers the last SetVertexShaderConstantF from c0 set.
    last_c0_count: u32,
}

static STATE: Mutex<State> = Mutex::new(State {
    target: 0,
    world: IDENTITY,
    view: IDENTITY,
    proj: IDENTITY,
    textures: [0; STAGES],
    base: 0,
    normal: 0,
    alpha: (0, 0),
    material: Material::DEFAULT,
    decl: 0,
    vb: 0,
    vb_offset: 0,
    stride: 0,
    ib: 0,
    draws: Vec::new(),
    vs: [[0.0; 4]; VS_REGS],
    dump_skin: false,
    list_skins: 0,
    last_c0_count: 0,
    dump_mesh: false,
    ps: [[0.0; 4]; 32],
    vs_shader: 0,
    ps_shader: 0,
});

pub fn dump_next_mesh() {
    if let Ok(mut s) = STATE.lock() {
        s.dump_mesh = true;
    }
}

pub fn list_skinned() {
    if let Ok(mut s) = STATE.lock() {
        s.list_skins = 2; // this frame's rest is partial: log the whole next one
    }
}

pub fn dump_next_skinned() {
    if let Ok(mut s) = STATE.lock() {
        s.dump_skin = true;
    }
}

/// Gamebryo's NiTransform (rotation 3x3 row-major, translation, scale), applied as
/// `R * (s * v) + T`, as a Direct3D world matrix.
fn world_from_ni(t: *const f32) -> Option<Matrix> {
    let mut v = [0f32; 13];
    if !crate::win::read_bytes(t as u32, bytemuck_bytes(&mut v)) {
        return None;
    }
    let (r, s) = (&v[..9], v[12]);
    Some([
        s * r[0], s * r[3], s * r[6], 0.,
        s * r[1], s * r[4], s * r[7], 0.,
        s * r[2], s * r[5], s * r[8], 0.,
        v[9], v[10], v[11], 1.,
    ])
}

/// NiTexturingProperty's map slots (Gamebryo order: base, dark, detail, gloss, glow,
/// bump, normal, parallax, decal).
const MAP_BASE: u32 = 0;
const MAP_NORMAL: u32 = 6;

/// The Direct3D texture of one of a mesh's material maps, following Gamebryo's
/// objects (read in the game, see NOTES.md): NiGeometry +0xC4 property state, +0x28
/// NiTexturingProperty, +0x20 map array (count u16 at +0x24), map +8 NiSourceTexture,
/// +0x24 NiDX9TextureData, +0x5C IDirect3DBaseTexture9.
fn material_map(geometry: u32, slot: u32) -> u32 {
    use crate::win::read_u32 as r;
    let get = || -> Option<u32> {
        let texturing = r(r(geometry + 0xC4)? + 0x28).filter(|&p| p != 0)?;
        let count = r(texturing + 0x24)? & 0xFFFF;
        if slot >= count {
            return None;
        }
        let map = r(r(texturing + 0x20)? + 4 * slot).filter(|&p| p != 0)?;
        let source = r(map + 8).filter(|&p| p != 0)?;
        let data = r(source + 0x24).filter(|&p| p != 0)?;
        r(data + 0x5C)
    };
    get().unwrap_or(0)
}

/// What a mesh's NiMaterialProperty (property state slot 3, +0x14) and
/// NiZBufferProperty (slot 11, +0x34) say.
#[derive(Clone, Copy, Debug)]
pub struct Material {
    pub ambient: [f32; 3],
    pub diffuse: [f32; 3],
    pub emissive: [f32; 3],
    /// Writes depth (the sky doesn't).
    pub z_write: bool,
}

impl Material {
    pub const DEFAULT: Material = Material { ambient: [1.0; 3], diffuse: [1.0; 3], emissive: [0.0; 3], z_write: true };

    /// NiMaterialProperty: floats from +0x1C: ambient, diffuse, specular, emissive
    /// (RGB each), glossiness, alpha. NiZBufferProperty: flags u16 at +0x18 (bit 0
    /// test, bit 1 write).
    fn of(geometry: u32) -> Material {
        use crate::win::read_u32 as r;
        let mut m = Material::DEFAULT;
        let Some(state) = r(geometry + 0xC4).filter(|&p| p != 0) else { return m };
        if let Some(mat) = r(state + 0x14).filter(|&p| p != 0) {
            let mut v = [0f32; 12];
            if crate::win::read_bytes(mat + 0x1C, bytemuck_bytes(&mut v)) {
                m.ambient = [v[0], v[1], v[2]];
                m.diffuse = [v[3], v[4], v[5]];
                m.emissive = [v[9], v[10], v[11]];
            }
        }
        if let Some(z) = r(state + 0x34).filter(|&p| p != 0).and_then(|z| r(z + 0x18)) {
            m.z_write = z & 2 != 0;
        }
        m
    }
}

/// The mesh's NiAlphaProperty (property state slot 0, +0x08): flags u16 at +0x18,
/// test reference u8 at +0x1A. No property: opaque.
fn alpha_mode(geometry: u32) -> (u16, u8) {
    use crate::win::read_u32 as r;
    let get = || -> Option<(u16, u8)> {
        let alpha = r(r(geometry + 0xC4)? + 0x08).filter(|&p| p != 0)?;
        let v = r(alpha + 0x18)?;
        Some((v as u16, (v >> 16) as u8))
    };
    get().unwrap_or((0, 0))
}

fn bytemuck_bytes(v: &mut [f32]) -> &mut [u8] {
    unsafe { std::slice::from_raw_parts_mut(v.as_mut_ptr() as *mut u8, v.len() * 4) }
}

fn matrix_at(p: u32) -> Option<Matrix> {
    let mut m = [0f32; 16];
    crate::win::read_bytes(p, bytemuck_bytes(&mut m)).then_some(m)
}

/// A renderer (thiscall) call: `this`, the stack arguments.
pub fn renderer_call(id: u32, _this: u32, args: &[u32]) {
    let Ok(mut s) = STATE.lock() else { return };
    match id {
        BEGIN_FRAME => {
            s.draws.clear();
            s.list_skins = s.list_skins.saturating_sub(1);
        }
        BEGIN_TARGET => s.target = args[0],
        DRAW_MESH => {
            if let Some(w) = world_from_ni(args[3] as *const f32) {
                // (the skinned draw has no world transform: its bones are in world space)
                s.world = w;
            }
            s.base = material_map(args[0], MAP_BASE);
            s.normal = material_map(args[0], MAP_NORMAL);
            s.alpha = alpha_mode(args[0]);
            s.material = Material::of(args[0]);
        }
        DRAW_SKINNED => {
            s.base = material_map(args[0], MAP_BASE);
            s.normal = material_map(args[0], MAP_NORMAL);
            s.alpha = alpha_mode(args[0]);
            s.material = Material::of(args[0]);
        }
        _ => {}
    }
}

/// A device (stdcall) call: its arguments after `this`, and where it was called from.
pub fn device_call(method: u32, args: &[u32], ret: u32) {
    let Ok(mut s) = STATE.lock() else { return };
    match method {
        DEV_SET_TRANSFORM => match args[0] {
            D3DTS_VIEW => s.view = matrix_at(args[1]).unwrap_or(s.view),
            D3DTS_PROJECTION => s.proj = matrix_at(args[1]).unwrap_or(s.proj),
            _ => {}
        },
        DEV_SET_DECL => s.decl = args[0],
        DEV_SET_VS => s.vs_shader = args[0],
        DEV_SET_PS => s.ps_shader = args[0],
        DEV_SET_PS_CONST => {
            let (start, count) = (args[0] as usize, args[2] as usize);
            if start < 32 {
                let n = count.min(32 - start);
                let mut buf = vec![0f32; n * 4];
                if crate::win::read_bytes(args[1], bytemuck_bytes(&mut buf)) {
                    for (i, r) in buf.chunks_exact(4).enumerate() {
                        s.ps[start + i] = [r[0], r[1], r[2], r[3]];
                    }
                }
            }
        }
        DEV_SET_VS_CONST => {
            let (start, count) = (args[0] as usize, args[2] as usize);
            if start == 0 {
                s.last_c0_count = count as u32;
            }
            if start < VS_REGS {
                let n = count.min(VS_REGS - start);
                let mut buf = vec![0f32; n * 4];
                if crate::win::read_bytes(args[1], bytemuck_bytes(&mut buf)) {
                    for (i, r) in buf.chunks_exact(4).enumerate() {
                        s.vs[start + i] = [r[0], r[1], r[2], r[3]];
                    }
                }
            }
        }
        DEV_DRAW_INDEXED if ret == SKINNED_DRAW_SITE && matches!(args[0], D3DPT_TRIANGLELIST | D3DPT_TRIANGLESTRIP) => {
            if s.dump_skin {
                s.dump_skin = false;
                dump_skinned(&s, args);
            }
            if s.list_skins == 1 {
                // where the draw's first vertex ends up, with its first bone
                let first = args[1] as usize + args[2] as usize;
                let p = unsafe { crate::d3d::with_buffer(s.vb, |vb| {
                    let at = s.vb_offset as usize + first * s.stride as usize;
                    vb.get(at..at + 12).map(|b| [0, 4, 8].map(|i| f32::from_le_bytes(b[i..i + 4].try_into().unwrap())))
                }) }.flatten().unwrap_or([0.0; 3]);
                let world: Vec<f32> = (0..3).map(|r| { let b = s.vs[r]; b[0] * p[0] + b[1] * p[1] + b[2] * p[2] + b[3] }).collect();
                crate::control::log(&format!("skinned: tris {} verts {} stride {} base {:#x} c0 count {} alpha {:x?} first vertex {:?} -> bone0 {:?}", args[5], args[3], s.stride, s.base, s.last_c0_count, s.alpha, p, world));
            }
            // bone k is registers 3k..3k+2
            let bones = std::sync::Arc::new(s.vs[..BONE_REGS].to_vec());
            let d = Draw {
                target: s.target,
                world: IDENTITY,
                view: s.view,
                proj: s.proj,
                textures: s.textures,
                base: s.base,
                normal: s.normal,
                alpha: s.alpha,
                material: s.material,
                decl: s.decl,
                vb: s.vb,
                vb_offset: s.vb_offset,
                stride: s.stride,
                ib: s.ib,
                base_vertex: args[1] as i32,
                start_index: args[4],
                triangles: args[5],
                strip: args[0] == D3DPT_TRIANGLESTRIP,
                bones: Some(bones),
            };
            if d.vb != 0 && d.ib != 0 && d.decl != 0 && d.triangles > 0 {
                s.draws.push(d);
            }
        }
        DEV_SET_TEXTURE if (args[0] as usize) < STAGES => s.textures[args[0] as usize] = args[1],
        DEV_SET_STREAM if args[0] == 0 => {
            (s.vb, s.vb_offset, s.stride) = (args[1], args[2], args[3]);
        }
        DEV_SET_INDICES => s.ib = args[0],
        DEV_DRAW_INDEXED if ret == MESH_DRAW_SITE && s.dump_mesh && s.material.z_write => {
            s.dump_mesh = false;
            let log = crate::control::log;
            log(&format!("mesh draw {args:x?} material {:?} light block below", s.material));
            for (i, r) in s.vs.iter().enumerate().take(16) {
                log(&format!("  vs c{i}: {:.4} {:.4} {:.4} {:.4}", r[0], r[1], r[2], r[3]));
            }
            for (i, r) in s.ps.iter().enumerate().take(16) {
                log(&format!("  ps c{i}: {:.4} {:.4} {:.4} {:.4}", r[0], r[1], r[2], r[3]));
            }
            log(&format!("  world {:?}", s.world));
            log(&format!("  vertex shader:\n{}", crate::d3d::disassemble(s.vs_shader)));
            log(&format!("  pixel shader:\n{}", crate::d3d::disassemble(s.ps_shader)));
        }
        DEV_DRAW_INDEXED if ret == MESH_DRAW_SITE && matches!(args[0], D3DPT_TRIANGLELIST | D3DPT_TRIANGLESTRIP) => {
            let d = Draw {
                target: s.target,
                world: s.world,
                view: s.view,
                proj: s.proj,
                textures: s.textures,
                base: s.base,
                normal: s.normal,
                alpha: s.alpha,
                material: s.material,
                decl: s.decl,
                vb: s.vb,
                vb_offset: s.vb_offset,
                stride: s.stride,
                ib: s.ib,
                base_vertex: args[1] as i32,
                start_index: args[4],
                triangles: args[5],
                strip: args[0] == D3DPT_TRIANGLESTRIP,
                bones: None,
            };
            if d.vb != 0 && d.ib != 0 && d.decl != 0 && d.triangles > 0 {
                s.draws.push(d);
            }
        }
        _ => {}
    }
}

fn dump_skinned(s: &State, args: &[u32]) {
    let log = crate::control::log;
    log(&format!("skinned draw: {args:x?} decl {:#x} vb {:#x} offset {} stride {} ib {:#x}", s.decl, s.vb, s.vb_offset, s.stride, s.ib));
    log(&format!("  material base {:#x} normal {:#x}, stages {:x?}", s.base, s.normal, s.textures));
    let mut elements = [0u8; 8 * 65];
    let mut n = 65u32;
    type GetDeclaration = unsafe extern "system" fn(usize, *mut u8, *mut u32) -> i32;
    let f: GetDeclaration = unsafe { std::mem::transmute(*(*(s.decl as *const *const usize)).add(4)) };
    if unsafe { f(s.decl as usize, elements.as_mut_ptr(), &mut n) } >= 0 {
        for e in elements.chunks_exact(8).take(n as usize) {
            log(&format!("  element stream {} offset {} type {} usage {} index {}", u16::from_le_bytes([e[0], e[1]]), u16::from_le_bytes([e[2], e[3]]), e[4], e[6], e[7]));
        }
    }
    let stride = s.stride as usize;
    let first = args[1] as usize + args[2] as usize; // base vertex + min index
    unsafe {
        crate::d3d::with_buffer(s.vb, |vb| {
            for v in 0..3 {
                let at = s.vb_offset as usize + (first + v) * stride;
                if let Some(bytes) = vb.get(at..at + stride) {
                    log(&format!("  vertex {v}: {:02x?}", bytes));
                }
            }
        })
    };
    for (i, r) in s.vs.iter().enumerate().take(210) {
        log(&format!("  c{i}: {:.4} {:.4} {:.4} {:.4}", r[0], r[1], r[2], r[3]));
    }
    log(&format!("  view {:?}", s.view));
    log(&format!("  proj {:?}", s.proj));
    log(&format!("  vertex shader:\n{}", crate::d3d::disassemble(s.vs_shader)));
    log(&format!("  pixel shader:\n{}", crate::d3d::disassemble(s.ps_shader)));
}

/// This frame's world draws, for the ones that went into the busiest render target
/// (the main view; shadow and reflection passes use other targets).
pub fn take_main_view() -> Vec<Draw> {
    let Ok(mut s) = STATE.lock() else { return Vec::new() };
    let draws = std::mem::take(&mut s.draws);
    let mut counts: Vec<(u32, usize)> = Vec::new();
    for d in &draws {
        match counts.iter_mut().find(|c| c.0 == d.target) {
            Some(c) => c.1 += 1,
            None => counts.push((d.target, 1)),
        }
    }
    let Some(&(main, _)) = counts.iter().max_by_key(|c| c.1) else { return draws };
    draws.into_iter().filter(|d| d.target == main).collect()
}
