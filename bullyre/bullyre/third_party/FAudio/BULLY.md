# FAudio, for Bully's XACT

The game plays its sound through XACT 3.1. It CoCreateInstances the engine (0x404F60, class 962f5027-99be-4692-a468-85802cf8de61), and its content is version 44 / tool 42 (ALRMCLK.xgs) and 43 (the sound banks inside the .sea files). XACT has no 64-bit runtime.

FAudio's FACT is XACT 3's engine reimplemented, and it reads content version 44. This copy is FAudio's source at the commit in `VERSION`:
- `include/` and `src/` as they are, minus the SDL platforms and XNA_Song;
- built with the native Win32 platform (FAUDIO_WIN32_PLATFORM: WASAPI);
- no WMA;
- one edit, in `include/FAudio.h`: with FAUDIO_STATIC defined (we are), FAUDIOAPI exports nothing. Otherwise bullyre.dll would export FAudio, and keep XNA_Song, whose Media Foundation imports aren't on Windows N editions.
- a second edit, in `src/FACT.c` (FACTCue_GetProperties, 2026-10-30): a track's duration is 0
  when its wave's sample rate is 0, instead of an integer divide by zero. The game crashed
  there in Sounds_Update on the first all-files run (a playing cue's wave with an empty
  format: see "Open" below).

The license is zlib (`LICENSE`).

`src/game/XactShim.cpp` puts XACT 3's interfaces over it: IXACT3Engine, IXACT3SoundBank, IXACT3WaveBank and IXACT3Cue, in their vtable order, each method calling the FACT function of the same name. The game's audio code (MDXact*.cpp, and the game's own while it runs) calls them as it called XACT's. Ours of 0x404F60 (XACTCreateEngine) makes one.

The shim adds four things:
- **Notifications** report our objects, not FACT's: the game matches the cue in a cue-destroyed notification (+0xF) against its own.
- **File reading:** XACT's default file reading (Win32 ReadFile and GetOverlappedResult on the game's handles) where the game leaves the callbacks empty. FACT's default would take the handle for its own stream type.
- **Cue properties** are copied into CoTaskMemAlloc's memory, since the game frees them with CoTaskMemFree.
- **Waves** (PrepareWave, IXACT3Wave): not implemented. The game doesn't use them.

## Checked

- `bullyre_xacttest.exe` uses FACT directly. ALRMCLK.xgs starts the engine, and all 322 .sea files' sound and wave banks load (11,075 cues). One cue in each is prepared.
- `bullyre_xactshimtest.exe` drives the shim through XACT's vtable slots with the game's runtime parameters and notification registration. Every bank loads. In each, a cue is found by name, prepared, has its state, properties and a variable read or set, and is stopped and destroyed. Every destroy brings the notification naming that cue, which is ours.

Nothing is played by either test.
- In the game (2026-10-30): with every game source file switched to ours and the engine
  ours of 0x404F60, the four-spot teleport trial (tests/trial.py) ran clean with sound
  through FAudio, once FACTCue_GetProperties was guarded (above).

## Open

- Why FACTCue_GetProperties met a playing wave with a zero sample rate (2026-10-30): a
  streaming bank's entry format, or a wave freed under the cue. The guard above stops the
  crash; the cause is still to be found (play a cue in bullyre_xactshimtest and read its
  properties while it plays).
