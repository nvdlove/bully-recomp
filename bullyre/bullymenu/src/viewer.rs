//! The Live Data page: everything bullydumper mapped (readable_offsets.csv, the Lua
//! natives, the functions), with values read out of the running game.
//!
//! Addresses are the dumper's notation: `0xADDR`, `[x]` = the pointer stored at x,
//! `+0x..` offsets, and a struct name (`ped+0x..`, `CPed+0x..`, `vehicle+0x..`) for fields
//! of an object, read from the player's ped or the player's vehicle.

use std::path::PathBuf;

use crate::game::Game;

const PLAYER: u32 = 0x00C1_AEA8;
const PED_POOL: u32 = 0x00C0_F5F0;
const VEHICLE_POOL: u32 = 0x00C0_F5F4;
const CLOCK_HOUR: u32 = 0x00BC_EB4B;
const CLOCK_MINUTE: u32 = 0x00BC_EB4A;
const CLOCK_PAUSED: u32 = 0x00BC_EB42;
const CHAPTER: u32 = 0x00A1_47E0;
const AREA: u32 = 0x00BD_1008;

pub struct Row {
    pub group: String,
    pub name: String,
    pub addr: String,
    pub ty: String,
    pub notes: String,
}

pub struct Db {
    pub rows: Vec<Row>,
    pub groups: Vec<String>,
    pub natives: usize,
    pub functions: usize,
    pub classes: usize,
}

/// One CSV line into fields ("" escapes inside quotes).
fn csv_fields(line: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut cur = String::new();
    let mut quoted = false;
    let mut chars = line.chars().peekable();
    while let Some(c) = chars.next() {
        match (c, quoted) {
            ('"', true) if chars.peek() == Some(&'"') => {
                cur.push('"');
                chars.next();
            }
            ('"', _) => quoted = !quoted,
            (',', false) => out.push(std::mem::take(&mut cur)),
            _ => cur.push(c),
        }
    }
    out.push(cur);
    out
}

/// The dumper's output folder (next to the menu's, like the unpacked exe).
fn dumper_dir() -> Option<PathBuf> {
    let starts = [std::env::current_exe().ok(), std::env::current_dir().ok()];
    starts.into_iter().flatten().find_map(|s| s.ancestors().map(|d| d.join("bullydumper")).find(|d| d.join("readable_offsets.csv").exists()))
}

impl Db {
    pub fn load() -> Result<Db, String> {
        let dir = dumper_dir().ok_or("bullydumper's readable_offsets.csv not found (run bullydumper with an output file)")?;
        let text = std::fs::read_to_string(dir.join("readable_offsets.csv")).map_err(|e| e.to_string())?;
        let mut rows = Vec::new();
        let mut groups: Vec<String> = Vec::new();
        for line in text.lines().skip(1) {
            let f = csv_fields(line);
            if f.len() < 7 {
                continue;
            }
            if groups.last() != Some(&f[0]) && !groups.contains(&f[0]) {
                groups.push(f[0].clone());
            }
            rows.push(Row { group: f[0].clone(), name: f[1].clone(), addr: f[2].clone(), ty: f[4].clone(), notes: f[6].clone() });
        }
        let count = |name: &str| std::fs::read_to_string(dir.join(name)).map(|t| t.lines().count().saturating_sub(1)).unwrap_or(0);
        let classes = groups.iter().filter(|g| g.starts_with("class ")).count();
        Ok(Db { rows, groups, natives: count("lua_natives.csv"), functions: count("functions.csv"), classes })
    }

    /// Rows matching the words of `filter` (all of them, any field) in `group`.
    pub fn query(&self, filter: &str, group: &str) -> Vec<usize> {
        let words: Vec<String> = filter.split_whitespace().map(|w| w.to_ascii_lowercase()).collect();
        self.rows
            .iter()
            .enumerate()
            .filter(|(_, r)| group.is_empty() || r.group == group)
            .filter(|(_, r)| {
                words.iter().all(|w| {
                    [&r.name, &r.addr, &r.notes, &r.group, &r.ty].iter().any(|f| f.to_ascii_lowercase().contains(w.as_str()))
                })
            })
            .map(|(i, _)| i)
            .collect()
    }
}

// ---- reading values ----------------------------------------------------------------

/// Objects the struct-relative addresses are read from.
pub struct Bases {
    ped: Option<u32>,
    vehicle: Option<u32>,
}

impl Bases {
    pub fn of(g: &Game) -> Bases {
        let ped = g.u32(PLAYER).filter(|&p| p != 0);
        let vehicle = ped.and_then(|p| g.u32(p + 0x1554)).filter(|&v| v != 0);
        Bases { ped, vehicle }
    }

