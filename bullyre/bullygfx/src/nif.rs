//! Gamebryo 2.3 files, version 20.3.0.9: models (`.nif`) and texture dictionaries (`.nft`,
//! the same container holding only texture blocks). Our own reader, written from the layout
//! as measured on the game's data. It hands values over as stored and interprets none of
//! them: no transform or skinning convention, no meaning given to property flags.
//!
//! The container:
//! - a text line ending in '\n';
//! - version u32, byte order u8 (1 = little-endian), user version u32 and block count u32,
//!   always little-endian;
//! - then, in the file's byte order: type count u16, the type names (u32 length + text),
//!   each block's type u16, each block's size u32, string count u32, longest string u32,
//!   the strings (u32 length + text), group count u32 + u32 per group;
//! - the blocks back to back;
//! - the roots: count u32 + block references.
//!
//! The header's block sizes make the reader check itself. A block of a decoded type has to
//! use exactly its size, or it is kept raw and listed in `Nif::mismatched` rather than
//! trusted. `tests::every_file_in_world_img` runs it on every model and texture in the game.
#![allow(dead_code)]

/// A block index, -1 for none.
pub type Ref = i32;
/// An index into `Nif::strings`, -1 for none.
pub type StringIndex = i32;

pub const VERSION: u32 = 0x1403_0009;

/// `PixelData::pixel_format` values present in the game's textures (Gamebryo's numbering;
/// each one's bytes per pixel is checked by the test).
pub const FORMAT_RGB: u32 = 0;
pub const FORMAT_RGBA: u32 = 1;
pub const FORMAT_PALETTE: u32 = 2;
pub const FORMAT_DXT1: u32 = 4;
pub const FORMAT_DXT5: u32 = 6;

/// `TexturingProperty::slots` indices, in the file's order.
pub const SLOT_BASE: usize = 0;
pub const SLOT_DARK: usize = 1;
pub const SLOT_DETAIL: usize = 2;
pub const SLOT_GLOSS: usize = 3;
pub const SLOT_GLOW: usize = 4;
pub const SLOT_BUMP: usize = 5;
pub const SLOT_NORMAL: usize = 6;
pub const SLOT_PARALLAX: usize = 7;
pub const SLOT_DECAL: usize = 8;

#[derive(Debug, Clone, Default)]
pub struct ObjectNet {
    pub name: StringIndex,
    pub extra_data: Vec<Ref>,
    pub controller: Ref,
}

#[derive(Debug, Clone, Default)]
pub struct AvObject {
    pub net: ObjectNet,
    pub flags: u16,
    pub translation: [f32; 3],
    /// the nine floats in file order
    pub rotation: [f32; 9],
    pub scale: f32,
    pub properties: Vec<Ref>,
    pub collision: Ref,
}

#[derive(Debug, Clone, Default)]
pub struct Node {
    pub av: AvObject,
    pub children: Vec<Ref>,
    pub effects: Vec<Ref>,
}

/// NiTriShape and NiTriStrips.
#[derive(Debug, Clone, Default)]
pub struct Geometry {
    pub av: AvObject,
    pub data: Ref,
    pub skin: Ref,
    pub material_names: Vec<StringIndex>,
    pub material_extra: Vec<i32>,
    pub active_material: i32,
    pub material_dirty: u8,
}

/// What NiTriShapeData and NiTriStripsData share. Absent arrays are empty.
#[derive(Debug, Clone, Default)]
pub struct GeometryData {
    pub group_id: i32,
    pub num_vertices: u16,
    pub keep_flags: u8,
    pub compress_flags: u8,
    pub vertices: Vec<[f32; 3]>,
    /// bits 0-5: UV set count; bit 12: tangents and bitangents follow the normals
    pub data_flags: u16,
    pub normals: Vec<[f32; 3]>,
    pub tangents: Vec<[f32; 3]>,
    pub bitangents: Vec<[f32; 3]>,
    pub center: [f32; 3],
    pub radius: f32,
    pub colors: Vec<[f32; 4]>,
    pub uv_sets: Vec<Vec<[f32; 2]>>,
    pub consistency: u16,
    pub additional_data: Ref,
}

