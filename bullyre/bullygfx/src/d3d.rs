//! Reading the game's Direct3D 9 resources: its vertex and index buffers are in the
//! MANAGED pool, so a read-only lock gives their system-memory copy; vertex
//! declarations say where positions and normals sit in a vertex.

use std::ffi::c_void;

// IDirect3DVertexBuffer9 / IDirect3DIndexBuffer9 vtable
const LOCK: usize = 11;
const UNLOCK: usize = 12;
const GET_DESC: usize = 13;
// IDirect3DVertexDeclaration9
const GET_DECLARATION: usize = 4;
const D3DLOCK_READONLY: u32 = 0x10;

unsafe fn method(obj: u32, i: usize) -> usize {
    unsafe { *(*(obj as *const *const usize)).add(i) }
}

/// Runs `f` on the whole buffer's bytes while it's locked read-only (vertex or index
/// buffer: same Lock/Unlock/GetDesc slots, and the size is the fifth field of either
/// desc).
pub unsafe fn with_buffer<R>(buf: u32, f: impl FnOnce(&[u8]) -> R) -> Option<R> {
    type GetDesc = unsafe extern "system" fn(usize, *mut u32) -> i32;
    type Lock = unsafe extern "system" fn(usize, u32, u32, *mut *mut c_void, u32) -> i32;
    type Unlock = unsafe extern "system" fn(usize) -> i32;
    let mut desc = [0u32; 6]; // VB: format, type, usage, pool, size, fvf; IB: format, type, usage, pool, size
    let get_desc: GetDesc = unsafe { std::mem::transmute(method(buf, GET_DESC)) };
    if unsafe { get_desc(buf as usize, desc.as_mut_ptr()) } < 0 {
        return None;
    }
    let size = desc[4] as usize;
    let lock: Lock = unsafe { std::mem::transmute(method(buf, LOCK)) };
    let unlock: Unlock = unsafe { std::mem::transmute(method(buf, UNLOCK)) };
    let mut p: *mut c_void = std::ptr::null_mut();
    let hr = unsafe { lock(buf as usize, 0, 0, &mut p, D3DLOCK_READONLY) };
    if hr < 0 || p.is_null() {
        static LOGGED: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);
        if LOGGED.fetch_add(1, std::sync::atomic::Ordering::Relaxed) < 20 {
            crate::control::log(&format!("lock failed {hr:#x}: buffer {buf:#x} desc {desc:x?}"));
        }
        return None;
    }
    let r = f(unsafe { std::slice::from_raw_parts(p as *const u8, size) });
    unsafe { unlock(buf as usize) };
    Some(r)
}

#[derive(Clone, Copy, Default, Debug)]
pub struct Layout {
    pub position: Option<(u16, u8)>, // offset, D3DDECLTYPE
    pub normal: Option<(u16, u8)>,
    pub uv: Option<(u16, u8)>,
    pub weights: Option<(u16, u8)>,
    pub indices: Option<(u16, u8)>,
    pub color: Option<(u16, u8)>,
}

const USAGE_POSITION: u8 = 0;
const USAGE_NORMAL: u8 = 3;
const USAGE_BLENDWEIGHT: u8 = 1;
const USAGE_BLENDINDICES: u8 = 2;
const USAGE_TEXCOORD: u8 = 5;
const USAGE_COLOR: u8 = 10;
const TYPE_D3DCOLOR: u8 = 4;
const TYPE_UBYTE4: u8 = 5;
const TYPE_FLOAT1: u8 = 0;
const TYPE_FLOAT2: u8 = 1;
const TYPE_FLOAT3: u8 = 2;
const TYPE_FLOAT16_2: u8 = 15;
const TYPE_FLOAT4: u8 = 3;

