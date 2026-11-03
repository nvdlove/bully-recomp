//! Intercom: your mic, made to sound like the boys' dorm PA, on a push-to-talk key.
//!
//! The game's PA announcements have the effect recorded in (their energy sits
//! almost entirely between 300 and 3,400 Hz, peaking around 600-1,200 Hz), so the
//! effect is rebuilt here to match: band-pass, a lift around 1 kHz, a little
//! speaker drive and a short hallway echo, plus an optional chime (the game's own,
//! see `load_chime`). Audio
//! runs on its own thread through waveIn / waveOut at 22,050 Hz (the rate of the
//! game's own speech), in 20 ms blocks. The filter was tuned by running a clean
//! game voice line through it and matching the band energy of a real PA line.

use std::f32::consts::PI;
use std::path::Path;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::thread::JoinHandle;

use crate::win::*;

const RATE: u32 = 22_050;
const BLOCK: usize = 441; // 20 ms
const IN_BUFFERS: usize = 6;
const OUT_BUFFERS: usize = 10;

#[derive(Clone, PartialEq, Debug)]
pub struct Config {
    pub enabled: bool,
    pub vk: i32, // push-to-talk virtual key (0 = none)
    pub key_label: String,
    pub dorm_only: bool,
    pub mic: i32, // waveIn device, -1 = system default
    pub volume: f32,
    pub room: f32, // hallway echo, 0..1
    pub chime: bool,
}

impl Default for Config {
    fn default() -> Config {
        Config { enabled: false, vk: 0x56, key_label: "V".into(), dorm_only: true, mic: -1, volume: 1.0, room: 0.35, chime: true }
    }
}

impl Config {
    pub fn set(&mut self, k: &str, v: &str) -> bool {
        let f = |d: f32| v.parse::<f32>().ok().filter(|x| x.is_finite()).unwrap_or(d);
        match k {
            "ic_on" => self.enabled = v == "1",
            "ic_vk" => self.vk = v.parse().unwrap_or(self.vk),
            "ic_key" => self.key_label = v.chars().filter(|c| *c != '|' && *c != '=').take(24).collect(),
            "ic_dorm" => self.dorm_only = v == "1",
            "ic_mic" => self.mic = v.parse().unwrap_or(-1),
            "ic_vol" => self.volume = f(1.0).clamp(0.0, 4.0),
            "ic_room" => self.room = f(0.35).clamp(0.0, 1.0),
            "ic_chime" => self.chime = v == "1",
            _ => return false,
        }
        true
    }

    pub fn to_kv(&self) -> String {
        format!(
            "ic_on={}|ic_vk={}|ic_key={}|ic_dorm={}|ic_mic={}|ic_vol={:.2}|ic_room={:.2}|ic_chime={}",
            self.enabled as u8, self.vk, self.key_label, self.dorm_only as u8, self.mic, self.volume, self.room, self.chime as u8
        )
    }
}

/// Microphones Windows knows about, in waveIn device order.
pub fn mics() -> Vec<String> {
    let n = unsafe { waveInGetNumDevs() };
    (0..n)
        .map(|i| {
            let mut caps: WAVEINCAPSW = unsafe { std::mem::zeroed() };
            unsafe { waveInGetDevCapsW(i as usize, &mut caps, std::mem::size_of::<WAVEINCAPSW>() as u32) };
            let len = caps.szPname.iter().position(|c| *c == 0).unwrap_or(32);
            String::from_utf16_lossy(&caps.szPname[..len])
        })
        .collect()
}

// ---- DSP -------------------------------------------------------------------

/// RBJ-cookbook biquad.
#[derive(Clone, Copy, Default)]
struct Biquad {
    b: [f32; 3],
    a: [f32; 2],
    z: [f32; 2],
}

