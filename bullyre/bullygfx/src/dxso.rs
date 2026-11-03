//! Direct3D 9 shader bytecode (DXSO) to WGSL, so the new renderer can run the game's
//! own shaders and match its picture exactly.
//!
//! Covers what Bully uses (vs/ps 2.0 and 3.0; see NOTES.md for the survey): the
//! arithmetic instructions, tex/texldp/texldb on 2D and cube samplers, cmp, relative
//! constant addressing through a0, def/defi/defb, swizzles, write masks, source
//! modifiers and saturate. Anything else is refused with an error rather than guessed.
//!
//! The interface every translated shader has, so any pair can be bound the same way:
//! - group 0 binding 0: vertex shader constants, `array<vec4<f32>, 256>` (c0..c255)
//! - group 0 binding 1: pixel shader constants, `array<vec4<f32>, 224>`
//! - group 0 binding 2: `Frame { half_pixel: vec4<f32> }`: D3D9 pixel centres sit half a
//!   pixel up and left of modern APIs'; the vertex shader moves positions right and down
//!   by that much
//! - group 1: sampler n is texture binding 2n and sampler binding 2n + 1
//! - vertex inputs: `@location(n)` for v<n>; the pipeline maps the vertex declaration
//!   onto these by usage (`Shader::inputs`)
//! - vertex outputs / pixel inputs: by semantic (`varying_location`); the vertex shader
//!   always writes all of them (zero if the game's didn't), so any pairing links

use std::collections::{BTreeMap, BTreeSet};
use std::fmt::Write;

pub const VS_CONSTANTS: usize = 256;
pub const PS_CONSTANTS: usize = 224;
/// Inter-stage locations the vertex shader always writes.
pub const VARYINGS: u32 = 14;

// declaration usages (D3DDECLUSAGE)
pub const USAGE_POSITION: u32 = 0;
pub const USAGE_BLENDWEIGHT: u32 = 1;
pub const USAGE_BLENDINDICES: u32 = 2;
pub const USAGE_NORMAL: u32 = 3;
pub const USAGE_PSIZE: u32 = 4;
pub const USAGE_TEXCOORD: u32 = 5;
pub const USAGE_TANGENT: u32 = 6;
pub const USAGE_BINORMAL: u32 = 7;
pub const USAGE_COLOR: u32 = 10;
pub const USAGE_FOG: u32 = 11;