#[derive(Debug, Clone, Default)]
pub struct TriShapeData {
    pub geometry: GeometryData,
    pub num_triangles: u16,
    pub num_triangle_points: u32,
    pub triangles: Vec<[u16; 3]>,
    pub match_groups: Vec<Vec<u16>>,
}

#[derive(Debug, Clone, Default)]
pub struct TriStripsData {
    pub geometry: GeometryData,
    pub num_triangles: u16,
    /// each strip's indices, as stored
    pub strips: Vec<Vec<u16>>,
}

/// NiSourceTexture and NiSourceCubeMap.
#[derive(Debug, Clone, Default)]
pub struct SourceTexture {
    pub net: ObjectNet,
    pub use_external: u8,
    pub file_name: StringIndex,
    pub pixel_data: Ref,
    pub pixel_layout: u32,
    pub use_mipmaps: u32,
    pub alpha_format: u32,
    pub is_static: u8,
    pub direct_render: u8,
    pub persist_render_data: u8,
}

#[derive(Debug, Clone, Default)]
pub struct MaterialProperty {
    pub net: ObjectNet,
    pub ambient: [f32; 3],
    pub diffuse: [f32; 3],
    pub specular: [f32; 3],
    pub emissive: [f32; 3],
    pub glossiness: f32,
    pub alpha: f32,
}

#[derive(Debug, Clone, Default)]
pub struct TexTransform {
    pub translation: [f32; 2],
    pub scale: [f32; 2],
    pub rotation: f32,
    pub method: u32,
    pub center: [f32; 2],
}

#[derive(Debug, Clone, Default)]
pub struct TexDesc {
    pub source: Ref,
    pub flags: u16,
    pub transform: Option<TexTransform>,
}

#[derive(Debug, Clone, Default)]
pub struct TexturingProperty {
    pub net: ObjectNet,
    pub flags: u16,
    /// one per slot (`SLOT_*`), None where the slot is empty
    pub slots: Vec<Option<TexDesc>>,
    /// the bump slot's six extra floats, when it is filled
    pub bump: Option<[f32; 6]>,
    /// the parallax slot's extra float, when it is filled
    pub parallax_offset: Option<f32>,
    /// shader textures: (texture, map number), None where the entry is empty
    pub shader_textures: Vec<Option<(TexDesc, u32)>>,
}

/// Flags as stored; their meaning is left to the user.
#[derive(Debug, Clone, Default)]
pub struct AlphaProperty {
    pub net: ObjectNet,
    pub flags: u16,
    pub threshold: u8,
}

/// Flags as stored; their meaning is left to the user.
#[derive(Debug, Clone, Default)]
pub struct StencilProperty {
    pub net: ObjectNet,
    pub flags: u16,
    pub reference: u32,
    pub mask: u32,
}

#[derive(Debug, Clone, Copy, Default)]
pub struct Channel {
    pub kind: u32,
    pub convention: u32,
    pub bits: u8,
    pub signed: u8,
}

#[derive(Debug, Clone, Copy, Default)]
pub struct Mip {
    pub width: u32,
    pub height: u32,
    /// from the start of a face
    pub offset: u32,
}

#[derive(Debug, Clone, Default)]
pub struct PixelData {
    pub pixel_format: u32,
    pub bits_per_pixel: u8,
    pub renderer_hint: i32,
    pub extra_data: u32,
    pub flags: u8,
    pub tiling: u32,
    pub srgb: u8,
    pub channels: [Channel; 4],
    pub palette: Ref,
    pub bytes_per_pixel: u32,
    pub mipmaps: Vec<Mip>,
    /// one face's bytes, every mip level included
    pub bytes_per_face: u32,
    pub num_faces: u32,
    /// the faces back to back, as stored
    pub pixels: Vec<u8>,
}

impl PixelData {
    /// One mip level of one face, as stored.
    pub fn level(&self, face: usize, level: usize) -> Option<&[u8]> {
        let start = self.mipmaps.get(level)?.offset as usize;
        let end = self.mipmaps.get(level + 1).map_or(self.bytes_per_face as usize, |m| m.offset as usize);
        let base = face * self.bytes_per_face as usize;
        self.pixels.get(base + start..base + end)
    }
}