/// Stream 0's position and normal, from the declaration's elements.
pub unsafe fn layout(decl: u32) -> Option<Layout> {
    type GetDeclaration = unsafe extern "system" fn(usize, *mut u8, *mut u32) -> i32;
    let f: GetDeclaration = unsafe { std::mem::transmute(method(decl, GET_DECLARATION)) };
    let mut elements = [0u8; 8 * 65];
    let mut n = 65u32;
    if unsafe { f(decl as usize, elements.as_mut_ptr(), &mut n) } < 0 {
        return None;
    }
    let mut out = Layout::default();
    for e in elements.chunks_exact(8).take(n as usize) {
        let stream = u16::from_le_bytes([e[0], e[1]]);
        if stream == 0xFF {
            break;
        }
        let (offset, ty, usage, index) = (u16::from_le_bytes([e[2], e[3]]), e[4], e[6], e[7]);
        if stream != 0 || index != 0 {
            continue;
        }
        match usage {
            USAGE_POSITION => out.position = Some((offset, ty)),
            USAGE_NORMAL => out.normal = Some((offset, ty)),
            USAGE_TEXCOORD => out.uv = Some((offset, ty)),
            USAGE_BLENDWEIGHT => out.weights = Some((offset, ty)),
            USAGE_BLENDINDICES => out.indices = Some((offset, ty)),
            USAGE_COLOR => out.color = Some((offset, ty)),
            _ => {}
        }
    }
    Some(out)
}

/// Vertex for the new renderer: position, normal, texture coordinates, colour (RGBA
/// bytes).
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Vertex {
    pub pos: [f32; 3],
    pub normal: [f32; 3],
    pub uv: [f32; 2],
    pub color: u32,
}

fn half(h: u16) -> f32 {
    let (s, e, m) = ((h >> 15) as u32, ((h >> 10) & 0x1F) as u32, (h & 0x3FF) as u32);
    let bits = match e {
        0 if m == 0 => s << 31,
        0 => {
            // subnormal
            let mut e = 127 - 14;
            let mut m = m;
            while m & 0x400 == 0 {
                m <<= 1;
                e -= 1;
            }
            (s << 31) | (e << 23) | ((m & 0x3FF) << 13)
        }
        31 => (s << 31) | (0xFF << 23) | (m << 13),
        _ => (s << 31) | ((e + 127 - 15) << 23) | (m << 13),
    };
    f32::from_bits(bits)
}

fn uv_at(v: &[u8], at: usize, ty: u8) -> Option<[f32; 2]> {
    match ty {
        TYPE_FLOAT2 | TYPE_FLOAT3 => {
            let b = v.get(at..at + 8)?;
            Some([0, 4].map(|i| f32::from_le_bytes(b[i..i + 4].try_into().unwrap())))
        }
        TYPE_FLOAT16_2 => {
            let b = v.get(at..at + 4)?;
            Some([0, 2].map(|i| half(u16::from_le_bytes([b[i], b[i + 1]]))))
        }
        _ => None,
    }
}

fn float3(v: &[u8], at: usize) -> Option<[f32; 3]> {
    let b = v.get(at..at + 12)?;
    Some([0, 4, 8].map(|i| f32::from_le_bytes(b[i..i + 4].try_into().unwrap())))
}

/// The buffer's vertices, from `offset` bytes in, `stride` apart.
pub fn vertices(vb: &[u8], offset: usize, stride: usize, l: &Layout) -> Option<Vec<Vertex>> {
    let (p_at, p_ty) = l.position?;
    if !matches!(p_ty, TYPE_FLOAT3 | TYPE_FLOAT4) || stride == 0 {
        return None;
    }
    let n = vb.len().saturating_sub(offset) / stride;
    let normal = l.normal.filter(|n| matches!(n.1, TYPE_FLOAT3 | TYPE_FLOAT4));
    (0..n)
        .map(|i| {
            let v = offset + i * stride;
            let p = float3(vb, v + p_at as usize)?;
            let n = normal.and_then(|(at, _)| float3(vb, v + at as usize)).unwrap_or([0., 0., 1.]);
            let t = l.uv.and_then(|(at, ty)| uv_at(vb, v + at as usize, ty)).unwrap_or([0., 0.]);
            // D3DCOLOR is B, G, R, A in memory
            let color = l
                .color
                .filter(|c| c.1 == TYPE_D3DCOLOR)
                .and_then(|(at, _)| vb.get(v + at as usize..v + at as usize + 4))
                .map_or(u32::MAX, |c| u32::from_le_bytes([c[2], c[1], c[0], c[3]]));
            Some(Vertex { pos: p, normal: n, uv: t, color })
        })
        .collect()
}

/// Skinned vertex: position, normal, texture coordinates, 3 blend weights (the 4th is
/// 1 - their sum), 4 bone indices (bytes packed in a u32).
#[repr(C)]
#[derive(Clone, Copy)]
pub struct SkinVertex {
    pub base: Vertex,
    pub weights: [f32; 3],
    pub indices: u32,
}