/// Where a vertex shader output / pixel shader input with this semantic lives.
pub fn varying_location(usage: u32, index: u32) -> Option<u32> {
    match usage {
        USAGE_TEXCOORD if index < 10 => Some(index),
        USAGE_COLOR if index < 2 => Some(10 + index),
        USAGE_FOG => Some(12),
        USAGE_PSIZE => Some(13),
        // vs_3_0 may output these as extra texture coordinates
        USAGE_NORMAL | USAGE_TANGENT | USAGE_BINORMAL | USAGE_POSITION if index < 10 => None,
        _ => None,
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    Vertex,
    Pixel,
}

// register types (D3DSPR_*)
const R_TEMP: u32 = 0;
const R_INPUT: u32 = 1;
const R_CONST: u32 = 2;
const R_ADDR_TEXTURE: u32 = 3; // a0 in vertex shaders, t# in ps_2_0
const R_RASTOUT: u32 = 4;
const R_ATTROUT: u32 = 5;
const R_OUTPUT: u32 = 6; // oT# in vs_2_0, o# in vs_3_0
const R_CONSTINT: u32 = 7;
const R_COLOROUT: u32 = 8;
const R_DEPTHOUT: u32 = 9;
const R_SAMPLER: u32 = 10;
const R_CONSTBOOL: u32 = 14;
const R_MISCTYPE: u32 = 17;

#[derive(Clone, Copy, Debug)]
struct Reg {
    ty: u32,
    num: u32,
    /// Relative addressing: the address register's component (0..3).
    rel: Option<u32>,
}

#[derive(Clone, Copy, Debug)]
struct Dst {
    reg: Reg,
    mask: u32,
    saturate: bool,
}

#[derive(Clone, Copy, Debug)]
struct Src {
    reg: Reg,
    swizzle: [u32; 4],
    modifier: u32,
}

#[derive(Debug)]
struct Instr {
    op: u32,
    control: u32,
    dst: Option<Dst>,
    src: Vec<Src>,
}

pub struct Shader {
    pub kind: Kind,
    pub major: u32,
    pub minor: u32,
    instrs: Vec<Instr>,
    /// dcl'd inputs: register number -> (usage, index)
    pub inputs: BTreeMap<u32, (u32, u32)>,
    /// vs_3_0 dcl'd outputs: register number -> (usage, index)
    outputs: BTreeMap<u32, (u32, u32)>,
    /// sampler number -> texture type (2 = 2D, 3 = cube, 4 = volume)
    pub samplers: BTreeMap<u32, u32>,
    defs: BTreeMap<u32, [f32; 4]>,
    defis: BTreeMap<u32, [i32; 4]>,
    defbs: BTreeMap<u32, bool>,
}

// opcodes (D3DSIO_*)
const OP_NOP: u32 = 0;
const OP_MOV: u32 = 1;
const OP_ADD: u32 = 2;
const OP_SUB: u32 = 3;
const OP_MAD: u32 = 4;
const OP_MUL: u32 = 5;
const OP_RCP: u32 = 6;
const OP_RSQ: u32 = 7;
const OP_DP3: u32 = 8;
const OP_DP4: u32 = 9;
const OP_MIN: u32 = 10;
const OP_MAX: u32 = 11;
const OP_SLT: u32 = 12;
const OP_SGE: u32 = 13;
const OP_EXP: u32 = 14;
const OP_LOG: u32 = 15;
const OP_LRP: u32 = 18;
const OP_FRC: u32 = 19;
const OP_DCL: u32 = 31;
const OP_POW: u32 = 32;
const OP_CRS: u32 = 33;
const OP_ABS: u32 = 35;
const OP_NRM: u32 = 36;
const OP_SINCOS: u32 = 37;
const OP_MOVA: u32 = 46;
const OP_DEFB: u32 = 47;
const OP_DEFI: u32 = 48;
const OP_TEX: u32 = 66;
const OP_DEF: u32 = 81;
const OP_CMP: u32 = 88;
const OP_DP2ADD: u32 = 90;
const OP_COMMENT: u32 = 0xFFFE;
const OP_END: u32 = 0xFFFF;

fn reg_of(p: u32) -> Reg {
    Reg { ty: ((p >> 28) & 7) | ((p >> 8) & 0x18), num: p & 0x7FF, rel: None }
}

impl Shader {
    /// How many float constant registers it reads (c0 up to the highest used), or
    /// None when it indexes them through a0 (then any could be read).
    pub fn float_constants_read(&self) -> Option<u32> {
        let mut n = 0;
        for i in &self.instrs {
            for s in &i.src {
                if s.reg.ty == R_CONST {
                    if s.reg.rel.is_some() {
                        return None;
                    }
                    n = n.max(s.reg.num + 1);
                }
            }
        }
        Some(n)
    }
}

pub fn parse(tokens: &[u32]) -> Result<Shader, String> {
    let version = *tokens.first().ok_or("empty")?;
    let kind = match version >> 16 {
        0xFFFE => Kind::Vertex,
        0xFFFF => Kind::Pixel,
        _ => return Err(format!("not a shader: {version:#x}")),
    };
    let (major, minor) = ((version >> 8) & 0xFF, version & 0xFF);
    if !(2..=3).contains(&major) {
        return Err(format!("shader model {major}.{minor} not handled"));
    }
    let mut sh = Shader {
        kind,
        major,
        minor,
        instrs: Vec::new(),
        inputs: BTreeMap::new(),
        outputs: BTreeMap::new(),
        samplers: BTreeMap::new(),
        defs: BTreeMap::new(),
        defis: BTreeMap::new(),
        defbs: BTreeMap::new(),
    };
    let mut i = 1;
    let tok = |i: usize| tokens.get(i).copied().ok_or_else(|| "truncated".to_string());
    while i < tokens.len() {
        let t = tokens[i];
        let op = t & 0xFFFF;
        if op == OP_COMMENT {
            i += 1 + ((t >> 16) & 0x7FFF) as usize;
            continue;
        }
        if op == OP_END {
            return Ok(sh);
        }
        let len = ((t >> 24) & 0xF) as usize;
        if t & (1 << 28) != 0 {
            return Err("predicated instructions not handled".into());
        }
        let control = (t >> 16) & 0xFF;
        let body = i + 1;
        let next = body + len;
        match op {
            OP_DCL => {
                let usage = tok(body)?;
                let d = tok(body + 1)?;
                let r = reg_of(d);
                match r.ty {
                    R_SAMPLER => {
                        sh.samplers.insert(r.num, (usage >> 27) & 0xF);
                    }
                    R_INPUT | R_ADDR_TEXTURE if kind == Kind::Vertex || r.ty == R_INPUT => {
                        sh.inputs.insert(r.num, (usage & 0x1F, (usage >> 16) & 0xF));
                    }
                    R_ADDR_TEXTURE => {
                        // ps_2_0 t#: texture coordinates
                        sh.inputs.insert(0x100 | r.num, (USAGE_TEXCOORD, r.num));
                    }
                    R_OUTPUT => {
                        sh.outputs.insert(r.num, (usage & 0x1F, (usage >> 16) & 0xF));
                    }
                    R_MISCTYPE => return Err("vPos/vFace not handled".into()),
                    _ => {}
                }
            }
            OP_DEF => {
                let r = reg_of(tok(body)?);
                let v = [1, 2, 3, 4].map(|k| f32::from_bits(tokens.get(body + k).copied().unwrap_or(0)));
                sh.defs.insert(r.num, v);
            }
            OP_DEFI => {
                let r = reg_of(tok(body)?);
                let v = [1, 2, 3, 4].map(|k| tokens.get(body + k).copied().unwrap_or(0) as i32);
                sh.defis.insert(r.num, v);
            }
            OP_DEFB => {
                let r = reg_of(tok(body)?);
                sh.defbs.insert(r.num, tok(body + 1)? != 0);
            }
            _ => {
                // destination then sources, each possibly followed by a relative
                // addressing token
                let has_dst = !matches!(op, OP_NOP);
                let mut k = body;
                let mut read = |k: &mut usize, is_dst: bool| -> Result<(u32, Reg), String> {
                    let p = tok(*k)?;
                    *k += 1;
                    let mut r = reg_of(p);
                    if p & (1 << 13) != 0 {
                        if is_dst {
                            return Err("relative destination not handled".into());
                        }
                        let a = tok(*k)?;
                        *k += 1;
                        r.rel = Some((a >> 16) & 3); // the address register's .x/.y/.z/.w
                    }
                    Ok((p, r))
                };
                let mut dst = None;
                if has_dst {
                    let (p, r) = read(&mut k, true)?;
                    if (p >> 20) & 0xE != 0 {
                        // partial precision / centroid: fine to ignore; shift isn't
                    }
                    if (p >> 24) & 0xF != 0 {
                        return Err("destination shift not handled".into());
                    }
                    dst = Some(Dst { reg: r, mask: (p >> 16) & 0xF, saturate: (p >> 20) & 1 != 0 });
                }
                let mut src = Vec::new();
                while k < next {
                    let (p, r) = read(&mut k, false)?;
                    let sw = (p >> 16) & 0xFF;
                    src.push(Src { reg: r, swizzle: [sw & 3, (sw >> 2) & 3, (sw >> 4) & 3, (sw >> 6) & 3], modifier: (p >> 24) & 0xF });
                }
                sh.instrs.push(Instr { op, control, dst, src });
            }
        }
        i = next;
    }
    Err("no end token".into())
}

const COMP: [char; 4] = ['x', 'y', 'z', 'w'];

impl Shader {
    fn is_vs(&self) -> bool {
        self.kind == Kind::Vertex
    }

    /// Temporaries, address registers and outputs used.
    fn used(&self) -> (BTreeSet<u32>, bool) {
        let mut temps = BTreeSet::new();
        let mut addr = false;
        for ins in &self.instrs {
            for r in ins.dst.iter().map(|d| d.reg).chain(ins.src.iter().map(|s| s.reg)) {
                match r.ty {
                    R_TEMP => {
                        temps.insert(r.num);
                    }
                    R_ADDR_TEXTURE if self.is_vs() => addr = true,
                    _ => {}
                }
                if r.rel.is_some() {
                    addr = true;
                }
            }
        }
        (temps, addr)
    }

    /// The WGSL for a register read, as a vec4<f32>.
    fn reg_read(&self, r: &Reg) -> Result<String, String> {
        let rel = |base: &str, n: u32| match r.rel {
            Some(c) => format!("{base}[clamp(a0.{} + {n}, 0, {})]", COMP[c as usize], if self.is_vs() { VS_CONSTANTS - 1 } else { PS_CONSTANTS - 1 }),
            None => format!("{base}[{n}]"),
        };
        Ok(match r.ty {
            R_TEMP => format!("r{}", r.num),
            R_INPUT if self.is_vs() => format!("v{}", r.num),
            R_INPUT => format!("inp.v{}", r.num),
            R_ADDR_TEXTURE if self.is_vs() => "vec4<f32>(a0)".to_string(),
            R_ADDR_TEXTURE => format!("inp.t{}", r.num),
            R_CONST => {
                if r.rel.is_none()
                    && let Some(v) = self.defs.get(&r.num)
                {
                    format!("vec4<f32>({:?}, {:?}, {:?}, {:?})", v[0], v[1], v[2], v[3])
                } else if self.is_vs() {
                    rel("vc", r.num)
                } else {
                    rel("pc", r.num)
                }
            }
            R_CONSTINT => {
                let v = self.defis.get(&r.num).ok_or("integer constant without def")?;
                format!("vec4<f32>({:?}, {:?}, {:?}, {:?})", v[0] as f32, v[1] as f32, v[2] as f32, v[3] as f32)
            }
            R_CONSTBOOL => {
                let v = *self.defbs.get(&r.num).ok_or("bool constant without def")?;
                format!("vec4<f32>({})", if v { 1.0 } else { 0.0 })
            }
            _ => return Err(format!("reading register type {} not handled", r.ty)),
        })
    }

    fn src(&self, s: &Src) -> Result<String, String> {
        let base = self.reg_read(&s.reg)?;
        let sw: String = s.swizzle.iter().map(|&c| COMP[c as usize]).collect();
        let v = if sw == "xyzw" { base } else { format!("({base}).{sw}") };
        Ok(match s.modifier {
            0 => v,
            1 => format!("(-{v})"),
            2 => format!("({v} - vec4<f32>(0.5))"),
            3 => format!("(-({v} - vec4<f32>(0.5)))"),
            4 => format!("({v} * 2.0 - vec4<f32>(1.0))"),
            5 => format!("(-({v} * 2.0 - vec4<f32>(1.0)))"),
            6 => format!("(vec4<f32>(1.0) - {v})"),
            7 => format!("({v} * 2.0)"),
            8 => format!("(-({v} * 2.0))"),
            11 => format!("abs({v})"),
            12 => format!("(-abs({v}))"),
            m => return Err(format!("source modifier {m} not handled")),
        })
    }

    /// The WGSL lvalue for a destination register.
    fn dst_name(&self, r: &Reg) -> Result<String, String> {
        Ok(match (self.kind, r.ty) {
            (_, R_TEMP) => format!("r{}", r.num),
            (Kind::Vertex, R_ADDR_TEXTURE) => "a0f".to_string(),
            (Kind::Vertex, R_RASTOUT) => match r.num {
                0 => "o_pos".to_string(),
                1 => "o_fog".to_string(),
                2 => "o_psize".to_string(),
                n => return Err(format!("rastout {n}")),
            },
            (Kind::Vertex, R_ATTROUT) => format!("o_color{}", r.num),
            (Kind::Vertex, R_OUTPUT) => format!("o{}", r.num),
            (Kind::Pixel, R_COLOROUT) => format!("oc{}", r.num),
            (Kind::Pixel, R_DEPTHOUT) => "o_depth".to_string(),
            (_, t) => return Err(format!("writing register type {t} not handled")),
        })
    }

    fn emit_instr(&self, ins: &Instr, out: &mut String) -> Result<(), String> {
        let s = |i: usize| -> Result<String, String> { self.src(ins.src.get(i).ok_or("missing source")?) };
        let scalar = |e: String| format!("vec4<f32>({e})");
        let value = match ins.op {
            OP_NOP => return Ok(()),
            OP_MOV => s(0)?,
            OP_ADD => format!("({} + {})", s(0)?, s(1)?),
            OP_SUB => format!("({} - {})", s(0)?, s(1)?),
            OP_MUL => format!("({} * {})", s(0)?, s(1)?),
            OP_MAD => format!("({} * {} + {})", s(0)?, s(1)?, s(2)?),
            // D3D9: rcp(0) = +inf, rsq(0) = +inf, rsq of |x|
            OP_RCP => scalar(format!("d3d_rcp(({}).x)", s(0)?)),
            OP_RSQ => scalar(format!("d3d_rsq(({}).x)", s(0)?)),
            OP_DP3 => scalar(format!("dot(({}).xyz, ({}).xyz)", s(0)?, s(1)?)),
            OP_DP4 => scalar(format!("dot({}, {})", s(0)?, s(1)?)),
            OP_MIN => format!("min({}, {})", s(0)?, s(1)?),
            OP_MAX => format!("max({}, {})", s(0)?, s(1)?),
            OP_SLT => format!("select(vec4<f32>(0.0), vec4<f32>(1.0), {} < {})", s(0)?, s(1)?),
            OP_SGE => format!("select(vec4<f32>(0.0), vec4<f32>(1.0), {} >= {})", s(0)?, s(1)?),
            OP_EXP => scalar(format!("exp2(({}).x)", s(0)?)),
            OP_LOG => scalar(format!("d3d_log(({}).x)", s(0)?)),
            OP_LRP => {
                let (a, b, c) = (s(0)?, s(1)?, s(2)?);
                format!("({a} * ({b} - {c}) + {c})")
            }
            OP_FRC => format!("fract({})", s(0)?),
            OP_POW => scalar(format!("d3d_pow(({}).x, ({}).x)", s(0)?, s(1)?)),
            OP_CRS => format!("vec4<f32>(cross(({}).xyz, ({}).xyz), 0.0)", s(0)?, s(1)?),
            OP_ABS => format!("abs({})", s(0)?),
            // nrm: all four components scaled by 1/|xyz|
            OP_NRM => {
                let a = s(0)?;
                format!("({a} * d3d_rsq(dot(({a}).xyz, ({a}).xyz)))")
            }
            OP_SINCOS => {
                let a = s(0)?;
                format!("vec4<f32>(cos(({a}).x), sin(({a}).x), 0.0, 0.0)")
            }
            OP_CMP => format!("select({}, {}, {} >= vec4<f32>(0.0))", s(2)?, s(1)?, s(0)?),
            OP_DP2ADD => scalar(format!("dot(({}).xy, ({}).xy) + ({}).x", s(0)?, s(1)?, s(2)?)),
            OP_MOVA => {
                // vs_3_0 rounds to nearest; the result goes to the integer a0
                let v = s(0)?;
                let d = ins.dst.ok_or("mova without destination")?;
                for c in 0..4 {
                    if d.mask & (1 << c) != 0 {
                        let _ = writeln!(out, "    a0.{0} = i32(round(({v}).{0}));", COMP[c]);
                    }
                }
                return Ok(());
            }
            OP_TEX => {
                let coords = s(0)?;
                let sampler = ins.src.get(1).ok_or("tex without sampler")?.reg;
                if sampler.ty != R_SAMPLER {
                    return Err("tex from a non-sampler".into());
                }
                let n = sampler.num;
                let ty = *self.samplers.get(&n).ok_or("tex from an undeclared sampler")?;
                let (t, sm) = (format!("t{n}"), format!("s{n}"));
                let project = ins.control & 1 != 0;
                let bias = ins.control & 2 != 0;
                let uv = match (ty, project) {
                    (3, _) => format!("({coords}).xyz"),
                    (2, true) => format!("(({coords}).xy / ({coords}).w)"),
                    (2, false) => format!("({coords}).xy"),
                    (t, _) => return Err(format!("sampler type {t} not handled")),
                };
                if self.is_vs() {
                    format!("textureSampleLevel({t}, {sm}, {uv}, 0.0)")
                } else if bias {
                    format!("textureSampleBias({t}, {sm}, {uv}, ({coords}).w)")
                } else {
                    format!("textureSample({t}, {sm}, {uv})")
                }
            }
            op => return Err(format!("opcode {op} not handled")),
        };
        let d = ins.dst.ok_or("instruction without destination")?;
        let name = self.dst_name(&d.reg)?;
        let v = if d.saturate { format!("clamp({value}, vec4<f32>(0.0), vec4<f32>(1.0))") } else { value };
        if d.reg.ty == R_ADDR_TEXTURE && self.is_vs() {
            return Err("writing a0 other than with mova".into());
        }
        if d.mask == 0xF {
            let _ = writeln!(out, "    {name} = {v};");
        } else {
            let _ = writeln!(out, "    {{ let t = {v};");
            for c in 0..4 {
                if d.mask & (1 << c) != 0 {
                    let _ = writeln!(out, "    {name}.{0} = t.{0};", COMP[c]);
                }
            }
            let _ = writeln!(out, "    }}");
        }
        Ok(())
    }

    /// A pixel shader input register's semantic.
    fn ps_input_semantic(&self, reg: u32) -> Option<(u32, u32)> {
        if reg & 0x100 != 0 {
            return Some((USAGE_TEXCOORD, reg & 0xFF));
        }
        if self.major >= 3 {
            self.inputs.get(&reg).copied()
        } else {
            // ps_2_0 v0/v1: the colours
            Some((USAGE_COLOR, reg))
        }
    }

    /// With every vertex input a float (what the checks use).
    pub fn to_wgsl(&self) -> Result<String, String> {
        let all = self.inputs.keys().map(|&n| (n, Input::Float)).collect();
        self.to_wgsl_with(&all)
    }

    /// `inputs`: the vertex inputs the pipeline supplies and their data type; the rest
    /// read as (0, 0, 0, 1).
    pub fn to_wgsl_with(&self, inputs: &BTreeMap<u32, Input>) -> Result<String, String> {
        let mut o = String::new();
        let (temps, addr) = self.used();
        let _ = writeln!(o, "// translated from {}s_{}_{} by bullygfx", if self.is_vs() { 'v' } else { 'p' }, self.major, self.minor);
        let _ = writeln!(o, "@group(0) @binding(0) var<uniform> vc: array<vec4<f32>, {VS_CONSTANTS}>;");
        let _ = writeln!(o, "@group(0) @binding(1) var<uniform> pc: array<vec4<f32>, {PS_CONSTANTS}>;");
        // half_pixel: xy; alpha: x = the alpha test's D3DCMP function (0 = off), y = reference
        let _ = writeln!(o, "struct Frame {{ half_pixel: vec4<f32>, alpha: vec4<f32> }};\n@group(0) @binding(2) var<uniform> frame: Frame;");
        for (&n, &ty) in &self.samplers {
            let tt = if ty == 3 { "texture_cube<f32>" } else { "texture_2d<f32>" };
            let _ = writeln!(o, "@group(1) @binding({}) var t{n}: {tt};\n@group(1) @binding({}) var s{n}: sampler;", 2 * n, 2 * n + 1);
        }
        o.push_str(HELPERS);

        // the varyings: every location, always
        let _ = writeln!(o, "struct Varyings {{\n    @builtin(position) pos: vec4<f32>,");
        for l in 0..VARYINGS {
            let _ = writeln!(o, "    @location({l}) l{l}: vec4<f32>,");
        }
        let _ = writeln!(o, "}};");

        let mut body = String::new();
        for ins in &self.instrs {
            self.emit_instr(ins, &mut body)?;
        }

        if self.is_vs() {
            let mut params = Vec::new();
            let mut prologue = String::new();
            for &n in self.inputs.keys() {
                match inputs.get(&n) {
                    Some(Input::Float) => params.push(format!("@location({n}) v{n}: vec4<f32>")),
                    Some(Input::Uint) => {
                        params.push(format!("@location({n}) v{n}_in: vec4<u32>"));
                        let _ = writeln!(prologue, "    let v{n} = vec4<f32>(v{n}_in);");
                    }
                    Some(Input::Sint) => {
                        params.push(format!("@location({n}) v{n}_in: vec4<i32>"));
                        let _ = writeln!(prologue, "    let v{n} = vec4<f32>(v{n}_in);");
                    }
                    None => {
                        let _ = writeln!(prologue, "    let v{n} = vec4<f32>(0.0, 0.0, 0.0, 1.0);");
                    }
                }
            }
            let _ = writeln!(o, "@vertex fn main({}) -> Varyings {{", params.join(", "));
            o.push_str(&prologue);
            for t in &temps {
                let _ = writeln!(o, "    var r{t} = vec4<f32>(0.0);");
            }
            if addr {
                let _ = writeln!(o, "    var a0 = vec4<i32>(0);\n    var a0f = vec4<f32>(0.0);");
            }
            // outputs
            let mut outs: Vec<(String, Option<u32>)> = Vec::new(); // (name, location); None = position
            if self.major >= 3 {
                for (&n, &(usage, index)) in &self.outputs {
                    let loc = if usage == USAGE_POSITION && index == 0 { None } else { Some(varying_location(usage, index).unwrap_or(u32::MAX)) };
                    outs.push((format!("o{n}"), loc));
                }
            } else {
                outs.push(("o_pos".into(), None));
                for n in 0..8 {
                    outs.push((format!("o{n}"), Some(n)));
                }
                outs.push(("o_color0".into(), Some(10)));
                outs.push(("o_color1".into(), Some(11)));
                outs.push(("o_fog".into(), Some(12)));
                outs.push(("o_psize".into(), Some(13)));
            }
            for (name, _) in &outs {
                let _ = writeln!(o, "    var {name} = vec4<f32>(0.0);");
            }
            o.push_str(&body);
            let _ = writeln!(o, "    var out: Varyings;");
            for l in 0..VARYINGS {
                let _ = writeln!(o, "    out.l{l} = vec4<f32>(0.0);");
            }
            let mut pos = "vec4<f32>(0.0, 0.0, 0.0, 1.0)".to_string();
            for (name, loc) in &outs {
                match loc {
                    None => pos = name.clone(),
                    Some(l) if *l < VARYINGS => {
                        let _ = writeln!(o, "    out.l{l} = {name};");
                    }
                    Some(_) => {} // an output nothing reads
                }
            }
            // D3D9's pixel centres: move by half a pixel, as the rasterizer won't
            let _ = writeln!(o, "    out.pos = {pos};\n    out.pos.x = out.pos.x + frame.half_pixel.x * out.pos.w;\n    out.pos.y = out.pos.y - frame.half_pixel.y * out.pos.w;");
            let _ = writeln!(o, "    return out;\n}}");
        } else {
            let _ = writeln!(o, "struct Inputs {{");
            let mut fields = Vec::new();
            for &n in self.inputs.keys() {
                let name = if n & 0x100 != 0 { format!("t{}", n & 0xFF) } else { format!("v{n}") };
                let sem = self.ps_input_semantic(n).ok_or(format!("pixel input {n} without a semantic"))?;
                let loc = varying_location(sem.0, sem.1).ok_or(format!("pixel input semantic {sem:?} has no location"))?;
                fields.push((name, loc));
            }
            // ps_2_0 reads v0/v1 without always declaring them
            if self.major < 3 {
                for c in 0..2 {
                    if !fields.iter().any(|f| f.0 == format!("v{c}")) {
                        fields.push((format!("v{c}"), 10 + c));
                    }
                }
            }
            let mut used_locs = BTreeSet::new();
            for (name, loc) in &fields {
                if used_locs.insert(*loc) {
                    let _ = writeln!(o, "    @location({loc}) {name}: vec4<f32>,");
                }
            }
            if used_locs.is_empty() {
                // no inputs (a flat colour): WGSL has no empty structs
                let _ = writeln!(o, "    @builtin(position) unused_pos: vec4<f32>,");
            }
            let _ = writeln!(o, "}};");
            let _ = writeln!(o, "@fragment fn main(inp: Inputs) -> @location(0) vec4<f32> {{");
            for t in &temps {
                let _ = writeln!(o, "    var r{t} = vec4<f32>(0.0);");
            }
            let _ = writeln!(o, "    var oc0 = vec4<f32>(0.0);\n    var oc1 = vec4<f32>(0.0);\n    var oc2 = vec4<f32>(0.0);\n    var oc3 = vec4<f32>(0.0);\n    var o_depth = vec4<f32>(0.0);");
            o.push_str(&body);
            // D3D9's alpha test (a render state, not part of the shader)
            let _ = writeln!(o, "{ALPHA_TEST}    return oc0;\n}}");
        }
        Ok(o)
    }
}

/// D3DRS_ALPHAFUNC against D3DRS_ALPHAREF: discard what fails.
const ALPHA_TEST: &str = r#"    let alpha_func = u32(frame.alpha.x);
    if alpha_func != 0u {
        let a = oc0.w;
        let r = frame.alpha.y;
        var passes = true;
        switch alpha_func {
            case 1u: { passes = false; }
            case 2u: { passes = a < r; }
            case 3u: { passes = a == r; }
            case 4u: { passes = a <= r; }
            case 5u: { passes = a > r; }
            case 6u: { passes = a != r; }
            case 7u: { passes = a >= r; }
            default: { passes = true; }
        }
        if !passes {
            discard;
        }
    }
"#;

/// A vertex input's data type as the pipeline delivers it.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Input {
    Float,
    Uint,
    Sint,
}