#[derive(Debug, Clone, Default)]
pub struct Palette {
    pub has_alpha: u8,
    pub entries: Vec<[u8; 4]>,
}

#[derive(Debug, Clone, Default)]
pub struct SkinInstance {
    pub data: Ref,
    pub partition: Ref,
    pub skeleton_root: Ref,
    pub bones: Vec<Ref>,
}

/// Rotation (nine floats in file order), translation and scale, as stored.
#[derive(Debug, Clone, Default)]
pub struct SkinTransform {
    pub rotation: [f32; 9],
    pub translation: [f32; 3],
    pub scale: f32,
}

#[derive(Debug, Clone, Default)]
pub struct SkinBone {
    pub transform: SkinTransform,
    pub sphere_center: [f32; 3],
    pub sphere_radius: f32,
    pub num_vertices: u16,
    /// (vertex index, weight), when the skin has weights
    pub weights: Vec<(u16, f32)>,
}

#[derive(Debug, Clone, Default)]
pub struct SkinData {
    pub transform: SkinTransform,
    pub has_weights: u8,
    pub bones: Vec<SkinBone>,
}

#[derive(Debug, Clone, Default)]
pub struct Partition {
    pub num_vertices: u16,
    pub num_triangles: u16,
    pub weights_per_vertex: u16,
    pub bones: Vec<u16>,
    pub vertex_map: Vec<u16>,
    /// num_vertices × weights_per_vertex
    pub weights: Vec<f32>,
    pub strips: Vec<Vec<u16>>,
    pub triangles: Vec<[u16; 3]>,
    /// num_vertices × weights_per_vertex
    pub bone_indices: Vec<u8>,
}

#[derive(Debug, Clone, Default)]
pub struct SkinPartition {
    pub partitions: Vec<Partition>,
}

#[derive(Debug, Clone)]
pub enum Block {
    Node(Node),
    TriShape(Geometry),
    TriStrips(Geometry),
    TriShapeData(TriShapeData),
    TriStripsData(TriStripsData),
    SourceTexture(SourceTexture),
    SourceCubeMap(SourceTexture),
    Material(MaterialProperty),
    Texturing(TexturingProperty),
    Alpha(AlphaProperty),
    Stencil(StencilProperty),
    PixelData(PixelData),
    Palette(Palette),
    SkinInstance(SkinInstance),
    SkinData(SkinData),
    SkinPartition(SkinPartition),
    /// A type this reader doesn't decode, or a decoded type whose layout did not use
    /// exactly the block's size (then also listed in `Nif::mismatched`).
    Raw(Vec<u8>),
}

#[derive(Debug, Clone, Default)]
pub struct Nif {
    pub big_endian: bool,
    pub user_version: u32,
    pub types: Vec<String>,
    /// each block's index into `types`
    pub block_types: Vec<u16>,
    pub strings: Vec<String>,
    pub groups: Vec<u32>,
    pub blocks: Vec<Block>,
    pub roots: Vec<Ref>,
    /// blocks of a decoded type that did not use exactly their size, kept raw
    pub mismatched: Vec<usize>,
    /// where the roots end: what follows in an archive entry is the sector padding
    pub end: usize,
}

impl Nif {
    pub fn type_name(&self, block: usize) -> &str {
        self.block_types.get(block).and_then(|&t| self.types.get(t as usize)).map_or("", |s| s.as_str())
    }

    pub fn string(&self, i: StringIndex) -> Option<&str> {
        usize::try_from(i).ok().and_then(|i| self.strings.get(i)).map(|s| s.as_str())
    }

    pub fn block(&self, r: Ref) -> Option<&Block> {
        usize::try_from(r).ok().and_then(|i| self.blocks.get(i))
    }

