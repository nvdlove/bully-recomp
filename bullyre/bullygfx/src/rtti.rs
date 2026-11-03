//! What a value passed to the renderer is: a Gamebryo object (named from the exe's
//! MSVC RTTI), a small number, a float, or some other pointer.

use crate::win::{read_bytes, read_u32};

/// The exe's .rdata (vtables, complete object locators) and .data (type descriptors).
const RDATA: std::ops::Range<u32> = 0x008F_C000..0x009C_4000;
const DATA: std::ops::Range<u32> = 0x009C_4000..0x020E_C000;

/// The class of the object at `p`: Gamebryo's own RTTI, else MSVC's.
pub fn class_of(p: u32) -> Option<String> {
    let vtable = read_u32(p)?;
    if !RDATA.contains(&vtable) {
        return None;
    }
    ni_class(vtable).or_else(|| msvc_class(vtable))
}

fn c_string(addr: u32) -> Option<String> {
    let mut buf = [0u8; 64];
    if !read_bytes(addr, &mut buf) {
        return None;
    }
    let len = buf.iter().position(|&c| c == 0)?;
    let s = std::str::from_utf8(&buf[..len]).ok()?;
    (!s.is_empty() && s.bytes().all(|c| c.is_ascii_graphic())).then(|| s.to_string())
}

/// Gamebryo objects (built without MSVC RTTI): vtable slot 2 is GetRTTI, compiled as
/// `mov eax, &NiRTTI; ret`, and an NiRTTI starts with a pointer to the class name.
fn ni_class(vtable: u32) -> Option<String> {
    let get_rtti = read_u32(vtable + 8)?;
    let mut code = [0u8; 6];
    if !read_bytes(get_rtti, &mut code) || code[0] != 0xB8 || code[5] != 0xC3 {
        return None;
    }
    let rtti = u32::from_le_bytes(code[1..5].try_into().unwrap());
    c_string(read_u32(rtti)?)
}

/// The vtable's complete object locator names it.
fn msvc_class(vtable: u32) -> Option<String> {
    let col = read_u32(vtable - 4)?;
    if !RDATA.contains(&col) || read_u32(col)? != 0 {
        return None;
    }
    let td = read_u32(col + 12)?;
    if !DATA.contains(&td) {
        return None;
    }
    let mut name = [0u8; 80];
    if !read_bytes(td + 8, &mut name) {
        return None;
    }
    let len = name.iter().position(|&c| c == 0)?;
    let s = std::str::from_utf8(&name[..len]).ok()?;
    let s = s.strip_prefix(".?AV").or_else(|| s.strip_prefix(".?AU"))?;
    Some(s.strip_suffix("@@").unwrap_or(s).to_string())
}

/// Short form for grouping calls: the class, "int N", "float", "ptr" or "?".
pub fn kind(v: u32) -> String {
    if let Some(c) = class_of(v) {
        return c;
    }
    if v < 0x1_0000 {
        return format!("{v}");
    }
    let f = f32::from_bits(v);
    if f.is_finite() && (1e-4..1e7).contains(&f.abs()) {
        return "float".into();
    }
    if read_u32(v).is_some() { "ptr".into() } else { format!("{v:#x}") }
}

/// Long form for one example call: floats' values, and what a plain pointer points at.
pub fn detail(v: u32) -> String {
    let k = kind(v);
    match k.as_str() {
        "float" => format!("{:.3}", f32::from_bits(v)),
        "ptr" => {
            let words: Vec<String> = (0..4)
                .filter_map(|i| read_u32(v + 4 * i))
                .map(|w| {
                    let f = f32::from_bits(w);
                    if f.is_finite() && (1e-4..1e7).contains(&f.abs()) { format!("{f:.2}") } else { format!("{w:#x}") }
                })
                .collect();
            format!("{v:#x}->[{}]", words.join(","))
        }
        _ => k,
    }
}