/// The buffer's skinned vertices. Indices stored as D3DCOLOR are read back in byte
/// order, as the game's shaders do (D3DCOLORtoUBYTE4).
pub fn skin_vertices(vb: &[u8], offset: usize, stride: usize, l: &Layout) -> Option<Vec<SkinVertex>> {
    let base = vertices(vb, offset, stride, l)?;
    let (w_at, w_ty) = l.weights?;
    let (i_at, i_ty) = l.indices?;
    // FLOAT1..FLOAT4: pieces whose vertices follow 2..4 bones store 1..3 weights (the
    // last is 1 - their sum); four are kept as three, the fourth implied
    let given = match w_ty {
        TYPE_FLOAT1 => 1,
        TYPE_FLOAT2 => 2,
        TYPE_FLOAT3 | TYPE_FLOAT4 => 3,
        _ => return None,
    };
    if !matches!(i_ty, TYPE_D3DCOLOR | TYPE_UBYTE4) {
        return None;
    }
    base.into_iter()
        .enumerate()
        .map(|(i, v)| {
            let at = offset + i * stride;
            let mut w = [0f32; 3];
            for (k, slot) in w.iter_mut().enumerate().take(given) {
                let o = at + w_at as usize + 4 * k;
                *slot = f32::from_le_bytes(vb.get(o..o + 4)?.try_into().unwrap());
            }
            if given < 3 {
                // the implied weight goes in the next slot, so the shader's own
                // "1 - sum" comes out 0
                w[given] = 1.0 - w[..given].iter().sum::<f32>();
            }
            let b = vb.get(at + i_at as usize..at + i_at as usize + 4)?;
            Some(SkinVertex { base: v, weights: w, indices: u32::from_le_bytes([b[0], b[1], b[2], b[3]]) })
        })
        .collect()
}

// ---- textures ------------------------------------------------------------------

// IDirect3DTexture9 vtable
const GET_TYPE: usize = 10;
const GET_LEVEL_COUNT: usize = 13;
const GET_LEVEL_DESC: usize = 17;
const LOCK_RECT: usize = 19;
const UNLOCK_RECT: usize = 20;
const D3DRTYPE_TEXTURE: u32 = 3;
const D3DPOOL_MANAGED: u32 = 1;

pub const FMT_DXT1: u32 = 0x3154_5844;
pub const FMT_DXT3: u32 = 0x3354_5844;
pub const FMT_DXT5: u32 = 0x3554_5844;
const FMT_A8R8G8B8: u32 = 21;
const FMT_X8R8G8B8: u32 = 22;
const FMT_R5G6B5: u32 = 23;
const FMT_A1R5G5B5: u32 = 25;
const FMT_A4R4G4B4: u32 = 26;
const FMT_L8: u32 = 50;
const FMT_A8L8: u32 = 51;

/// How many times a texture couldn't be read (the first few are logged).
static UNREADABLE_SAID: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(0);

/// A texture's pixels: block-compressed (as stored) or 8-bit RGBA.
pub struct Texture {
    pub format: u32, // FMT_DXT* or 0 for RGBA8
    pub width: u32,
    pub height: u32,
    pub levels: Vec<Vec<u8>>, // tightly packed rows (compressed: rows of 4x4 blocks)
}

fn expand(bits: u32, n: u32) -> u8 {
    ((bits * 255 + ((1 << n) - 1) / 2) / ((1 << n) - 1)) as u8
}