/// D3D9's edge cases, which WGSL leaves undefined or different.
const HELPERS: &str = r#"
fn d3d_rcp(x: f32) -> f32 {
    if x == 0.0 { return 3.4028235e38; }
    return 1.0 / x;
}
fn d3d_rsq(x: f32) -> f32 {
    let a = abs(x);
    if a == 0.0 { return 3.4028235e38; }
    return inverseSqrt(a);
}
fn d3d_log(x: f32) -> f32 {
    let a = abs(x);
    if a == 0.0 { return -3.4028235e38; }
    return log2(a);
}
fn d3d_pow(x: f32, y: f32) -> f32 {
    let a = abs(x);
    if a == 0.0 {
        if y == 0.0 { return 1.0; }
        return 0.0;
    }
    return exp2(y * log2(a));
}
"#;

pub fn translate(tokens: &[u32]) -> Result<(Shader, String), String> {
    let sh = parse(tokens)?;
    let text = sh.to_wgsl()?;
    Ok((sh, text))
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Every shader collected from the game translates, and wgpu's own front end
    /// (naga) accepts the result.
    #[test]
    fn all_game_shaders_translate_and_validate() {
        let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("testdata/shaders");
        let mut total = 0;
        let mut failures = Vec::new();
        for entry in std::fs::read_dir(&dir).unwrap() {
            let path = entry.unwrap().path();
            let bytes = std::fs::read(&path).unwrap();
            let tokens: Vec<u32> = bytes.chunks_exact(4).map(|c| u32::from_le_bytes(c.try_into().unwrap())).collect();
            total += 1;
            let name = path.file_name().unwrap().to_string_lossy().to_string();
            let text = match translate(&tokens) {
                Ok((_, t)) => t,
                Err(e) => {
                    failures.push(format!("{name}: translate: {e}"));
                    continue;
                }
            };
            let module = match naga::front::wgsl::parse_str(&text) {
                Ok(m) => m,
                Err(e) => {
                    failures.push(format!("{name}: wgsl: {}\n{text}", e.emit_to_string(&text)));
                    continue;
                }
            };
            if let Err(e) = naga::valid::Validator::new(naga::valid::ValidationFlags::all(), naga::valid::Capabilities::all()).validate(&module) {
                failures.push(format!("{name}: validate: {e:?}"));
            }
        }
        assert!(total > 0, "no shaders in {}", dir.display());
        assert!(failures.is_empty(), "{} of {total} failed:\n{}", failures.len(), failures.iter().take(5).cloned().collect::<Vec<_>>().join("\n\n"));
    }
}

#[cfg(test)]
mod print {
    /// `cargo test print_shader -- --nocapture` with BULLYGFX_SHADER=<file name>.
    #[test]
    fn print_shader() {
        let Ok(name) = std::env::var("BULLYGFX_SHADER") else { return };
        let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("testdata/shaders").join(name);
        let bytes = std::fs::read(path).unwrap();
        let tokens: Vec<u32> = bytes.chunks_exact(4).map(|c| u32::from_le_bytes(c.try_into().unwrap())).collect();
        println!("{}", super::translate(&tokens).map(|(_, t)| t).unwrap_or_else(|e| e));
    }
}