    pub fn parse(data: &[u8]) -> Result<Nif, String> {
        let line = data.iter().position(|&c| c == b'\n').ok_or("no header line")?;
        if !data.starts_with(b"Gamebryo File Format, Version ") {
            return Err("not a Gamebryo file".into());
        }
        let mut r = Reader { d: data, p: line + 1, be: false };
        let version = r.u32()?;
        if version != VERSION {
            return Err(format!("version {version:#x}, not 20.3.0.9"));
        }
        let order = r.u8()?;
        let user_version = r.u32()?;
        let num_blocks = r.u32()? as usize;
        r.be = order != 1;
        let num_types = r.u16()? as usize;
        let types = r.list(num_types, |r| r.text())?;
        let block_types = r.list(num_blocks, |r| r.u16())?;
        let sizes = r.list(num_blocks, |r| r.u32())?;
        let num_strings = r.u32()? as usize;
        let _longest = r.u32()?;
        let strings = r.list(num_strings, |r| r.text())?;
        let num_groups = r.u32()? as usize;
        let groups = r.list(num_groups, |r| r.u32())?;

        let mut blocks = Vec::with_capacity(num_blocks);
        let mut mismatched = Vec::new();
        for (i, (&t, &size)) in block_types.iter().zip(&sizes).enumerate() {
            let bytes = r.take(size as usize)?;
            let ty = types.get(t as usize).ok_or_else(|| format!("block {i}: type {t} of {}", types.len()))?;
            let mut b = Reader { d: bytes, p: 0, be: r.be };
            blocks.push(match decode(ty, &mut b) {
                Ok(Some(block)) if b.p == bytes.len() => block,
                Ok(None) => Block::Raw(bytes.to_vec()),
                _ => {
                    mismatched.push(i);
                    Block::Raw(bytes.to_vec())
                }
            });
        }
        let num_roots = r.u32()? as usize;
        let roots = r.list(num_roots, |r| r.i32())?;
        Ok(Nif { big_endian: r.be, user_version, types, block_types, strings, groups, blocks, roots, mismatched, end: r.p })
    }
}

struct Reader<'a> {
    d: &'a [u8],
    p: usize,
    be: bool,
}

type Res<T> = Result<T, String>;

impl<'a> Reader<'a> {
    fn take(&mut self, n: usize) -> Res<&'a [u8]> {
        let end = self.p.checked_add(n).filter(|&e| e <= self.d.len()).ok_or_else(|| format!("{n} bytes at {:#x} run past the end", self.p))?;
        let s = &self.d[self.p..end];
        self.p = end;
        Ok(s)
    }
    fn array<const N: usize>(&mut self) -> Res<[u8; N]> {
        Ok(self.take(N)?.try_into().unwrap())
    }
    fn u8(&mut self) -> Res<u8> {
        Ok(self.take(1)?[0])
    }
    fn u16(&mut self) -> Res<u16> {
        let b = self.array()?;
        Ok(if self.be { u16::from_be_bytes(b) } else { u16::from_le_bytes(b) })
    }
    fn u32(&mut self) -> Res<u32> {
        let b = self.array()?;
        Ok(if self.be { u32::from_be_bytes(b) } else { u32::from_le_bytes(b) })
    }
    fn i32(&mut self) -> Res<i32> {
        Ok(self.u32()? as i32)
    }
    fn f32(&mut self) -> Res<f32> {
        Ok(f32::from_bits(self.u32()?))
    }
    fn floats<const N: usize>(&mut self) -> Res<[f32; N]> {
        let mut out = [0.0; N];
        for v in &mut out {
            *v = self.f32()?;
        }
        Ok(out)
    }
    /// u32 length + bytes
    fn text(&mut self) -> Res<String> {
        let n = self.u32()? as usize;
        Ok(String::from_utf8_lossy(self.take(n)?).into_owned())
    }
    fn list<T>(&mut self, n: usize, mut f: impl FnMut(&mut Self) -> Res<T>) -> Res<Vec<T>> {
        // (the capacity is bounded by what is left: a bad count fails on reading, not allocating)
        let mut out = Vec::with_capacity(n.min(self.d.len() - self.p));
        for _ in 0..n {
            out.push(f(self)?);
        }
        Ok(out)
    }
    /// u32 count + references
    fn refs(&mut self) -> Res<Vec<Ref>> {
        let n = self.u32()? as usize;
        self.list(n, |r| r.i32())
    }
    /// a u8 "present" flag, then n items when it is set
    fn optional<T>(&mut self, n: usize, f: impl FnMut(&mut Self) -> Res<T>) -> Res<Vec<T>> {
        if self.u8()? != 0 { self.list(n, f) } else { Ok(Vec::new()) }
    }
}