/// A MANAGED 2D texture's levels (anything else, e.g. render targets, isn't readable).
pub unsafe fn texture(tex: u32) -> Option<Texture> {
    type GetType = unsafe extern "system" fn(usize) -> u32;
    type GetLevelCount = unsafe extern "system" fn(usize) -> u32;
    type GetLevelDesc = unsafe extern "system" fn(usize, u32, *mut u32) -> i32;
    type LockRect = unsafe extern "system" fn(usize, u32, *mut crate::win::LockedRect, *const c_void, u32) -> i32;
    type UnlockRect = unsafe extern "system" fn(usize, u32) -> i32;
    let t: GetType = unsafe { std::mem::transmute(method(tex, GET_TYPE)) };
    if unsafe { t(tex as usize) } != D3DRTYPE_TEXTURE {
        return None;
    }
    let count: GetLevelCount = unsafe { std::mem::transmute(method(tex, GET_LEVEL_COUNT)) };
    let desc_of: GetLevelDesc = unsafe { std::mem::transmute(method(tex, GET_LEVEL_DESC)) };
    let lock: LockRect = unsafe { std::mem::transmute(method(tex, LOCK_RECT)) };
    let unlock: UnlockRect = unsafe { std::mem::transmute(method(tex, UNLOCK_RECT)) };
    // D3DSURFACE_DESC: format, type, usage, pool, multisample, quality, width, height
    let mut desc = [0u32; 8];
    if unsafe { desc_of(tex as usize, 0, desc.as_mut_ptr()) } < 0 {
        return None;
    }
    if desc[3] != D3DPOOL_MANAGED {
        if desc[2] & D3DUSAGE_RENDERTARGET != 0 {
            return unsafe { render_target_texture(tex, desc[0], desc[6], desc[7]) };
        }
        // (the replay asks again now and then: say so for the first few only)
        if UNREADABLE_SAID.fetch_add(1, std::sync::atomic::Ordering::Relaxed) < 20 {
            crate::control::log(&format!("texture {tex:#x} unreadable: format {:#x} usage {:#x} pool {} {}x{}", desc[0], desc[2], desc[3], desc[6], desc[7]));
        }
        return None;
    }
    let (format, width, height) = (desc[0], desc[6], desc[7]);
    let compressed = matches!(format, FMT_DXT1 | FMT_DXT3 | FMT_DXT5);
    if compressed && (width % 4 != 0 || height % 4 != 0) {
        return None;
    }
    let bpp = match format {
        FMT_A8R8G8B8 | FMT_X8R8G8B8 => 4,
        FMT_R5G6B5 | FMT_A1R5G5B5 | FMT_A4R4G4B4 | FMT_A8L8 => 2,
        FMT_L8 => 1,
        _ if compressed => 0,
        _ => return None,
    };
    let mut levels = Vec::new();
    for level in 0..unsafe { count(tex as usize) } {
        let (w, h) = ((width >> level).max(1), (height >> level).max(1));
        // compressed: rows of 4x4 blocks, 8 bytes (DXT1) or 16 per block
        let (rows, row_bytes) = if compressed {
            (h.div_ceil(4), w.div_ceil(4) * if format == FMT_DXT1 { 8 } else { 16 })
        } else {
            (h, w * bpp)
        };
        let mut locked = crate::win::LockedRect::new(); // pitch, bits
        if unsafe { lock(tex as usize, level, &mut locked, std::ptr::null(), D3DLOCK_READONLY) } < 0 {
            break;
        }
        let (pitch, bits) = (locked.pitch as usize, locked.bits as *const u8);
        let mut out = Vec::with_capacity((rows * row_bytes) as usize * if compressed { 1 } else { 4 / bpp as usize });
        for y in 0..rows as usize {
            let row = unsafe { std::slice::from_raw_parts(bits.add(y * pitch), row_bytes as usize) };
            if compressed {
                out.extend_from_slice(row);
                continue;
            }
            for px in row.chunks_exact(bpp as usize) {
                let v = px.iter().rev().fold(0u32, |a, &b| (a << 8) | b as u32);
                out.extend(match format {
                    FMT_A8R8G8B8 => [px[2], px[1], px[0], px[3]],
                    FMT_X8R8G8B8 => [px[2], px[1], px[0], 255],
                    FMT_R5G6B5 => [expand(v >> 11, 5), expand((v >> 5) & 63, 6), expand(v & 31, 5), 255],
                    FMT_A1R5G5B5 => [expand((v >> 10) & 31, 5), expand((v >> 5) & 31, 5), expand(v & 31, 5), if v >> 15 != 0 { 255 } else { 0 }],
                    FMT_A4R4G4B4 => [expand((v >> 8) & 15, 4), expand((v >> 4) & 15, 4), expand(v & 15, 4), expand(v >> 12, 4)],
                    FMT_A8L8 => [px[0], px[0], px[0], px[1]],
                    _ => [px[0], px[0], px[0], 255], // L8
                });
            }
        }
        unsafe { unlock(tex as usize, level) };
        levels.push(out);
        // wgpu wants every level of a block-compressed texture to be whole blocks
        if compressed && ((w >> 1) % 4 != 0 || (h >> 1) % 4 != 0) {
            break;
        }
    }
    (!levels.is_empty()).then_some(Texture { format: if compressed { format } else { 0 }, width, height, levels })
}