impl Biquad {
    fn make(b0: f32, b1: f32, b2: f32, a0: f32, a1: f32, a2: f32) -> Biquad {
        Biquad { b: [b0 / a0, b1 / a0, b2 / a0], a: [a1 / a0, a2 / a0], z: [0.0; 2] }
    }
    fn highpass(f: f32, q: f32) -> Biquad {
        let w = 2.0 * PI * f / RATE as f32;
        let (c, al) = (w.cos(), w.sin() / (2.0 * q));
        Biquad::make((1.0 + c) / 2.0, -(1.0 + c), (1.0 + c) / 2.0, 1.0 + al, -2.0 * c, 1.0 - al)
    }
    fn lowpass(f: f32, q: f32) -> Biquad {
        let w = 2.0 * PI * f / RATE as f32;
        let (c, al) = (w.cos(), w.sin() / (2.0 * q));
        Biquad::make((1.0 - c) / 2.0, 1.0 - c, (1.0 - c) / 2.0, 1.0 + al, -2.0 * c, 1.0 - al)
    }
    fn peak(f: f32, q: f32, db: f32) -> Biquad {
        let w = 2.0 * PI * f / RATE as f32;
        let (c, al, a) = (w.cos(), w.sin() / (2.0 * q), 10f32.powf(db / 40.0));
        Biquad::make(1.0 + al * a, -2.0 * c, 1.0 - al * a, 1.0 + al / a, -2.0 * c, 1.0 - al / a)
    }
    fn run(&mut self, x: f32) -> f32 {
        let y = self.b[0] * x + self.z[0];
        self.z[0] = self.b[1] * x - self.a[0] * y + self.z[1];
        self.z[1] = self.b[2] * x - self.a[1] * y;
        y
    }
}

/// Feedback comb for the echo.
struct Comb {
    buf: Vec<f32>,
    pos: usize,
    feedback: f32,
    damp: f32,
    last: f32,
}

impl Comb {
    fn new(ms: f32, feedback: f32) -> Comb {
        Comb { buf: vec![0.0; (ms / 1000.0 * RATE as f32) as usize], pos: 0, feedback, damp: 0.35, last: 0.0 }
    }
    fn run(&mut self, x: f32) -> f32 {
        let out = self.buf[self.pos];
        self.last = out * (1.0 - self.damp) + self.last * self.damp;
        self.buf[self.pos] = x + self.last * self.feedback;
        self.pos = (self.pos + 1) % self.buf.len();
        out
    }
}

struct Dsp {
    eq: [Biquad; 5],
    combs: [Comb; 3],
    env: f32,         // push-to-talk gate, 0..1
    tail: usize,      // samples left to play out after the key is let go
    chime: Vec<f32>,
    chime_at: Option<usize>,
    was_down: bool,
}

impl Dsp {
    fn new(chime: Vec<f32>) -> Dsp {
        Dsp {
            eq: [
                Biquad::highpass(420.0, 0.8),
                Biquad::highpass(420.0, 0.8),
                Biquad::peak(900.0, 0.8, 7.0),
                Biquad::lowpass(3000.0, 0.8),
                Biquad::lowpass(3000.0, 0.8),
            ],
            combs: [Comb::new(71.0, 0.45), Comb::new(113.0, 0.4), Comb::new(157.0, 0.35)],
            env: 0.0,
            tail: 0,
            chime,
            chime_at: None,
            was_down: false,
        }
    }

    /// Returns the block to play, or None when there's nothing to hear.
    fn process(&mut self, input: &[i16], down: bool, cfg: &Config) -> Option<Vec<i16>> {
        if down && !self.was_down && cfg.chime && !self.chime.is_empty() {
            self.chime_at = Some(0);
        }
        self.was_down = down;
        let active = down || self.env > 0.001 || self.tail > 0 || self.chime_at.is_some();
        let mut out = Vec::with_capacity(input.len());
        for &x in input {
            let target = if down { 1.0 } else { 0.0 };
            let rate = if target > self.env { 0.004 } else { 0.0006 }; // ~10 ms attack, ~80 ms release
            self.env += (target - self.env) * rate * 10.0;
            let mut v = x as f32 / 32768.0 * 2.5 * self.env; // mics are quiet; make up some level
            for f in self.eq.iter_mut() {
                v = f.run(v);
            }
            v = (v * 1.8).tanh() / 1.8f32.tanh(); // small speaker's grit
            // the chime is recorded through the PA already
            if let Some(t) = self.chime_at {
                v += self.chime[t];
                self.chime_at = (t + 1 < self.chime.len()).then_some(t + 1);
            }
            let wet: f32 = self.combs.iter_mut().map(|c| c.run(v)).sum::<f32>() / 3.0;
            let y = (v + cfg.room * wet) * cfg.volume;
            out.push((y.clamp(-1.0, 1.0) * 32767.0) as i16);
        }
        if down {
            self.tail = RATE as usize; // let the echo ring out for a second
        } else {
            self.tail = self.tail.saturating_sub(input.len());
        }
        active.then_some(out)
    }
}