fn decode(ty: &str, r: &mut Reader) -> Res<Option<Block>> {
    Ok(Some(match ty {
        "NiNode" => Block::Node(Node { av: av_object(r)?, children: r.refs()?, effects: r.refs()? }),
        "NiTriShape" => Block::TriShape(geometry(r)?),
        "NiTriStrips" => Block::TriStrips(geometry(r)?),
        "NiTriShapeData" => {
            let geometry = geometry_data(r)?;
            let num_triangles = r.u16()?;
            let num_triangle_points = r.u32()?;
            let triangles = r.optional(num_triangles as usize, |r| Ok([r.u16()?, r.u16()?, r.u16()?]))?;
            let groups = r.u16()? as usize;
            let match_groups = r.list(groups, |r| {
                let n = r.u16()? as usize;
                r.list(n, |r| r.u16())
            })?;
            Block::TriShapeData(TriShapeData { geometry, num_triangles, num_triangle_points, triangles, match_groups })
        }
        "NiTriStripsData" => {
            let geometry = geometry_data(r)?;
            let num_triangles = r.u16()?;
            let num_strips = r.u16()? as usize;
            let lengths = r.list(num_strips, |r| r.u16())?;
            let strips = if r.u8()? != 0 { lengths.iter().map(|&n| r.list(n as usize, |r| r.u16())).collect::<Res<_>>()? } else { Vec::new() };
            Block::TriStripsData(TriStripsData { geometry, num_triangles, strips })
        }
        "NiSourceTexture" => Block::SourceTexture(source_texture(r)?),
        "NiSourceCubeMap" => Block::SourceCubeMap(source_texture(r)?),
        "NiMaterialProperty" => Block::Material(MaterialProperty {
            net: object_net(r)?,
            ambient: r.floats()?,
            diffuse: r.floats()?,
            specular: r.floats()?,
            emissive: r.floats()?,
            glossiness: r.f32()?,
            alpha: r.f32()?,
        }),
        "NiTexturingProperty" => {
            let net = object_net(r)?;
            let flags = r.u16()?;
            let count = r.u32()? as usize;
            let (mut bump, mut parallax_offset) = (None, None);
            let mut slots = Vec::with_capacity(count.min(16));
            for slot in 0..count {
                if r.u8()? == 0 {
                    slots.push(None);
                    continue;
                }
                slots.push(Some(tex_desc(r)?));
                match slot {
                    SLOT_BUMP => bump = Some(r.floats()?),
                    SLOT_PARALLAX => parallax_offset = Some(r.f32()?),
                    _ => {}
                }
            }
            // (each: a "present" byte, then the texture and its map number; the water decals use two)
            let num_shader = r.u32()? as usize;
            let shader_textures = r.list(num_shader, |r| if r.u8()? != 0 { Ok(Some((tex_desc(r)?, r.u32()?))) } else { Ok(None) })?;
            Block::Texturing(TexturingProperty { net, flags, slots, bump, parallax_offset, shader_textures })
        }
        "NiAlphaProperty" => Block::Alpha(AlphaProperty { net: object_net(r)?, flags: r.u16()?, threshold: r.u8()? }),
        "NiStencilProperty" => Block::Stencil(StencilProperty { net: object_net(r)?, flags: r.u16()?, reference: r.u32()?, mask: r.u32()? }),
        "NiPixelData" => {
            let pixel_format = r.u32()?;
            let bits_per_pixel = r.u8()?;
            let renderer_hint = r.i32()?;
            let extra_data = r.u32()?;
            let flags = r.u8()?;
            let tiling = r.u32()?;
            let srgb = r.u8()?;
            let mut channels = [Channel::default(); 4];
            for c in &mut channels {
                *c = Channel { kind: r.u32()?, convention: r.u32()?, bits: r.u8()?, signed: r.u8()? };
            }
            let palette = r.i32()?;
            let num_mipmaps = r.u32()? as usize;
            let bytes_per_pixel = r.u32()?;
            let mipmaps = r.list(num_mipmaps, |r| Ok(Mip { width: r.u32()?, height: r.u32()?, offset: r.u32()? }))?;
            let bytes_per_face = r.u32()?;
            let num_faces = r.u32()?;
            let total = (bytes_per_face as usize).checked_mul(num_faces as usize).ok_or("pixel size overflows")?;
            let pixels = r.take(total)?.to_vec();
            Block::PixelData(PixelData {
                pixel_format,
                bits_per_pixel,
                renderer_hint,
                extra_data,
                flags,
                tiling,
                srgb,
                channels,
                palette,
                bytes_per_pixel,
                mipmaps,
                bytes_per_face,
                num_faces,
                pixels,
            })
        }
        "NiPalette" => {
            let has_alpha = r.u8()?;
            let n = r.u32()? as usize;
            Block::Palette(Palette { has_alpha, entries: r.list(n, |r| r.array())? })
        }
        "NiSkinInstance" => Block::SkinInstance(SkinInstance { data: r.i32()?, partition: r.i32()?, skeleton_root: r.i32()?, bones: r.refs()? }),
        "NiSkinData" => {
            let transform = skin_transform(r)?;
            let num_bones = r.u32()? as usize;
            let has_weights = r.u8()?;
            let bones = r.list(num_bones, |r| {
                let transform = skin_transform(r)?;
                let sphere_center = r.floats()?;
                let sphere_radius = r.f32()?;
                let num_vertices = r.u16()?;
                let weights = if has_weights != 0 { r.list(num_vertices as usize, |r| Ok((r.u16()?, r.f32()?)))? } else { Vec::new() };
                Ok(SkinBone { transform, sphere_center, sphere_radius, num_vertices, weights })
            })?;
            Block::SkinData(SkinData { transform, has_weights, bones })
        }
        "NiSkinPartition" => {
            let n = r.u32()? as usize;
            let partitions = r.list(n, |r| {
                let num_vertices = r.u16()?;
                let num_triangles = r.u16()?;
                let num_bones = r.u16()? as usize;
                let num_strips = r.u16()? as usize;
                let weights_per_vertex = r.u16()?;
                let per_vertex = num_vertices as usize * weights_per_vertex as usize;
                let bones = r.list(num_bones, |r| r.u16())?;
                let vertex_map = r.optional(num_vertices as usize, |r| r.u16())?;
                let weights = r.optional(per_vertex, |r| r.f32())?;
                let lengths = r.list(num_strips, |r| r.u16())?;
                let (mut strips, mut triangles) = (Vec::new(), Vec::new());
                if r.u8()? != 0 {
                    if num_strips != 0 {
                        strips = lengths.iter().map(|&len| r.list(len as usize, |r| r.u16())).collect::<Res<_>>()?;
                    } else {
                        triangles = r.list(num_triangles as usize, |r| Ok([r.u16()?, r.u16()?, r.u16()?]))?;
                    }
                }
                let bone_indices = r.optional(per_vertex, |r| r.u8())?;
                Ok(Partition { num_vertices, num_triangles, weights_per_vertex, bones, vertex_map, weights, strips, triangles, bone_indices })
            })?;
            Block::SkinPartition(SkinPartition { partitions })
        }
        _ => return Ok(None),
    }))
}