/// Whether the resource is a cube texture (D3DRTYPE_CUBETEXTURE).
pub unsafe fn is_cube(tex: u32) -> bool {
    type GetType = unsafe extern "system" fn(usize) -> u32;
    let t: GetType = unsafe { std::mem::transmute(method(tex, GET_TYPE)) };
    unsafe { t(tex as usize) == 5 }
}

/// A cheap fingerprint of a managed texture's top level, straight from the locked
/// memory (nothing copied): (width, height, format, sampled bytes). None for what
/// `texture` reads another way (render targets) or can't read.
pub unsafe fn texture_signature(tex: u32, sig: impl Fn(&[u8]) -> u64) -> Option<u64> {
    type GetType = unsafe extern "system" fn(usize) -> u32;
    type GetLevelDesc = unsafe extern "system" fn(usize, u32, *mut u32) -> i32;
    type LockRect = unsafe extern "system" fn(usize, u32, *mut crate::win::LockedRect, *const c_void, u32) -> i32;
    type UnlockRect = unsafe extern "system" fn(usize, u32) -> i32;
    let t: GetType = unsafe { std::mem::transmute(method(tex, GET_TYPE)) };
    if unsafe { t(tex as usize) } != D3DRTYPE_TEXTURE {
        return None;
    }
    let desc_of: GetLevelDesc = unsafe { std::mem::transmute(method(tex, GET_LEVEL_DESC)) };
    let mut desc = [0u32; 8];
    if unsafe { desc_of(tex as usize, 0, desc.as_mut_ptr()) } < 0 || desc[3] != D3DPOOL_MANAGED {
        return None;
    }
    let (format, width, height) = (desc[0], desc[6], desc[7]);
    let rows = if matches!(format, FMT_DXT1 | FMT_DXT3 | FMT_DXT5) { height.div_ceil(4) } else { height };
    let lock: LockRect = unsafe { std::mem::transmute(method(tex, LOCK_RECT)) };
    let unlock: UnlockRect = unsafe { std::mem::transmute(method(tex, UNLOCK_RECT)) };
    let mut locked = crate::win::LockedRect::new();
    if unsafe { lock(tex as usize, 0, &mut locked, std::ptr::null(), D3DLOCK_READONLY) } < 0 {
        return None;
    }
    let bytes = unsafe { std::slice::from_raw_parts(locked.bits as *const u8, locked.pitch as usize * rows as usize) };
    let s = sig(bytes) ^ ((width as u64) << 32 | height as u64) ^ format as u64;
    unsafe { unlock(tex as usize, 0) };
    Some(s)
}

const D3DUSAGE_RENDERTARGET: u32 = 1;
const D3DPOOL_SYSTEMMEM: u32 = 2;
// IDirect3DResource9 / IDirect3DTexture9 / IDirect3DSurface9 / IDirect3DDevice9 slots
const RES_GET_DEVICE: usize = 3;
const TEX_GET_SURFACE_LEVEL: usize = 18;
const SURF_LOCK_RECT: usize = 13;
const SURF_UNLOCK_RECT: usize = 14;
const DEV_GET_RENDER_TARGET_DATA: usize = 32;
const DEV_CREATE_OFFSCREEN: usize = 36;