// ---- audio thread ----------------------------------------------------------

struct Shared {
    cfg: Mutex<Config>,
    allowed: AtomicBool, // e.g. "in the boys' dorm"
    talking: AtomicBool,
    stop: AtomicBool,
    error: Mutex<String>,
}

pub struct Intercom {
    shared: Arc<Shared>,
    thread: Option<JoinHandle<()>>,
    mic: i32,
}

impl Default for Intercom {
    fn default() -> Intercom {
        Intercom {
            shared: Arc::new(Shared {
                cfg: Mutex::new(Config::default()),
                allowed: AtomicBool::new(false),
                talking: AtomicBool::new(false),
                stop: AtomicBool::new(false),
                error: Mutex::new(String::new()),
            }),
            thread: None,
            mic: -1,
        }
    }
}

impl Intercom {
    /// Applies settings: starts, stops or restarts the audio thread as needed.
    pub fn configure(&mut self, cfg: &Config) {
        *self.shared.cfg.lock().unwrap() = cfg.clone();
        let running = self.thread.as_ref().is_some_and(|t| !t.is_finished());
        if !cfg.enabled || (running && cfg.mic != self.mic) {
            self.stop();
        }
        if cfg.enabled && self.thread.as_ref().is_none_or(|t| t.is_finished()) {
            self.shared.stop.store(false, Ordering::SeqCst);
            self.shared.error.lock().unwrap().clear();
            self.mic = cfg.mic;
            let shared = self.shared.clone();
            self.thread = Some(std::thread::spawn(move || {
                if let Err(e) = run(&shared) {
                    *shared.error.lock().unwrap() = e;
                }
                shared.talking.store(false, Ordering::SeqCst);
            }));
        }
    }

    pub fn set_allowed(&self, allowed: bool) {
        self.shared.allowed.store(allowed, Ordering::SeqCst);
    }

    pub fn talking(&self) -> bool {
        self.shared.talking.load(Ordering::SeqCst)
    }

    pub fn error(&self) -> String {
        self.shared.error.lock().unwrap().clone()
    }

    pub fn stop(&mut self) {
        self.shared.stop.store(true, Ordering::SeqCst);
        if let Some(t) = self.thread.take() {
            let _ = t.join();
        }
    }
}

impl Drop for Intercom {
    fn drop(&mut self) {
        self.stop();
    }
}

fn format() -> WAVEFORMATEX {
    WAVEFORMATEX { wFormatTag: 1, nChannels: 1, nSamplesPerSec: RATE, nAvgBytesPerSec: RATE * 2, nBlockAlign: 2, wBitsPerSample: 16, cbSize: 0 }
}