fn object_net(r: &mut Reader) -> Res<ObjectNet> {
    Ok(ObjectNet { name: r.i32()?, extra_data: r.refs()?, controller: r.i32()? })
}

fn av_object(r: &mut Reader) -> Res<AvObject> {
    Ok(AvObject {
        net: object_net(r)?,
        flags: r.u16()?,
        translation: r.floats()?,
        rotation: r.floats()?,
        scale: r.f32()?,
        properties: r.refs()?,
        collision: r.i32()?,
    })
}

fn geometry(r: &mut Reader) -> Res<Geometry> {
    let av = av_object(r)?;
    let data = r.i32()?;
    let skin = r.i32()?;
    let n = r.u32()? as usize;
    let material_names = r.list(n, |r| r.i32())?;
    let material_extra = r.list(n, |r| r.i32())?;
    Ok(Geometry { av, data, skin, material_names, material_extra, active_material: r.i32()?, material_dirty: r.u8()? })
}

fn geometry_data(r: &mut Reader) -> Res<GeometryData> {
    let group_id = r.i32()?;
    let num_vertices = r.u16()?;
    let n = num_vertices as usize;
    let keep_flags = r.u8()?;
    let compress_flags = r.u8()?;
    let vertices = r.optional(n, |r| r.floats())?;
    let data_flags = r.u16()?;
    let (mut normals, mut tangents, mut bitangents) = (Vec::new(), Vec::new(), Vec::new());
    if r.u8()? != 0 {
        normals = r.list(n, |r| r.floats())?;
        if data_flags & 0x1000 != 0 {
            tangents = r.list(n, |r| r.floats())?;
            bitangents = r.list(n, |r| r.floats())?;
        }
    }
    let center = r.floats()?;
    let radius = r.f32()?;
    let colors = r.optional(n, |r| r.floats())?;
    let uv_sets = (0..(data_flags & 0x3F)).map(|_| r.list(n, |r| r.floats())).collect::<Res<_>>()?;
    Ok(GeometryData {
        group_id,
        num_vertices,
        keep_flags,
        compress_flags,
        vertices,
        data_flags,
        normals,
        tangents,
        bitangents,
        center,
        radius,
        colors,
        uv_sets,
        consistency: r.u16()?,
        additional_data: r.i32()?,
    })
}