    fn named(&self, name: &str) -> Option<u32> {
        match name {
            "ped" | "CPed" | "CPlayerPed" | "CPhysical" | "CEntity" | "CPlaceable" => self.ped,
            "vehicle" | "CVehicle" | "CAutomobile" | "CBike" => self.vehicle,
            _ => None,
        }
    }
}

/// An address expression: terms joined by `+`; a term is `0x..`, a name, or `[expr]`.
fn eval(g: &Game, bases: &Bases, s: &str) -> Option<u32> {
    let s = s.trim();
    let mut total: u32 = 0;
    let mut depth = 0;
    let mut start = 0;
    let bytes = s.as_bytes();
    let mut terms = Vec::new();
    for (i, &c) in bytes.iter().enumerate() {
        match c {
            b'[' => depth += 1,
            b']' => depth -= 1,
            b'+' if depth == 0 => {
                terms.push(&s[start..i]);
                start = i + 1;
            }
            _ => {}
        }
    }
    terms.push(&s[start..]);
    for t in terms {
        let t = t.trim();
        let v = if let Some(inner) = t.strip_prefix('[').and_then(|x| x.strip_suffix(']')) {
            g.u32(eval(g, bases, inner)?).filter(|&p| p != 0)?
        } else if let Some(hex) = t.strip_prefix("0x") {
            u32::from_str_radix(hex, 16).ok()?
        } else {
            bases.named(t)?
        };
        total = total.wrapping_add(v);
    }
    Some(total)
}

fn read_as(g: &Game, addr: u32, ty: &str) -> String {
    let base = ty.split(['[', ' ']).next().unwrap_or("");
    let count = if let Some(n) = ty.split('[').nth(1).and_then(|x| x.trim_end_matches(']').parse::<u32>().ok()) {
        n
    } else if ty.ends_with("[]") {
        4
    } else {
        1
    };
    let size = match base {
        "byte" => 1,
        "int16" => 2,
        "double" => 8,
        _ => 4,
    };
    let mut parts = Vec::new();
    for i in 0..count.min(8) {
        let a = addr + i * size;
        let mut b = [0u8; 8];
        if !g.read_bytes(a, &mut b[..size as usize]) {
            return "unreadable".into();
        }
        let s = match base {
            "float" => {
                let v = f32::from_le_bytes(b[..4].try_into().unwrap());
                if v.abs() >= 1e7 || (v != 0.0 && v.abs() < 1e-4) { format!("{v:e}") } else { format!("{v:.3}") }
            }
            "double" => format!("{:.3}", f64::from_le_bytes(b)),
            "byte" => {
                if ty.contains("bool") { (if b[0] != 0 { "true" } else { "false" }).into() } else { b[0].to_string() }
            }
            "int16" => i16::from_le_bytes([b[0], b[1]]).to_string(),
            "int32" => {
                let v = i32::from_le_bytes(b[..4].try_into().unwrap());
                if ty.contains("bool") { (if v != 0 { "true" } else { "false" }).into() } else { v.to_string() }
            }
            _ => format!("0x{:08X}", u32::from_le_bytes(b[..4].try_into().unwrap())),
        };
        parts.push(s);
    }
    let more = if ty.ends_with("[]") { ", …" } else { "" };
    if parts.len() == 1 { parts.remove(0) } else { format!("{}{more}", parts.join(", ")) }
}

/// The live value of a row, or why there isn't one.
pub fn value(g: &Game, bases: &Bases, row: &Row) -> String {
    if row.ty == "function" {
        return "code".into();
    }
    match eval(g, bases, &row.addr) {
        Some(a) => read_as(g, a, &row.ty),
        None if row.addr.contains('[') => "null pointer".into(),
        None => "needs an instance".into(),
    }
}

// ---- the dashboard ----------------------------------------------------------------

fn js(s: &str) -> String {
    crate::js(s)
}

fn position(g: &Game, ped: u32) -> Option<[f32; 3]> {
    let m = g.u32(ped + 0x14).unwrap_or(0);
    let at = if m != 0 { m + 0x30 } else { ped + 4 };
    Some([g.f32(at)?, g.f32(at + 4)?, g.f32(at + 8)?])
}

/// Live objects in a pool: (object address) for every used slot.
fn pool_objects(g: &Game, pool_ptr: u32) -> (Vec<u32>, u32) {
    let Some(pool) = g.u32(pool_ptr).filter(|&p| p != 0) else { return (Vec::new(), 0) };
    let (objects, map, cap, stride) = (g.u32(pool).unwrap_or(0), g.u32(pool + 4).unwrap_or(0), g.u32(pool + 8).unwrap_or(0), g.u32(pool + 0xC).unwrap_or(0));
    if objects == 0 || map == 0 || cap == 0 || cap > 4096 || stride == 0 {
        return (Vec::new(), cap);
    }
    let mut flags = vec![0u8; cap as usize];
    if !g.read_bytes(map, &mut flags) {
        return (Vec::new(), cap);
    }
    ((0..cap).filter(|&i| flags[i as usize] & 0x80 == 0).map(|i| objects + i * stride).collect(), cap)
}