fn run(shared: &Shared) -> Result<(), String> {
    let mic = shared.cfg.lock().unwrap().mic;
    let fmt = format();
    let event = unsafe { CreateEventW(std::ptr::null_mut(), 0, 0, std::ptr::null()) };
    let mut hin: HANDLE = std::ptr::null_mut();
    let dev = if mic < 0 { WAVE_MAPPER } else { mic as u32 };
    let r = unsafe { waveInOpen(&mut hin, dev, &fmt, event as usize, 0, CALLBACK_EVENT) };
    if r != 0 {
        unsafe { CloseHandle(event) };
        return Err(format!("couldn't open the microphone (error {r})"));
    }
    let mut hout: HANDLE = std::ptr::null_mut();
    let r = unsafe { waveOutOpen(&mut hout, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL) };
    if r != 0 {
        unsafe {
            waveInClose(hin);
            CloseHandle(event);
        }
        return Err(format!("couldn't open the speakers (error {r})"));
    }
    const HDR: u32 = std::mem::size_of::<WAVEHDR>() as u32;
    // headers and sample memory stay put until both devices are closed
    let mut in_data: Vec<Vec<i16>> = (0..IN_BUFFERS).map(|_| vec![0i16; BLOCK]).collect();
    let mut in_hdr: Vec<WAVEHDR> = (0..IN_BUFFERS).map(|_| unsafe { std::mem::zeroed() }).collect();
    let mut out_data: Vec<Vec<i16>> = (0..OUT_BUFFERS).map(|_| vec![0i16; BLOCK]).collect();
    let mut out_hdr: Vec<WAVEHDR> = (0..OUT_BUFFERS).map(|_| unsafe { std::mem::zeroed() }).collect();
    unsafe {
        for (h, d) in in_hdr.iter_mut().zip(in_data.iter_mut()) {
            h.lpData = d.as_mut_ptr() as *mut u8;
            h.dwBufferLength = (BLOCK * 2) as u32;
            waveInPrepareHeader(hin, h, HDR);
            waveInAddBuffer(hin, h, HDR);
        }
        for (h, d) in out_hdr.iter_mut().zip(out_data.iter_mut()) {
            h.lpData = d.as_mut_ptr() as *mut u8;
            h.dwBufferLength = (BLOCK * 2) as u32;
            waveOutPrepareHeader(hout, h, HDR);
            h.dwFlags |= WHDR_DONE; // free to use
        }
        waveInStart(hin);
    }
    let chime = load_chime(&crate::game::game_dir()).unwrap_or_else(|e| {
        *shared.error.lock().unwrap() = format!("no chime ({e})");
        Vec::new()
    });
    let mut dsp = Dsp::new(chime);
    let mut next = 0;
    while !shared.stop.load(Ordering::SeqCst) {
        unsafe { WaitForSingleObject(event, 100) };
        // capture buffers come back in order
        while in_hdr[next].dwFlags & WHDR_DONE != 0 && !shared.stop.load(Ordering::SeqCst) {
            let cfg = shared.cfg.lock().unwrap().clone();
            let n = in_hdr[next].dwBytesRecorded as usize / 2;
            let down = cfg.enabled && cfg.vk != 0 && shared.allowed.load(Ordering::SeqCst) && unsafe { GetAsyncKeyState(cfg.vk) } as u16 & 0x8000 != 0;
            shared.talking.store(down, Ordering::SeqCst);
            if let Some(block) = dsp.process(&in_data[next][..n], down, &cfg)
                && let Some(slot) = out_hdr.iter().position(|h| h.dwFlags & WHDR_DONE != 0)
            {
                out_data[slot][..block.len()].copy_from_slice(&block);
                let h = &mut out_hdr[slot];
                h.dwBufferLength = (block.len() * 2) as u32;
                h.dwFlags &= !WHDR_DONE;
                unsafe { waveOutWrite(hout, h, HDR) };
            }
            let h = &mut in_hdr[next];
            h.dwFlags &= !WHDR_DONE;
            unsafe { waveInAddBuffer(hin, h, HDR) };
            next = (next + 1) % IN_BUFFERS;
        }
    }
    unsafe {
        waveInStop(hin);
        waveInReset(hin);
        waveOutReset(hout);
        for h in in_hdr.iter_mut() {
            waveInUnprepareHeader(hin, h, HDR);
        }
        for h in out_hdr.iter_mut() {
            waveOutUnprepareHeader(hout, h, HDR);
        }
        waveInClose(hin);
        waveOutClose(hout);
        CloseHandle(event);
    }
    drop((in_data, out_data));
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn config_round_trip() {
        let a = Config { enabled: true, vk: 5, key_label: "Mouse 4".into(), dorm_only: false, mic: 2, volume: 1.5, room: 0.1, chime: false };
        let mut b = Config::default();
        for kv in a.to_kv().split('|') {
            let (k, v) = kv.split_once('=').unwrap();
            assert!(b.set(k, v));
        }
        assert_eq!(a, b);
    }

    #[test]
    fn filter_is_a_band_pass() {
        // energy of a tone after the intercom EQ, relative to the input
        let gain = |freq: f32| {
            let mut d = Dsp::new(Vec::new());
            let cfg = Config { room: 0.0, chime: false, volume: 1.0, ..Default::default() };
            let tone: Vec<i16> = (0..RATE as usize).map(|i| ((2.0 * PI * freq * i as f32 / RATE as f32).sin() * 3000.0) as i16).collect();
            let out = d.process(&tone, true, &cfg).unwrap();
            let rms = |s: &[i16]| (s.iter().map(|&x| (x as f32).powi(2)).sum::<f32>() / s.len() as f32).sqrt();
            rms(&out[RATE as usize / 2..]) / rms(&tone[RATE as usize / 2..])
        };
        let (low, mid, high) = (gain(100.0), gain(1000.0), gain(8000.0));
        assert!(mid > 4.0 * low && mid > 4.0 * high, "low {low} mid {mid} high {high}");
    }

    #[test]
    fn pa_chime_from_the_game() {
        let dir = crate::game::game_dir();
        let dir = dir.as_path();
        if !dir.join(SPEECH_BIN).exists() {
            return; // game not installed here
        }
        let chime = load_chime(dir).unwrap();
        let secs = chime.len() as f32 / RATE as f32;
        let peak = chime.iter().fold(0f32, |m, x| m.max(x.abs()));
        assert!((0.7..0.75).contains(&secs) && (0.3..=1.0).contains(&peak), "{secs} s, peak {peak}");
    }
}