/// A texture the game draws into (e.g. Jimmy's outfit, put together at run time):
/// copied to system memory with GetRenderTargetData, top level only.
unsafe fn render_target_texture(tex: u32, format: u32, width: u32, height: u32) -> Option<Texture> {
    type GetDevice = unsafe extern "system" fn(usize, *mut usize) -> i32;
    type GetSurfaceLevel = unsafe extern "system" fn(usize, u32, *mut usize) -> i32;
    type CreateOffscreen = unsafe extern "system" fn(usize, u32, u32, u32, u32, *mut usize, *mut c_void) -> i32;
    type GetRtData = unsafe extern "system" fn(usize, usize, usize) -> i32;
    type LockRect = unsafe extern "system" fn(usize, *mut crate::win::LockedRect, *const c_void, u32) -> i32;
    type UnlockRect = unsafe extern "system" fn(usize) -> i32;
    type Release = unsafe extern "system" fn(usize) -> u32;
    if !matches!(format, FMT_A8R8G8B8 | FMT_X8R8G8B8) {
        if UNREADABLE_SAID.fetch_add(1, std::sync::atomic::Ordering::Relaxed) < 20 {
            crate::control::log(&format!("render target texture {tex:#x}: format {format:#x} not handled"));
        }
        return None;
    }
    let release = |o: u32| unsafe { std::mem::transmute::<usize, Release>(method(o, 2))(o as usize) };
    // the real device (resources know it; the game only sees our wrapper)
    let mut device = 0usize;
    if unsafe { std::mem::transmute::<usize, GetDevice>(method(tex, RES_GET_DEVICE))(tex as usize, &mut device) } < 0 {
        return None;
    }
    let device = crate::win::obj32(device);
    let mut surface = 0usize;
    let mut sys = 0usize;
    let mut out = None;
    if unsafe { std::mem::transmute::<usize, GetSurfaceLevel>(method(tex, TEX_GET_SURFACE_LEVEL))(tex as usize, 0, &mut surface) } >= 0 {
        let surface = crate::win::obj32(surface);
        let create: CreateOffscreen = unsafe { std::mem::transmute(method(device, DEV_CREATE_OFFSCREEN)) };
        if unsafe { create(device as usize, width, height, format, D3DPOOL_SYSTEMMEM, &mut sys, std::ptr::null_mut()) } >= 0 {
            let sys = crate::win::obj32(sys);
            let get: GetRtData = unsafe { std::mem::transmute(method(device, DEV_GET_RENDER_TARGET_DATA)) };
            let mut locked = crate::win::LockedRect::new();
            if unsafe { get(device as usize, surface as usize, sys as usize) } >= 0
                && unsafe { std::mem::transmute::<usize, LockRect>(method(sys, SURF_LOCK_RECT))(sys as usize, &mut locked, std::ptr::null(), D3DLOCK_READONLY) } >= 0
            {
                let (pitch, bits) = (locked.pitch as usize, locked.bits as *const u8);
                let mut rgba = Vec::with_capacity((width * height * 4) as usize);
                for y in 0..height as usize {
                    let row = unsafe { std::slice::from_raw_parts(bits.add(y * pitch), width as usize * 4) };
                    for px in row.chunks_exact(4) {
                        rgba.extend([px[2], px[1], px[0], if format == FMT_X8R8G8B8 { 255 } else { px[3] }]);
                    }
                }
                unsafe { std::mem::transmute::<usize, UnlockRect>(method(sys, SURF_UNLOCK_RECT))(sys as usize) };
                out = Some(Texture { format: 0, width, height, levels: vec![rgba] });
            }
            release(sys);
        }
        release(surface);
    }
    release(device);
    out
}

/// A shader's bytecode (IDirect3DVertexShader9 / IDirect3DPixelShader9 GetFunction)
/// as text, through the game's own d3dx9_38 (D3DXDisassembleShader).
pub fn disassemble(shader: u32) -> String {
    type GetFunction = unsafe extern "system" fn(usize, *mut u8, *mut u32) -> i32;
    type Disassemble = unsafe extern "system" fn(*const u32, i32, *const u8, *mut usize) -> i32;
    type BufPtr = unsafe extern "system" fn(usize) -> *const u8;
    type BufSize = unsafe extern "system" fn(usize) -> u32;
    type Release = unsafe extern "system" fn(usize) -> u32;
    if shader == 0 {
        return "(none)".into();
    }
    let get: GetFunction = unsafe { std::mem::transmute(method(shader, 4)) };
    let mut size = 0u32;
    if unsafe { get(shader as usize, std::ptr::null_mut(), &mut size) } < 0 || size == 0 {
        return "(no bytecode)".into();
    }
    let mut code = vec![0u8; size as usize];
    if unsafe { get(shader as usize, code.as_mut_ptr(), &mut size) } < 0 {
        return "(no bytecode)".into();
    }
    let module = unsafe { crate::win::LoadLibraryW(crate::win::wide("d3dx9_38.dll").as_ptr()) };
    let f = unsafe { crate::win::GetProcAddress(module, c"D3DXDisassembleShader".as_ptr() as *const u8) };
    if f.is_null() {
        return format!("({} bytes, no D3DXDisassembleShader)", code.len());
    }
    let f: Disassemble = unsafe { std::mem::transmute(f) };
    let mut buf = 0usize;
    if unsafe { f(code.as_ptr() as *const u32, 0, std::ptr::null(), &mut buf) } < 0 || buf == 0 {
        return "(disassembly failed)".into();
    }
    let buf = crate::win::obj32(buf);
    let text = unsafe {
        let p: BufPtr = std::mem::transmute(method(buf, 3));
        let n: BufSize = std::mem::transmute(method(buf, 4));
        let bytes = std::slice::from_raw_parts(p(buf as usize), n(buf as usize) as usize);
        String::from_utf8_lossy(bytes).trim_end_matches('\0').to_string()
    };
    let release: Release = unsafe { std::mem::transmute(method(buf, 2)) };
    unsafe { release(buf as usize) };
    text
}