/// Everything the cards show, as a JS object.
pub fn dashboard(g: &Game) -> String {
    let b = |a: u32| {
        let mut x = [0u8; 1];
        g.read_bytes(a, &mut x).then_some(x[0])
    };
    let ped = g.u32(PLAYER).filter(|&p| p != 0);
    let mut o = Vec::new();
    if let Some(p) = ped {
        let pos = position(g, p).unwrap_or([0.0; 3]);
        o.push(format!("pos:[{:.2},{:.2},{:.2}]", pos[0], pos[1], pos[2]));
        o.push(format!("health:{:.1}", g.f32(p + 0x1CB8).unwrap_or(0.0)));
        o.push(format!("maxHealth:{:.1}", g.f32(p + 0x1CA4).unwrap_or(0.0)));
        o.push(format!("money:{}", g.u32(p + 0x1CA0).unwrap_or(0) as i32));
        o.push(format!("trouble:{}", g.u32(p + 0x1D40).unwrap_or(0) as i32));
        o.push(format!("heading:{:.1}", g.f32(p + 0x1700).unwrap_or(0.0).to_degrees()));
        o.push(format!("busted:{}", g.u32(p + 0x1ED4).unwrap_or(0) as i32));
        let veh = g.u32(p + 0x1554).unwrap_or(0);
        o.push(format!("vehicle:{}", if veh != 0 { js(&format!("model {} · {:.0} hp", g.u32(veh + 0x10C).map(|v| (v >> 16) as u16).unwrap_or(0), g.f32(veh + 0x340).unwrap_or(0.0))) } else { js("on foot") }));
        // the nearest peds and vehicles
        let (peds, ped_cap) = pool_objects(g, PED_POOL);
        let dist = |q: [f32; 3]| ((q[0] - pos[0]).powi(2) + (q[1] - pos[1]).powi(2) + (q[2] - pos[2]).powi(2)).sqrt();
        let mut near: Vec<(f32, String)> = peds
            .iter()
            .filter(|&&q| q != p)
            .filter_map(|&q| {
                let at = position(g, q)?;
                let model = g.u32(q + 0x10C).map(|v| (v >> 16) as u16).unwrap_or(0);
                let faction = g.u32(q + 0x1310).unwrap_or(0) as i32;
                let health = g.f32(q + 0x1CB8).unwrap_or(0.0);
                let d = dist(at);
                Some((d, format!("{{model:{model},faction:{faction},health:{health:.0},dist:{d:.1}}}")))
            })
            .collect();
        near.sort_by(|a, b| a.0.total_cmp(&b.0));
        o.push(format!("peds:[{}]", near.iter().take(8).map(|x| x.1.clone()).collect::<Vec<_>>().join(",")));
        o.push(format!("pedCount:{},pedCap:{ped_cap}", peds.len()));
        let (vehs, veh_cap) = pool_objects(g, VEHICLE_POOL);
        let mut nearv: Vec<(f32, String)> = vehs
            .iter()
            .filter_map(|&v| {
                let at = position(g, v)?;
                let model = g.u32(v + 0x10C).map(|x| (x >> 16) as u16).unwrap_or(0);
                let d = dist(at);
                Some((d, format!("{{model:{model},health:{:.0},dist:{d:.1}}}", g.f32(v + 0x340).unwrap_or(0.0))))
            })
            .collect();
        nearv.sort_by(|a, b| a.0.total_cmp(&b.0));
        o.push(format!("vehicles:[{}]", nearv.iter().take(5).map(|x| x.1.clone()).collect::<Vec<_>>().join(",")));
        o.push(format!("vehCount:{},vehCap:{veh_cap}", vehs.len()));
    }
    o.push(format!("hour:{},minute:{}", b(CLOCK_HOUR).unwrap_or(0), b(CLOCK_MINUTE).unwrap_or(0)));
    o.push(format!("clockPaused:{}", b(CLOCK_PAUSED).unwrap_or(0) != 0));
    o.push(format!("chapter:{}", b(CHAPTER).unwrap_or(0)));
    o.push(format!("area:{}", g.u32(AREA).unwrap_or(0) as i32));
    o.push(format!("inGame:{}", ped.is_some()));
    format!("{{{}}}", o.join(","))
}