/// `--render-intercom in.wav out.wav`: runs a 16-bit mono WAV through the
/// intercom sound (key held, no chime), for comparing with the game's own PA lines.
pub fn render_file(input: &str, output: &str) -> Result<(), String> {
    let d = std::fs::read(input).map_err(|e| format!("{input}: {e}"))?;
    let data = d.windows(4).position(|w| w == b"data").ok_or("no data chunk")?;
    let pcm: Vec<i16> = d[data + 8..].as_chunks::<2>().0.iter().map(|c| i16::from_le_bytes(*c)).collect();
    let cfg = Config { enabled: true, chime: false, ..Config::default() };
    let mut dsp = Dsp::new(Vec::new());
    let mut out = Vec::new();
    for block in pcm.chunks(BLOCK) {
        out.extend(dsp.process(block, true, &cfg).unwrap_or_default());
    }
    let bytes: Vec<u8> = out.iter().flat_map(|s| s.to_le_bytes()).collect();
    let mut w = Vec::new();
    w.extend(b"RIFF");
    w.extend((36 + bytes.len() as u32).to_le_bytes());
    w.extend(b"WAVEfmt ");
    w.extend(16u32.to_le_bytes());
    w.extend([1u8, 0, 1, 0]);
    w.extend(RATE.to_le_bytes());
    w.extend((RATE * 2).to_le_bytes());
    w.extend([2u8, 0, 16, 0]);
    w.extend(b"data");
    w.extend((bytes.len() as u32).to_le_bytes());
    w.extend(bytes);
    std::fs::write(output, w).map_err(|e| format!("{output}: {e}"))
}

// ---- the PA chime ------------------------------------------------------------

/// The chime before Ms. Danvers' PA announcements. The game has it only as the start
/// of one of her lines, Danvers_PACHAPT2_v7, in audio\PLAYLIST\Speech.bin: 880 and
/// 600 Hz tones from 0.09 s, her voice from 0.825 s. Read from the game's own files.
const SPEECH_LIST: &str = r"audio\PLAYLIST\Speech.lst";
const SPEECH_BIN: &str = r"audio\PLAYLIST\Speech.bin";
const CHIME_LINE: &str = "Danvers_PACHAPT2_v7.rsm";
const CHIME_FROM: f32 = 0.09;
const CHIME_TO: f32 = 0.815;

