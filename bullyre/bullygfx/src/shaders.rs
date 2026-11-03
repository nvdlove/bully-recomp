//! Collecting the game's shaders: every vertex and pixel shader it creates is saved
//! as its Direct3D 9 bytecode to `bullygfx_shaders\vs_<hash>.bin` / `ps_<hash>.bin`
//! next to the game, for the translator to be built and checked against offline.

use std::collections::HashSet;
use std::sync::Mutex;

use crate::win::read_bytes;

pub const DEV_CREATE_VS: u32 = 91;
pub const DEV_CREATE_PS: u32 = 106;

/// A sane upper bound on a shader's length, in tokens.
const MAX_TOKENS: usize = 1 << 16;

static SEEN: Mutex<Option<HashSet<u64>>> = Mutex::new(None);

/// The bytecode at `code`, up to and including the end token. Comment blocks (the
/// constant table among them) are skipped by their length: their contents can hold
/// anything, the end token's value included.
pub fn bytecode(code: u32) -> Option<Vec<u32>> {
    let token = |i: usize| -> Option<u32> {
        let mut b = [0u8; 4];
        read_bytes(code + 4 * i as u32, &mut b).then(|| u32::from_le_bytes(b))
    };
    let mut out = vec![token(0)?]; // version
    while out.len() < MAX_TOKENS {
        let t = token(out.len())?;
        out.push(t);
        match t & 0xFFFF {
            0xFFFF => return Some(out),
            0xFFFE => {
                for _ in 0..(t >> 16) & 0x7FFF {
                    let c = token(out.len())?;
                    out.push(c);
                }
            }
            _ => {}
        }
    }
    None
}

pub fn hash(tokens: &[u32]) -> u64 {
    tokens.iter().fold(0xcbf2_9ce4_8422_2325u64, |h, &t| (h ^ t as u64).wrapping_mul(0x100_0000_01b3))
}

/// CreateVertexShader / CreatePixelShader: `code` is the bytecode pointer.
pub fn created(method: u32, code: u32) {
    let Some(tokens) = bytecode(code) else { return };
    let h = hash(&tokens);
    {
        let Ok(mut seen) = SEEN.lock() else { return };
        if !seen.get_or_insert_with(HashSet::new).insert(h) {
            return;
        }
    }
    let dir = crate::win::game_file("bullygfx_shaders");
    let _ = std::fs::create_dir_all(&dir);
    let kind = if method == DEV_CREATE_VS { "vs" } else { "ps" };
    let bytes: Vec<u8> = tokens.iter().flat_map(|t| t.to_le_bytes()).collect();
    let _ = std::fs::write(dir.join(format!("{kind}_{h:016x}.bin")), bytes);
}
