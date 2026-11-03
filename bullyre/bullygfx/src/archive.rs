//! The game's streaming archive: `Stream/World.img`, with its directory `World.dir` of
//! 32-byte entries (offset and size in 2048-byte sectors, then a 24-byte name). Read-only,
//! for tools and tests; the running game's archive goes through `modfs`.
#![allow(dead_code)]

use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::path::Path;

pub const SECTOR: u64 = 2048;

pub struct Entry {
    pub name: String,
    /// bytes from the start of World.img
    pub offset: u64,
    /// whole sectors: the file plus its padding
    pub size: u64,
}

pub struct Archive {
    file: File,
    pub entries: Vec<Entry>,
}

impl Archive {
    /// `stream` is the game's `Stream` folder.
    pub fn open(stream: &Path) -> std::io::Result<Archive> {
        let dir = std::fs::read(stream.join("World.dir"))?;
        let entries = dir
            .chunks_exact(32)
            .map(|e| {
                let word = |i: usize| u32::from_le_bytes([e[i], e[i + 1], e[i + 2], e[i + 3]]) as u64;
                let name = e[8..32].split(|&c| c == 0).next().unwrap_or_default();
                Entry { name: String::from_utf8_lossy(name).into_owned(), offset: word(0) * SECTOR, size: word(4) * SECTOR }
            })
            .collect();
        Ok(Archive { file: File::open(stream.join("World.img"))?, entries })
    }

    pub fn find(&self, name: &str) -> Option<&Entry> {
        self.entries.iter().find(|e| e.name.eq_ignore_ascii_case(name))
    }

    /// An entry's bytes, its sector padding included.
    pub fn read(&self, entry: &Entry) -> std::io::Result<Vec<u8>> {
        let mut file = &self.file;
        file.seek(SeekFrom::Start(entry.offset))?;
        let mut data = vec![0; entry.size as usize];
        file.read_exact(&mut data)?;
        Ok(data)
    }
}