pub fn load_chime(game_dir: &Path) -> Result<Vec<f32>, String> {
    use std::io::{Read, Seek, SeekFrom};
    let list = std::fs::read_to_string(game_dir.join(SPEECH_LIST)).map_err(|e| format!("{SPEECH_LIST}: {e}"))?;
    let index = list.lines().position(|l| l.trim() == CHIME_LINE).ok_or(format!("{CHIME_LINE} isn't in Speech.lst"))?;
    // Speech.bin: "Hash", count, ?, then per line (in Speech.lst order) offset, size,
    // name hash; each line is a small sound bank with its own wave bank.
    let mut f = std::fs::File::open(game_dir.join(SPEECH_BIN)).map_err(|e| format!("{SPEECH_BIN}: {e}"))?;
    let mut entry = [0u8; 8];
    f.seek(SeekFrom::Start(12 + 12 * index as u64))
        .and_then(|_| f.read_exact(&mut entry))
        .map_err(|e| format!("{SPEECH_BIN}: {e}"))?;
    let (off, size) = (u32::from_le_bytes(entry[..4].try_into().unwrap()), u32::from_le_bytes(entry[4..].try_into().unwrap()));
    let mut bank = vec![0u8; size as usize];
    f.seek(SeekFrom::Start(off as u64)).and_then(|_| f.read_exact(&mut bank)).map_err(|e| format!("{SPEECH_BIN}: {e}"))?;
    let (line, _) = bank_wave(&bank, 0).ok_or(format!("can't read {CHIME_LINE}"))?;
    let at = |s: f32| ((s * RATE as f32) as usize).min(line.len());
    let mut out = line[at(CHIME_FROM)..at(CHIME_TO)].to_vec();
    // fade the last 15 ms, where her first word would start
    let fade = (RATE as usize * 15 / 1000).min(out.len());
    let n = out.len();
    for k in 0..fade {
        out[n - fade + k] *= 1.0 - k as f32 / fade as f32;
    }
    Ok(out)
}

/// One mono MS-ADPCM wave of the XACT wave bank ("WBND") inside a sound bank,
/// resampled to `RATE`, and where its loop ends.
fn bank_wave(sea: &[u8], i: usize) -> Option<(Vec<f32>, usize)> {
    let b = &sea[sea.windows(4).position(|w| w == b"WBND")?..];
    let u = |o: usize| b.get(o..o + 4).map(|x| u32::from_le_bytes(x.try_into().unwrap()) as usize);
    // signature, 2 versions, then 5 segments (offset, length): bank data, entries, -, -, wave data
    let (data, entries, waves) = (u(12)?, u(20)?, u(44)?);
    // bank data: flags, entry count, name[64], entry size
    if i >= u(data + 4)? {
        return None;
    }
    let e = entries + i * u(data + 72)?;
    // entry: flags/duration, format, play offset, play length, loop start, loop length
    let (fmt, off, len, loop_start, loop_len) = (u(e + 4)?, u(e + 8)?, u(e + 12)?, u(e + 16)?, u(e + 20)?);
    // format: tag:2 (2 = ADPCM), channels:3, rate:18, block align:8 (bytes - 22)
    let (tag, channels, rate, align) = (fmt & 3, (fmt >> 2) & 7, (fmt >> 5) & 0x3FFFF, (fmt >> 23) & 0xFF);
    if tag != 2 || channels != 1 || rate == 0 {
        return None;
    }
    // the speech lines' play length can run past the end of their bank
    let raw = b.get(waves + off..)?;
    let pcm: Vec<f32> = raw[..len.min(raw.len())].chunks_exact(align + 22).flat_map(adpcm_block).collect();
    let step = rate as f32 / RATE as f32;
    let out = (0..(pcm.len() as f32 / step) as usize)
        .map(|k| {
            let x = k as f32 * step;
            let j = x as usize;
            pcm[j] + (pcm.get(j + 1).copied().unwrap_or(0.0) - pcm[j]) * x.fract()
        })
        .collect();
    Some((out, ((loop_start + loop_len) as f32 / step) as usize))
}

/// Microsoft ADPCM, one mono block: predictor, delta, two starting samples, then nibbles.
fn adpcm_block(b: &[u8]) -> Vec<f32> {
    const COEF: [(i32, i32); 7] = [(256, 0), (512, -256), (0, 0), (192, 64), (240, 0), (460, -208), (392, -232)];
    const ADAPT: [i32; 16] = [230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230];
    let (c1, c2) = COEF[b[0] as usize % 7];
    let s = |o: usize| i16::from_le_bytes([b[o], b[o + 1]]) as i32;
    let (mut delta, mut s1, mut s2) = (s(1), s(3), s(5));
    let mut out = vec![s2, s1];
    for n in b[7..].iter().flat_map(|&x| [x >> 4, x & 15]) {
        let signed = if n & 8 != 0 { n as i32 - 16 } else { n as i32 };
        let p = (((s1 * c1 + s2 * c2) >> 8) + signed * delta).clamp(-32768, 32767);
        out.push(p);
        (s2, s1) = (s1, p);
        delta = (ADAPT[n as usize] * delta >> 8).max(16);
    }
    out.into_iter().map(|x| x as f32 / 32768.0).collect()
}