fn source_texture(r: &mut Reader) -> Res<SourceTexture> {
    Ok(SourceTexture {
        net: object_net(r)?,
        use_external: r.u8()?,
        file_name: r.i32()?,
        pixel_data: r.i32()?,
        pixel_layout: r.u32()?,
        use_mipmaps: r.u32()?,
        alpha_format: r.u32()?,
        is_static: r.u8()?,
        direct_render: r.u8()?,
        persist_render_data: r.u8()?,
    })
}

fn tex_desc(r: &mut Reader) -> Res<TexDesc> {
    let source = r.i32()?;
    let flags = r.u16()?;
    let transform = if r.u8()? != 0 {
        Some(TexTransform { translation: r.floats()?, scale: r.floats()?, rotation: r.f32()?, method: r.u32()?, center: r.floats()? })
    } else {
        None
    };
    Ok(TexDesc { source, flags, transform })
}

fn skin_transform(r: &mut Reader) -> Res<SkinTransform> {
    Ok(SkinTransform { rotation: r.floats()?, translation: r.floats()?, scale: r.f32()? })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::archive::Archive;
    use std::collections::BTreeMap;

    const GAME: &str = r"C:\Program Files (x86)\Steam\steamapps\common\Bully Scholarship Edition";
    /// Texture dictionaries the container rules don't fit, all four big-endian (also unread
    /// by the other projects that looked at them): kept out of the strict checks, listed.
    const ODD: [&str; 4] = ["BBonusB.nft", "Barr01_Switch.nft", "BeerKeg.nft", "BirdBath.nft"];

    /// Level 0's size from its dimensions, per format: what fixes the format numbers.
    fn level0_bytes(format: u32, w: usize, h: usize) -> Option<usize> {
        let blocks = w.div_ceil(4) * h.div_ceil(4);
        Some(match format {
            FORMAT_RGB => w * h * 3,
            FORMAT_RGBA => w * h * 4,
            FORMAT_PALETTE => w * h,
            FORMAT_DXT1 => blocks * 8,
            FORMAT_DXT5 => blocks * 16,
            _ => return None,
        })
    }

    /// Every model and texture dictionary in World.img (skipped without the game; another
    /// install: BULLY_DIR=<game folder>). Every block of a decoded type uses exactly its size,
    /// every file ends in sector padding, and every texture's first level has the size its
    /// format and dimensions give. `cargo test --release world_img -- --nocapture` prints counts.
    #[test]
    fn every_file_in_world_img() {
        let dir = std::env::var("BULLY_DIR").unwrap_or_else(|_| GAME.into());
        let Ok(archive) = Archive::open(&std::path::Path::new(&dir).join("Stream")) else {
            eprintln!("skipped: no World.img under {dir}");
            return;
        };
        let (mut files, mut big, mut odd_seen) = (0, 0, Vec::new());
        let mut decoded: BTreeMap<&'static str, usize> = BTreeMap::new();
        let mut raw: BTreeMap<String, usize> = BTreeMap::new();
        let mut formats: BTreeMap<u32, usize> = BTreeMap::new();
        let mut failures = Vec::new();
        for e in &archive.entries {
            let lower = e.name.to_ascii_lowercase();
            if !(lower.ends_with(".nif") || lower.ends_with(".nft")) {
                continue;
            }
            files += 1;
            let odd = ODD.iter().any(|o| o.eq_ignore_ascii_case(&e.name));
            let data = archive.read(e).unwrap();
            let nif = match Nif::parse(&data) {
                Ok(n) => n,
                Err(err) => {
                    if odd { odd_seen.push(format!("{}: {err}", e.name)) } else { failures.push(format!("{}: {err}", e.name)) }
                    continue;
                }
            };
            big += nif.big_endian as usize;
            let tail = &data[nif.end..];
            let padding = tail.len() < 2048 && tail.iter().all(|&b| b == 0);
            if !padding || !nif.mismatched.is_empty() {
                let what = format!("{}: {} bytes after the roots, mismatched blocks {:?}", e.name, tail.len(),
                    nif.mismatched.iter().map(|&i| nif.type_name(i)).collect::<Vec<_>>());
                if odd { odd_seen.push(what) } else { failures.push(what) }
                continue;
            }
            for (i, b) in nif.blocks.iter().enumerate() {
                let kind = match b {
                    Block::Raw(_) => {
                        *raw.entry(nif.type_name(i).to_string()).or_default() += 1;
                        continue;
                    }
                    Block::Node(_) => "NiNode",
                    Block::TriShape(_) => "NiTriShape",
                    Block::TriStrips(_) => "NiTriStrips",
                    Block::TriShapeData(_) => "NiTriShapeData",
                    Block::TriStripsData(_) => "NiTriStripsData",
                    Block::SourceTexture(_) => "NiSourceTexture",
                    Block::SourceCubeMap(_) => "NiSourceCubeMap",
                    Block::Material(_) => "NiMaterialProperty",
                    Block::Texturing(_) => "NiTexturingProperty",
                    Block::Alpha(_) => "NiAlphaProperty",
                    Block::Stencil(_) => "NiStencilProperty",
                    Block::Palette(_) => "NiPalette",
                    Block::SkinInstance(_) => "NiSkinInstance",
                    Block::SkinData(_) => "NiSkinData",
                    Block::SkinPartition(_) => "NiSkinPartition",
                    Block::PixelData(p) => {
                        *formats.entry(p.pixel_format).or_default() += 1;
                        let m = p.mipmaps.first().copied().unwrap_or_default();
                        let want = level0_bytes(p.pixel_format, m.width as usize, m.height as usize);
                        let have = p.level(0, 0).map(|l| l.len());
                        if want.is_none() || want != have {
                            failures.push(format!("{}: block {i} format {} {}x{}: level 0 is {have:?} bytes, expected {want:?}", e.name, p.pixel_format, m.width, m.height));
                        }
                        "NiPixelData"
                    }
                };
                *decoded.entry(kind).or_default() += 1;
            }
        }
        println!("{files} files ({big} big-endian)");
        println!("decoded, every one using exactly its size: {decoded:?}");
        println!("kept raw (types not decoded): {raw:?}");
        println!("pixel formats: {formats:?}");
        println!("odd files: {odd_seen:#?}");
        assert!(files > 10_000, "only {files} models and texture dictionaries");
        assert!(failures.is_empty(), "{} failures, first: {:#?}", failures.len(), &failures[..failures.len().min(10)]);
    }
}