// IDirect3DCubeTexture9: same slots as a 2D texture, with a face argument
const CUBE_GET_LEVEL_DESC: usize = 17;
const CUBE_LOCK_RECT: usize = 19;
const CUBE_UNLOCK_RECT: usize = 20;
const D3DRTYPE_CUBETEXTURE: u32 = 5;

/// A MANAGED cube texture: all six faces, each with its levels, face by face.
pub unsafe fn cube_texture(tex: u32) -> Option<Texture> {
    type GetType = unsafe extern "system" fn(usize) -> u32;
    type GetLevelCount = unsafe extern "system" fn(usize) -> u32;
    type GetLevelDesc = unsafe extern "system" fn(usize, u32, *mut u32) -> i32;
    type LockRect = unsafe extern "system" fn(usize, u32, u32, *mut crate::win::LockedRect, *const c_void, u32) -> i32;
    type UnlockRect = unsafe extern "system" fn(usize, u32, u32) -> i32;
    if unsafe { std::mem::transmute::<usize, GetType>(method(tex, GET_TYPE))(tex as usize) } != D3DRTYPE_CUBETEXTURE {
        return None;
    }
    let mut desc = [0u32; 8];
    if unsafe { std::mem::transmute::<usize, GetLevelDesc>(method(tex, CUBE_GET_LEVEL_DESC))(tex as usize, 0, desc.as_mut_ptr()) } < 0 || desc[3] != D3DPOOL_MANAGED {
        return None;
    }
    let (format, size) = (desc[0], desc[6]);
    let compressed = matches!(format, FMT_DXT1 | FMT_DXT3 | FMT_DXT5);
    if !compressed && !matches!(format, FMT_A8R8G8B8 | FMT_X8R8G8B8) {
        return None;
    }
    let count = unsafe { std::mem::transmute::<usize, GetLevelCount>(method(tex, GET_LEVEL_COUNT))(tex as usize) };
    // levels a block-compressed texture can have (whole blocks)
    let mut usable = 0;
    while usable < count && (!compressed || (size >> usable) >= 4) {
        usable += 1;
    }
    let lock: LockRect = unsafe { std::mem::transmute(method(tex, CUBE_LOCK_RECT)) };
    let unlock: UnlockRect = unsafe { std::mem::transmute(method(tex, CUBE_UNLOCK_RECT)) };
    let mut levels = Vec::new();
    for face in 0..6 {
        for level in 0..usable {
            let s = (size >> level).max(1);
            let (rows, row_bytes) = if compressed { (s.div_ceil(4), s.div_ceil(4) * if format == FMT_DXT1 { 8 } else { 16 }) } else { (s, s * 4) };
            let mut locked = crate::win::LockedRect::new();
            if unsafe { lock(tex as usize, face, level, &mut locked, std::ptr::null(), D3DLOCK_READONLY) } < 0 {
                return None;
            }
            let (pitch, bits) = (locked.pitch as usize, locked.bits as *const u8);
            let mut out = Vec::with_capacity((rows * row_bytes) as usize);
            for y in 0..rows as usize {
                let row = unsafe { std::slice::from_raw_parts(bits.add(y * pitch), row_bytes as usize) };
                if compressed {
                    out.extend_from_slice(row);
                } else {
                    for px in row.chunks_exact(4) {
                        out.extend([px[2], px[1], px[0], if format == FMT_X8R8G8B8 { 255 } else { px[3] }]);
                    }
                }
            }
            unsafe { unlock(tex as usize, face, level) };
            levels.push(out);
        }
    }
    Some(Texture { format: if compressed { format } else { 0 }, width: size, height: size, levels })
}
