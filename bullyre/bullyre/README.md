# bullyre

Bully: Scholarship Edition's game code, rewritten as source, one function at a time,
towards a native 64-bit build. It's a fan project, not affiliated with Rockstar
Games, and it needs your own copy of the game: none of its assets are included.

## How it works

The original 32-bit game keeps running. `bullyre.dll` is loaded by the game's very
first instruction (bullymenu patches the entry point of the suspended process to
`LoadLibraryA("bullyre.dll")` through the game's own import) and replaces game
functions with ours, each in one of three modes (`bullyre.ini` next to the game,
`Name = mode`):

| mode | what runs |
|---|---|
| `original` | the game's code; ours isn't installed |
| `shadow` | both on every call, results compared (mismatches in `bullyre_log.txt`); the game keeps the original's result |
| `ours` | our code instead of the game's |

A new function starts in `shadow` when it can (no side effects the game would see
twice), goes to `ours` once shadowing is clean over real play, and is then tested in play
(parity screenshots against the original, and long soak runs with mods).

Registering one, next to its code:

```cpp
REVERSED("CStreaming::GetInfoIndex", 0x0052DC90, &GetInfoIndex, &Shadow_GetInfoIndex,
         &Original_GetInfoIndex, re::Mode::Shadow);
```

## Rules

- Game data is declared at its address with `GAME_VAR` / `GAME_ARRAY`. While the
  original runs, our structs must have the game's exact layout: every struct gets
  `static_assert`s on its size (and offsets where it matters).
- Names follow GTA San Andreas where Bully's code is the same (it mostly is for
  streaming, model info, pools, the renderer's scene lists); otherwise what the code does.
- Every function records its address and calling convention.
- Nothing may depend on 32-bit pointers beyond the layout asserts: that's what the
  64-bit build changes.

## Where things are

- `src/reversed.*` the registry and hooking (MinHook); `src/startup.cpp` what runs before
  the game's code, however bullyre got in
- `src/game/` the game's code, by class
- `src/host/` bullyre_host.exe: the game run from our own exe (below)
- `tools/progress.py` how far along we are (against bullydumper's `functions.csv`)

## The host exe (step 4: our code as its own program)

`bullyre_host.exe` runs the game without Bully.exe. It is linked at the game's base
(0x400000) with the game's whole image reserved at the start of its `.text`
(`src/host/reserve.asm`); its entry point maps `Bully_unpacked.exe` there before our C
runtime starts (sections, imports, protections, and our in-memory header edited so the
game's C runtime and resources see the game's sections), then installs our functions as
bullyre.dll would and calls the game's own entry point. The game's libraries (Gamebryo,
XACT, Lua, its C runtime) still run from that image until they are replaced.

```
bullyre_host.exe --check [--image <Bully_unpacked.exe>]   # map, install, verify, exit
```

writes `bullyre_host_check.txt` (every section as in the file, protections, the 200
imports, the header as the game's runtime reads it, the icon, an installed function).
bullymenu runs it with "Run from bullyre's own exe" on: it starts the host suspended,
waits for it to map the game (events `Local\bullyre_host_mapped_<pid>` / `_go_<pid>`),
writes its patches, then lets it run.

## Libraries from their source

The libraries linked into the game are replaced whole, from their source, configured as
the game built them, rather than reversed function by function:

- **Lua 5.0.2** (`third_party/lua-5.0.2`, 2026-10-22). Its `BULLY.md` lists the game's
  configuration (float numbers, 4 bytes of user state, the game's allocator, no parser)
  and the edits to the library tables. `src/game/LuaLibrary.cpp` registers it at the 93
  entry points the game's code and the natives reach. Our code calls it directly
  (`LIB_FN` in reversed.h, `src/game/Lua502.h`). `tools/lua_map.txt` pairs the rest of the
  game's copy with the source. `bullyre_luatest.exe` loads all 515 of the game's scripts
  with it.
- **libjpeg 6b** (`third_party/jpeg-6b`, 2026-10-22), the photo album's codec, in place of
  the reversed copy (the Jpeg*.cpp files an agent wrote). Its `BULLY.md` has the game's
  build options. `src/game/JpegLibrary.cpp` registers its 51 entry points;
  `src/game/JpegPhotoCodec.cpp` (game code) calls it directly. `tools/jpegparity.py`: the
  game's encoder and decoder against ours in unicorn, byte for byte the same.
- **The C runtime** (2026-10-23): `src/game/Crt.h` is the one place the game's code reaches
  it. Strings, memory, formatting, parsing, sorting and the maths come from ours (the 437
  declarations of the game's VC8 copy forward to `crt::`; the x87 bridges to its maths are
  gone). What keeps state the game's own code shares (its heap, its rand sequence, FILE,
  atexit/exit, C++ exceptions) stays the game's until the 64-bit step.
- **XACT → FAudio** (`third_party/FAudio`, 2026-10-23): the game's XACT 3.1 engine is
  replaced by FAudio's FACT behind XACT's own interfaces (`src/game/XactShim.cpp`, made by
  ours of XACTCreateEngine 0x404F60), so the game's audio code is unchanged. All 322 of the
  game's banks load through it (`bullyre_xacttest`, `bullyre_xactshimtest`).

## Building

```
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release
```

bullymenu copies the newest `build/Release/bullyre.dll` and `bullyre_host.exe` next to
the game when it launches it, and loads the dll when it's there (or runs the host).

## Roadmap

1. Streaming: the model table, requests, loading, removal. The core (14 functions,
   requests through loading and removal) runs as ours by default since 2026-09-29.
2. The texture dictionaries (CTxdStore: done, with its name tree), model info, the
   pools, the file loader, collision, the memory manager, CdStream: written
   2026-09-29 (565 functions registered), most of it still in `original` mode until
   each batch is switched on and checked (`python tools/registrations.py`).
   Second pass the same day (2,592 registered): entities and the world, peds and
   their objectives, vehicles and objects, weapons, cameras and input, the HUD and
   frontend, CGame and placement loading, and 961 of the 1,489 script natives.
   2026-10-01: ALL of the game's own code (0x401000..0x73AE00) written: 16,028
   functions registered (Ghidra's 15,150 plus ~900 starts it missed), builds clean,
   the game runs with them installed (52 ours, the rest `original`). Untested beyond
   that: the debugging phase switches them on batch by batch. Known issues to fix
   first: tools/review_notes.md; register hazards: register_risks.txt
   (tools/regcheck); emulator side-by-side tests: tests/emu.
   2026-10-30: switched on in batches and tested with tests/trial.py (four teleports):
   every game source file (15,908 functions), then the static initializers, the
   common-block allocator, Lua, libjpeg, and all of src/gamebryo: ALL 27,325 registered
   functions run as `ours` through the trial, 0 original, 0 shadow mismatches
   (tests/everything.txt; the staged sets are tests/all_files_1030*.txt,
   all_plus_*.txt). Fixed on the way: FAudio's FACTCue_GetProperties divided by zero
   (third_party/FAudio/BULLY.md), HUDRadar::FlashBlipsOfType compared the wrong blip
   field, two more register hazards (0x6CACC0, 0x6CAD20). tools/diffsweep.py over the
   game code found nothing else real (tools/diffsweep_1030b.txt; its DIFFs are
   triaged noise: constructors' eax, byte results, uninitialised pads). The gameplay
   smoke (tests/gameplay.py: keys, menu, weather, clock, teleports) passed with everything
   ours. The soak (24 teleports in rain with the mod on) then found three more:
   CEntity::GetNearestPointOnBox (vf25) and twelve others were `void` where the game
   returns its output pointer in eax (fixed; tools/outptrcheck.py finds the class);
   NiDX9Renderer::PackSkinnedGeometryBuffer as ours draws with no index buffer bound and
   past its vertex buffer (fixed 2026-10-31: it read the shader declaration's
   PackUntransformedVB, which returns the locked vertex data, as a byte, so an address
   ending in 00 counted as a failure and the stream was packed again in the FVF layout; in
   the 64-bit build, where it always runs as ours, that was torn and spiky characters;
   tools/narrow_return_check.py finds the class); and an intermittent heap corruption
   (bad frees, effect states "destructed twice", garbage in their light lists), found the
   same day with the heap validator written for it ("heap_check = N" in bullyre.ini,
   src/game/HeapCheck.cpp, CommonBlocks.cpp; tests/README.md): ArtArray::Reserve
   (0x6DB480, the generic pointer-array grow CWeapon::GetChild uses) didn't store the new
   capacity, so every weapon's child array grew once and later children were written past
   its end into the neighbouring heap item (the whole hunt: tests/crashes/
   soak_everything_20261030_crash.txt). Fixed; three 60-teleport fast soaks clean after it.
   Also learnt: Bully's own NiAllocator (src/game/NiAllocator.cpp) sends every Gamebryo
   allocation to CMemoryMgr, so the Gamebryo small-object allocator is never used. Open:
   the game with bullygfx sits at ~2.7 GB of private memory in a 32-bit process and twice
   exited silently near 3 GB (an out-of-memory abort; gone with the 64-bit build). Still to
   do before the 64-bit step: missions, vehicles, the front end, saving.
3. The world: entities, sectors, the renderer's lists (bullygfx already replaces the
   D3D9 renderer).
4. Peds, vehicles, the player, weapons; the script VM and its 1,489 natives.
5. The libraries from their source (above): Lua 5.0.2 and libjpeg 6b done 2026-10-22;
   the C runtime's pure parts and XACT → FAudio done 2026-10-23; TinyXML goes with Gamebryo. Audio (XACT): its replacement (FAudio),
   since XACT is 32-bit only.
6. Gamebryo: what the game uses of it (scene graph, NIF loading, animation). The game's code
   reaches 4,089 of the library's functions (tools/gamebryo_reached.txt: calls, function
   pointers, every slot of every vtable it uses, closed over the library). It uses their
   objects by offset and by vtable slot, so they're written like the game's code: same
   layouts, one function per address, in src/gamebryo/ (core: NiCore.h; rules: the
   "Gamebryo passes" section of tools/reversing_rules.md; chunks: tools/chunks_gb/; progress:
   `python tools/remaining.py --gamebryo`). Started 2026-10-23. bullygfx stays the renderer
   behind it.
7. With nothing of the original left: the 64-bit build. The plan (2026-10-23): 64-bit code
   over the game's 32-bit data layout, so none of the 34,000 field accesses by offset has to
   be rewritten:
   - **Game data stays below 4 GB.** The game's heap, its Lua allocator and its pools are
     reserved low. The data sections are mapped at their own addresses, as the host does
     now. The exe is based below 4 GB too, so our code's addresses fit in 32 bits.
   - **Pointer fields stay 4 bytes.** The accessors (`At<T*>`, about 9,000 uses) read and
     write 32-bit pointers: `re::At<T>` in reversed.h is the one definition (on x64,
     `At<T*>` is a `Ptr32<T>&`); all 242 of the files' own copies became `using re::At;`
     on 2026-10-30 (tools/unify_at.py, and the three of another shape by hand). The same day
     1,361 pointer members with layout comments in 296 files became `re::Ptr32<T>` by
     script (the Win32 build is unchanged by both: the alias is the pointer type).
     Typed structs use `re::Ptr32<T>`
     (reversed.h: the pointer type on Win32, a 4-byte wrapper with the pointer's operators
     on x64), and their static_asserts keep the layouts honest. First done 2026-10-30:
     MemoryMgr.h (HeapBlockDesc, CMemoryHeap, CommonBlocksHandle), which then compiles
     clean as x64 and unchanged as Win32. Parameters that point into such fields change with
     them (`HeapBlockDesc** table` became `re::Ptr32<HeapBlockDesc>* table`). The heap's
     lock (0x18 bytes of CRITICAL_SECTION in the layout) is the pattern's exception: x64's
     is 0x28, so the 64-bit build keeps locks outside the layout.
   - **Calls go to our code** (done 2026-10-31, src/x64/dispatch.cpp): the game's code range
     (0x401000..0x8FB992, reserved at the start of the 64-bit exe's .text, which is linked at
     0x400000 like the host) becomes a table of x64 jumps, one at each of the game's function
     addresses. So every game address our code calls (GAME_FN, casts, re::Ours) and every one
     the game's data holds (vtables, the static initializers' table, callback tables) lands in
     our code unchanged: no vtable patching, no GAME_FN rewrite.
     - A registered function: a jump to its entry. The 12,449 bodies written
       `__fastcall F(Self*, re::Edx, args)` for the game's __thiscall functions get an adapter
       taking the game's (self, args) (on x64 the dummy edx would shift every argument by one
       register): `re::Edx` is void* on Win32 (unchanged) and a marker type on x64, and
       `RE_BODY(fn)` at each registration makes `re::BodyOf<fn>`'s adapter
       (tools/edx_marker.py did both conversions). A member function is its own entry.
     - One of the game's jump thunks: a jump to its target's stub (if that is a start).
     - A `jmp [slot]` through the import table: the 64-bit DLL's export of the same name; a
       jump through a game variable: an island reading the 4-byte pointer when called.
     - Anything else: a trap naming the address and its caller (bullyre_x64 --lenient logs it
       and returns 0 instead: a survey of what's missing along a path).
     - The import table is bound at load: each 4-byte slot holds an island below 2 GB jumping
       to the export (some forward to ntdll, above 4 GB). The 32-bit kernel32 exports x64's
       hasn't (InterlockedIncrement and kin) are ours; ExitProcess and MessageBox are watched.
     - GAME_VAR stays where the data is mapped (the exe's .rdata/.data copied in at their
       addresses).
     - Registered bodies may not return a re::Ptr32 by value (BodyOf static_asserts it): on x64
       a class with constructors comes back through a hidden pointer, the game's convention in
       rax.
   - **The 20 inline asm blocks** (15 files) each keep their asm for the 32-bit build under
     `#if defined(_M_IX86)` and have a 64-bit branch (done 2026-10-30): the 16
     register-argument calls (CutsceneCamera's key readers, Occlusion's `out` in ecx,
     RwStateEmulation's enum tables in eax, Sprite's r/g/b in al/cl/dl, VisibilityPlugins,
     AiCombatLocoTracks, BullyMainScene, the Automobile* and BikeProcess damage builders)
     call our body through `re::Ours<Signature>(address)` (reversed.h: the registry's
     `ours` for a game address; the unhooked bodies are registered for it with
     `RE_UNHOOKED_FN(name, address, &fn)`, never installed); the 2 calls whose callee
     destroys a reference built in its argument slot (VehicleAudio2.cpp: Init and
     PlayCollision, and the Automobile*/BikeProcess ones) build it in a local and pass a
     copy; the 2 `fnstcw` reads return a constant (PlayerInput 0x027F, Projectile 0x007F:
     the two disagree; the game under D3D9 runs the x87 at 24-bit precision, 0x007F, to be
     checked in the 32-bit build before the constant matters). The x86-only trap code
     (startup.cpp's CONTEXT registers and x87 state, MainLoop's __control87_2) is under
     `_M_IX86` too.
   - **Libraries:**
     - Gamebryo goes to our engine (bullygfx: scene graph, NIF loading via nif.rs).
     - XACT goes to FAudio (done: the shim).
     - The C runtime's stateful parts move to ours.
     - D3DX and XInput link their 64-bit builds.
   - **The 4 GB that's left is data's own.** No exe, DLLs or driver mappings sit in it, and
     textures and GPU buffers live in the engine above it.
   - **Measured 2026-10-30** (`cmake -S . -B build64 -A x64 -DBULLYRE_X64_EXPERIMENT=ON`,
     then `cmake --build build64 --config Release -- -m > build64/errorsN.txt` and
     `python tools/x64errors.py build64/errorsN.txt`; a full x64 compile takes about 2.5
     minutes): the first x64 compile of all of src/game and src/gamebryo failed on 2,145
     layout static_asserts (832 structs in 391 files), the 20 inline-asm blocks and 9
     x86-isms, nothing else. The same day, by tools (the Win32 build is unchanged by all of
     them, Ptr32 being the pointer type there; it was rebuilt clean after each step):
     - `tools/ptr32_members.py`: every pointer member of a layout (a struct with a
       sizeof/offsetof assert, its bases, the structs its members are of, across headers)
       is a `re::Ptr32<T>`, function pointers and pointer typedefs (NiFixedString, the
       `using ReadFn = R(__cdecl*)(..)` kinds) included; the templated layouts
       (NiTArray<T>, NiTListNode<T>, NiTSet<T>, NiTLargeArray<T>, FixedStringMap<T>, the
       game's arrays) hold `re::Field<T>` elements (a Ptr32 when T is a pointer). 1,837
       members in all; the layout asserts went from 2,145 to 6 (CDummy/CPhysical through
       World.h's CEntryInfoList, fixed since). libjpeg's and DirectInput's structs keep the
       system's 64-bit layouts: their asserts are `#if !defined(_WIN64)`.
     - `tools/ptr32_types.py`: every `T**` is a `re::Ptr32<T>*`, every `T*&` a
       `re::Ptr32<T>&`, every `R (__conv**)(A)` a `re::Ptr32<R __conv(A)>*` (6,251 sites in
       718 files): a pointer to a pointer in this code reaches into the game's data. Left
       alone: `T* const*` (pointers to our own constant pointers: GAME_FN variables, string
       tables; the game parameters of that shape are fixed from the errors), the hooks'
       `reinterpret_cast<void**>(&Original...)`.
     - `GAME_VAR` / `GAME_ARRAY` of a pointer type are the game's 4-byte pointers
       (`re::Field<type>`); on x64 `NiFixedString` is `re::Ptr32<const char>` (NiCore.h),
       so the handle has one type in fields, locals and parameters.
     - `re::Ptr32` grew the pointer's operators for MSVC (subscript, arithmetic,
       comparisons, `static_cast<U*>` through a templated explicit conversion) and
       `re::PtrCast<To>(p)` replaces a reinterpret_cast of a Ptr32 (a pointer or an address
       too: the same spelling everywhere).
     - `tools/ptr32_ripple.py build64/errorsN.txt`: the error-driven fixes of what those
       leave: casts of a Ptr32 (PtrCast; a cast to a function pointer wraps the operand in
       `static_cast<void*>`), `T* const*` parameters of GAME_FNs and our functions fed a
       Ptr32 (`const re::Ptr32<T>*`, only where the declaration's type is the message's: the
       same name is declared with other types in other files), locals whose address or
       reference goes where the game writes a 4-byte pointer (`T* local` ->
       `re::Ptr32<T> local`, arrays included, function parameters included), `auto*` from
       a Ptr32, ternaries mixing a Ptr32 with a raw pointer (the Ptr32 side cast), a local
       fed to a system API's `void**` (back to a plain pointer). Each round: run it, rebuild
       Win32 (the gate), rebuild x64, look at what's left. Hand fixes along the way: the
       pool templates (`Any(CPool<T>*)` and kin in Pools.cpp, Tagging.cpp) and
       NiActorManager's AssignReleasingFirst got x64 overloads taking the Ptr32 (a template
       can't deduce through it); Frontend's VCall takes the object as a deduced parameter;
       VCall.h's Import reads the slot as a plain pointer; HeapCheck reads the TEB through
       NtCurrentTeb.
     - `tools/ptr32_at_clones.py`: the files' own copies of `At` (`template <typename T> T&
       Field(void* object, size_t offset)` and kin, 31 of them) return `re::Field<T>&`: for a
       pointer T, the game's 4-byte slot, not a reference to an 8-byte pointer over it.
     - Added for the last rounds: `re::Ptr32<U>` converts to `re::Ptr32<T>` where `U*`
       converts to `T*` (Ptr32<Derived> to Ptr32<Base>, Ptr32<char> to Ptr32<const char>,
       any to Ptr32<void>); `re::PtrCast<re::Ptr32<F>>(address)` for the key tables of
       function pointers; the slot/value templates (`SetReference(re::Ptr32<T>& slot, T*
       value)` and kin) take `std::type_identity_t<T>*` or have an x64 overload for a Ptr32
       value, so a value read from a Ptr32 field is accepted; the system / COM out-pointers
       (DirectInput's CreateDevice, XACT's CreateSoundBank / GetProperties / Prepare,
       D3DXCreateEffectCompiler, D3D10CreateDevice) take `void**` with a plain `void*` local
       again, since the API writes a 64-bit pointer there.
     - Survey trail: 2,847 errors (errors2, before the day's pass) -> 2,766 -> 2,713 ->
       2,214 -> 1,000 -> 732 -> 321 -> 29 -> 10 -> 0: **the x64 compile of all 956 files of
       src/game and src/gamebryo is clean (2026-10-30 evening, build64/errors11.txt)**. The
       Win32 DLL was rebuilt clean after each round and the game ran it through
       `tests/trial.py @tests/everything_stable.txt` (3,857 frames, 0 mismatches) and
       `tests/gameplay.py` (9,046 frames, 0 mismatches) at the end.
     - What the x64 compiler still warns about is the map of the bring-up work: 30 C4311
       (a pointer truncated to 32 bits: a real pointer stored in a game word, in Lua glue,
       the DX9 renderer's buffers, the material libraries, Occlusion's corner tables), 312
       C4312 (a 32-bit address made a pointer: fine while the game's data sits below 4 GB),
       30 C4302, 32 C4244; the 10 C4477 (`%s` given a Ptr32: 4 bytes through the varargs)
       are fixed (`static_cast<const char*>`). `cat build64/errors9.txt build64/errors10.txt
       build64/errors11.txt | grep "warning C4311"` lists the truncations.
     - Known for the 64-bit bring-up, not the compile: pointers to system or library
       objects stored in game fields or passed through game-typed slots (the D3D9 device
       and its objects in NiDX9Renderer, XACT cues and banks in MDXact*, DirectShow in
       MoviePlayer, the registry key in GameSettings, the D3D10 resources in
       NiD3D10DeviceState) are 64-bit in a 64-bit process and don't fit a Ptr32: those paths
       go to bullygfx/FAudio or get a handle table; a stack address stored in a game field
       truncates (the data heap is below 4 GB, the stack isn't: the dialog template cursor
       of NiD3D10Select/NiDX9Select, the by-value EntityRef of VehicleAudio); the COM
       vtables read through `re::Ptr32<re::Ptr32<void>>` (NiExtraDataHelpers' effect
       compiler) read 4-byte slots of a 64-bit object. Also to do: the 400-odd
       `RE_UNHOOKED` bodies registered for `re::Ours` (21 are), GAME_FN resolving through the
       registry, the vtables, the libraries.

   - **The 64-bit bring-up, 2026-10-31** (bullyre_x64.exe, built with BULLYRE_X64_EXPERIMENT;
     src/x64/main.cpp: `bullyre_x64 [--init] [--winmain] [--lenient] [--report]`):
     - At first the process wasn't large-address-aware (/LARGEADDRESSAWARE:NO): Windows then
       put every allocation, stack, handle, COM object and DLL (ntdll aside) below 2 GB, so any
       pointer the 64-bit code made fit the game's 4-byte fields, at the price of a 2 GB
       process. **Lifted 2026-11-01** (the remaster needs the memory): the exe is
       large-address-aware, and what the game's fields point at comes from the low heap
       (src/x64/lowheap.cpp: regions below 2 GB; the game's malloc family, all our new/delete,
       and the small blocks of the system DLLs whose objects the game keeps: D3D9, D3DX,
       DirectInput, XInput, each CoCreateInstance class's DLL, through their heap imports).
       The 2-4 GB band is held empty, and re::Ptr32 stops the game at the store of anything
       above 4 GB (kPointerTooWide, the crash report names the store). What can't be low is
       kept native: GAME_VAR_NATIVE (a DLL's handle, its functions), re::NativeField (a locked
       buffer's bits), boxed FILE*s. `--stress-high` takes all free space below 4 GB at start,
       so every other allocation lands high and a missed pointer shows at once: the
       four-teleport tour passes 150 s with it.
     - **bullygfx in 64-bit** (2026-11-01): the replacement renderer builds for x86_64 too
       (bullygfx's NOTES.md, "The 64-bit game") and is loaded as bullygfx64.dll from the
       exe's folder for the renderer's "D3D9.DLL" (dispatch.cpp Game_LoadLibraryA; the system
       d3d9 is loaded first so its heap is the low heap's). The exe exports bullyre_low_alloc,
       bullyre_low_free and bullyre_low_pages (lowheap.cpp) for what bullygfx hands the game.
       First run: the tour, 150 s, the replay renderer and a 1,971-texture mod on, the process
       at 2.45 GB. Then, for mods that bring their own texture files (named by paths of the
       author's machine, sent to bullygfx's cache by its CreateFileA hook): crt64's fopen opens
       through the game's CreateFileA import slot as the game's runtime did (it called the system
       directly: those textures came out as Gamebryo's magenta); the system DLLs' blocks of
       256 KB and up (a texture's system copy) come from the top of the address space, so a big
       mod's data doesn't use up the space below 2 GB the low heap grows into, and the low heap
       reserves 1.5 GB at the start; NiCriticalSection::Unlock (0x401000, the DDS reader's) got
       its body. The tour with that mod: 150 s without --lenient, ~29 fps (the 32-bit game: ~34),
       the process at 4.6 GB. (Under cdb the same scene runs at ~7 fps: measure without it;
       `--profile S` samples the main thread.)
     - **Every mission** (2026-11-01): tests/missions64.py runs the 250 missions of the game's
       table (classes, minigames, races, jobs and the story; not the tutorial popups, doors or
       debug tests) in games side by side: bullyre_x64 `--missions S:E:LIST` force-starts each
       as the script function ForceStartMissionIndex does, `--mission-pass P` passes it
       (CMissionSlot::Pass), `--start` presses ENTER until the game has started, `--tag T`
       names the run's files (and bullygfx64's command file), `--shots-every N` takes
       bullygfx64's pictures. A run waits for the slot to settle between missions and answers
       "This will end your current task" with Yes. Found and fixed with it: the bike crash
       (PlayerUserInputHandler::Update read the vehicle's data one pointer too deep: every
       ride with the keyboard's look-back keys), ScriptCameraController::SetPath's PathNode
       local (0x14 for 0x24 bytes: the /GS cookie stopped Gym 1), the CdStream semaphore
       (the game names it, so copies of the game running together woke each other's stream
       threads and hung at STORY; unnamed now; the Sync/thread race is closed with a lock too),
       and the system heap getting low-heap blocks from DLLs whose imports aren't turned
       (ntdll's RtlFreeHeap / RtlReAllocateHeap / RtlSizeHeap hooked: shcore.dll was one).
       tools/local_buffer_check.py then found six more short stack buffers the game's code
       writes past (CharTriggers' ped position, NiCloningProcess x2, the display settings, a
       prop's trigger path, the shadows' first index): it estimates from the original code how
       far a game function reaches through each local buffer or scalar handed to it. The
       sweeps then found (and the reruns passed): the arcade games' RwRasterLock storing the
       locked texture's pixels (anywhere in the 64-bit process) in a 4-byte field (now a
       re::NativeField); and three paths where the original game itself stops, hit by
       force-started missions: the PropAnim pool full (220) starting a race in the business
       district (CreatePropForTrigger now leaves the prop out), a weapon snapshot restored
       without one (4.06), PAnim natives on a trigger whose area isn't loaded (the carnival
       rides). Result (build64/missions/final_results.txt): 244 of the 250 ran without a crash;
       the other 6 end or wait on their own when started this way (state 12, or loading), none
       crashed. The test windows open without taking the focus (re::Background: dispatch
       overrides of CreateWindowExA / ShowWindow / SetForegroundWindow), and the 32-bit trial
       after it all: 4,061 frames, 0 mismatches.
     - **Remaster switches** (2026-11-01, runflags.h; bullyre_x64 turns them on, the 32-bit
       build keeps the game's behaviour): the light manager's shadow maps at 2048 (the game's
       1024; each light's own size is copied into its generator when the light is handed out,
       so that is scaled; `--shadow-map N`), and screens wider than 16:9 up to 32:9 (the
       settings check stopped at 1.8 and fell back to 800x600; the 3D view keeps 16:9's
       vertical view and widens at the sides, the camera's side culling planes with it, and the
       HUD's scales squeeze by the screen's aspect over 16:9 as the game squeezes them for 16:9).
       Checked at 1920x810. bullygfx64 asks for FLOAT32_FILTERABLE: the High shader set's shadow
       maps are R32F, and without it every bind group with one was invalid (a black window).
     - **Seamless doors, step 1** (2026-11-01, src/game/SeamlessDoors.cpp; on in bullyre_x64,
       `--no-seamless-doors` turns them off): no fade to black at doors. Near a door into an
       interior, that interior is loaded beside the current area with the game's own extra scene
       (WorldStreamArea / AreaAddExtraScene: placement, collision, models, and the area counted as
       current); near a way out, the town's models around the place outside are asked for every
       half second (kept loaded, the town not made current). A door used after a second of that
       skips the fade (AreaTransitionManager state 4), and the interior's lights are handed out in
       the switch frame. The bike shop from the street: 12 black frames and a 5-frame fade-in
       before, none after, both ways. `--doors` lists each area's doors (with where they lead)
       and times each transition's states; `--use-door S:AREA` uses the nearest door to AREA.
       Left: the game's own door camera shot and the area script's load (60-340 ms, the player
       held at the door); step 2 draws the interior through the open door.
     - **Seamless doors, step 2: portals** (2026-11-02): the interior is drawn live through its
       open door, from the main camera moved through the doorway. A door's two sides are its
       trigger here and the one on the other side whose way back comes out by it (the door
       props' positions and headings; the door model's box gives the doorway, both leaves of a
       double door); an interior's own triggers only exist while it's current, so every list's
       doors are noted in bullyre_x64_doorways.txt beside the exe. The pass (SeamlessDoorsDraw,
       from MdBullyMainViewRenderClick::RenderScene after the world, before the close-up layer):
       bullygfx64 gets the doorway (bullygfx_portal: it marks the stencil where the doorway
       passes the depth test and pushes the depth there to the far plane, portal.rs); the
       render list is made from the moved camera (only its frustum: CRenderer::ScanWorld under
       re::PortalScan), the area's side of its own doorway left out (the street backdrop seen
       through its door from inside), the list "rendered" (RpAtomic::DefaultRender only places
       geometry under Lit Objects and marks it; the click's culler draws), the area's lights
       measured from where the player would be in it (re::LightsFrom) and attached after the
       geometry is (the light manager attaches lights to the nodes there), the new nodes alone
       brought up to date (the town's effect states left alone: made again they'd remake its
       materials every frame), then the scene culled from that camera into the accumulator with
       render state 196 = 'PRT1' (bullygfx keeps those draws to the stencil). The door's camera
       (controller 4) is left out while a portal is ready, and the door's countdown ends when
       the player is through the doorway. ~0.7 ms a frame; the dorm and the bike shop checked.
       The switch (2026-11-02) puts the player where the doorway maps them, half a metre inside
       the door with the heading turned likewise (re::SeamlessDoorPlacement), instead of the
       area's way in metres further on, and the follow camera looks the way the main camera
       did, turned. Test flags: `--portal-test` (the door looked through not drawn),
       `--portal-full` (the doorway the whole screen), `--no-portal`.
     - **Seamless doors, both ways and through the switch** (2026-11-02, later):
       - Out of an interior: the town is drawn through its door too. The town behind an exit
         is the extra scene's point for placement and collision only (re-flagged inactive, so
         nothing counts it current around the interior), its models asked for; the pass draws
         the town's sky first (MdBullyMainViewRenderClick::RenderBackground, kept to the
         doorway) and takes the town's fog constants (BullyMain::UpdateFogConstants, 0x4013C0,
         for the pass's camera: the interior's made the town white).
       - The switch carries everything over (re::SeamlessDoorSwitch, ATM state 9 before the
         world is cleared): the area left becomes the extra scene around its door's way-in
         point, ready at once, its loaded models near the door pinned (GAME_REQUIRED) while
         needed; the doorway is inverted (the door back); the camera goes on exactly where it
         was drawn, mapped through (the last drawn LTM: by state 9 the camera's frame has been
         moved to the player's head), placed there (FollowCameraController3::Place takes it,
         re::SeamlessCameraPlace), not swept in by the walls (re::SeamlessCameraFree), the fov
         as drawn (the switch resets it to the 85 default) and eased, not snapped.
       - While the camera is still on the old side ("the reverse"), the area left is drawn
         AROUND the doorway (bullygfx_portal2 mode 1, PORTAL_AROUND: the stencil 2 over the
         doorway with the nearer depth, 1 elsewhere at the far plane; the area's draws 'PRT2'
         keep to stencil >= 1, its sky 'PRT1' to 1; the doorway quad isn't cut by the near
         plane), and this side's new, shut doors and an interior's backdrop are left out of the
         main view. It ends when the drawn camera crosses the doorway, or the player is 8 m
         away (60 s at most). The new area's door list isn't filled for a frame or two after
         the switch: the preload is kept meanwhile.
       - The pass's scan fixes: the main camera's interior box (rs_bCameraInInterior: the scan
         kept to its sectors and heights) and the occluders aren't the pass camera's; its own
         camera's sectors are scanned; objects and dummies the streaming let go of are made
         again by the doorway (CEntity vtable[10]); the pass camera's near plane is the main
         one's (RenderWare's default keeps it >= far / 500); models it shows aren't faded in
         from nothing. The other side's own door (when its placement made one) is left out of
         the pass: shut, it stood in the doorway (the girls' dorm's looked black from inside).
       - A doorway's line is the door prop's own heading (trigger +0x10 degrees less +0x38
         radians, as CreateAreaProps turns it), not the trigger's +0x38: on the girls' dorm the
         two are 180 degrees apart, which put the doorway a door's width aside (the player came
         out a metre beside the door, the portal looked at the wall). The doorways table has a
         ninth field for it (rewritten when an old entry learns it).
       - Checked with shot bursts: the boys' dorm both ways (walking in with R; out with
         `--use-door`) and the girls' dorm out. Left: leaving an interior waits in ATM state 6
         for the area's Lua scripts to end (CLurStore +0x2B09); the girls' dorm's sometimes take
         ~11 s, the player standing at the door meanwhile (the original game is black then);
         walk-in triggers (no door prop) have no doorway (no portal, the route's place).
     - **Two areas live at a seamless door** (2026-11-03; `--no-two-areas` turns it off): the
       area behind the door the player is by (or just came through) is peopled too, and the
       switch deletes nobody.
       - Facts it rests on: a ped's +0x111 is always 0 (no ped knows its area); peds are kept by
         distance from FindPlayerCentreOfWorld (PedPop.dat: in view 55/50 m, out of view 30/20,
         exterior/interior, |dz| <= interiorHeight + 4 inside); the interiors are hundreds of
         metres from the town, so an interior's peds can't live while the player is outside;
         PedArrivals::AreaChanged (0x48BAC0, from PlacePlayer) deletes every ambient ped at
         an area change and StopPeds (0x48B480) stops them all. The ped pool is 24 (19 ambient).
       - CPedPopulation::Update runs a second pass (PopulationPeds.cpp two_areas): CGame::currArea
         made the area there, FindPlayerCentreOfWorld answering the place the doorway maps the
         player to (re::SeamlessPopulationGhost; the two at least 120 m apart, the preload
         ready), its own spawn points and forced model (swapped in), its own type counts (each
         pass counts only its peds: numPeds adjusted around the passes), CCamera made the main
         camera moved through the doorway (re::SeamlessGhostCamera) and IsSphereVisible
         answering what the doorway shows of it (re::SeamlessSeenThroughDoorway). A ped is the
         pass's whose centre it is nearer; CullPeds and RemoveFarPed skip the other pass's
         (RemoveFarPed's force, the pool nearly full, takes the other side's first). The second
         pass makes its peds out of the doorway's view as far as cull[3] (not cull[5]: by a
         doorway most of an interior's near floor is in view) and keeps them to cull[0].
       - At a seamless switch AreaChanged moves the followers as ever and leaves the rest (both
         sides' are about already), without the spawn ring; StopPeds isn't called.
       - Checked: the boys' dorm in (its 5 made within half a second while the player is outside,
         out of view, kept through the switch) and out (its 5 kept behind, the town's by the door
         kept going in), three switches in a row with no crash; the girls' dorm out (a town ped
         made by the door beforehand; the dorm's kept behind, those that went upstairs culled by
         the interior's height rule). Not yet: area scripts' peds (the prefects; a script runs only
         once the player is in its area), a bigger ped pool (the pool's 24 is inlined in places:
         PedCollisionResponse 0x18, PedDamage 0x17, NativesPedScript, PedCollisionQueries'
         24-entry results, PedNavPoint's loop).
     - **Launching from anywhere** (2026-11-02): `bullyre_x64.exe --play --game-dir <folder>`
       runs the game in <folder> wherever the exe is. The folder is made current at once (logs,
       crash reports, the doorways table go there), bullygfx64 is told it (BULLYGFX_GAME_DIR:
       its settings, mods, Stream and caches, else the exe's folder as before), and the game's
       own GetModuleFileNameA for its module is answered with <folder>\Bully_unpacked.exe
       (src/x64/dispatch.cpp: WinMain makes that folder the game's path). bullymenu (the
       launcher, ../bullymenu) has a game folder picker and a "64-bit remaster" switch that
       starts bullyre_x64.exe from beside itself so (no patches, no injection: the 64-bit
       game's code isn't the game's; its data is, at the same addresses, so the menu's data
       features still apply). It's one file: ../BullyRemaster/BullyRemaster.exe is bullymenu
       built with bullyre_x64.exe, its .map and bullygfx64.dll deflated inside it (bullymenu's
       build.rs takes them from build64/Release and bullygfx's x86_64 build), unpacked into
       %LOCALAPPDATA%\BullyRemaster\bin when the bundle changes and run from there; its
       settings and WebView data live in %LOCALAPPDATA%\BullyRemaster too. A new game build
       ships by rebuilding bullymenu.
     - The 64-bit build's world pools are bigger (CPools::Initialise: prop anims 220 -> 1000,
       dummies 300 -> 1200, buildings 2250 -> 6000, pointer nodes 15000 -> 0x3FFF, the most a
       sector list's packed node can index): with an interior kept loaded beside the town the
       game's sizes ran out (a placement file's rail made from a full prop anim pool: a null
       prop's flags set).
     - Test runs with `--keys`/`--start` read only their own keys and mouse: with
       `--keep-active` the game reads the keyboard however it's focused, so keys typed into
       other windows used to move it.
     - All 27,525 registrations run their static initialisation in the 64-bit process; the
       dispatch tests pass (cdecl and thiscall bodies through game addresses, float and stack
       arguments, thunks, the trap).
     - `--init`: the game's own 2,758 static initializers (the table at 0x8FC350) run through
       their game addresses: all 2,758, no faults, 1,825 exit functions registered.
     - `--winmain`: the game's WinMain gets through its setup, the display settings and
       DirectDraw's video memory, all of Gamebryo's static data (NiStaticDataManager: the
       memory manager, the string table, the shader constant tables, every library's init),
       CGame::InitialiseOnceBeforeRW, into CreateApp: BullyMain::Initialise, NiSample's
       Initialize, NiShadowManager::Initialize (where it stopped: a pointer set's Realloc).
     - What the bring-up found, by class, and fixed (tools for the classes):
       the game's C runtime's stateful functions (heap, rand, files, exit list, C++ exception
       objects: src/x64/crt64.cpp, ours on x64); short functions never registered (too short
       to hook: tools/unhooked_tiny.py wrote 47 bodies from their bytes; tools/unhooked_bodies.py
       registered 128 written ones with RE_UNHOOKED_FN); the files' Import helpers reading an
       import slot as 8 bytes; system COM objects' vtables read as 4-byte slots (re::ComVtable,
       tools/com_vtables.py: a __stdcall call through a vtable is a COM one, a __thiscall one a
       game object's); NiCore's VtableOf reading a game object's vtable pointer as 8 bytes;
       system structs in game buffers at x86 sizes (SYSTEM_INFO, MSG; CRITICAL_SECTION: a game
       section's first 4 bytes point to a real one, re::GameSection, also for the import
       slots' calls); size_t& of the game's 4-byte sizes (NiAllocator); `T* const*` of the
       game's ABI (tools/ptr32_constptr.py, 326 parameters) and the game's pointer tables
       (tools/ptr32_tables.py, 87 sites); pointer members of the game's structs that had no
       size assert (tools/ptr32_members.py now also takes GAME_VAR and At<> types: 262 more);
       `At<uintptr_t>` and kin writing 8 bytes over a 4-byte field (tools/ptr32_uintptr.py, 556
       sites); a Ptr32 returned by value (5 declarations, 2 bodies). Each round: Win32 build
       and tests/trial.py as the gate (4,246 frames, 0 mismatches after all of them).
     - The loop from here: `bullyre_x64 --winmain` (under cdb for a stack:
       "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe" -y build64\Release
       -lines -c "g;kn 16;q" build64\Release\bullyre_x64.exe --winmain; bullyre_x64 has a pdb),
       fix what stops it, rebuild (build64 and the Win32 gate), again. `tools/x64census.py`
       counts what's left: the game functions with no body, the ones our source or the game's
       data still names, and writes build64/x64_named_starts.txt (the dispatch's extra stubs).
       Known ahead: OVERLAPPED in CdStream's game layout (0x14 bytes, 0x20 on x64), the D3D9
       structs with pointers in game memory (D3DPRESENT_PARAMETERS, D3DLOCKED_RECT), the
       renderer (the system's 64-bit d3d9 for now; bullygfx built for x64 later), the 30
       pointer truncations the compiler lists (C4311).
     - **Later on 2026-10-31: the 64-bit game plays.** bullyre_x64 --winmain goes through the
       title menu (STORY), loads the save, runs the scripts, and puts Jimmy in the dorm with
       the world rendered, peds, HUD and audio; scripted keys walk him around and teleports
       take him outdoors (the trial's four spots): 170 s at about 60 fps, no crash, without
       --lenient (every function reached has a body). Test-run switches: `--keep-active`
       (no pause without the focus), `--keys K@S[:MS],...` (DirectInput keys merged into the
       game's keyboard reads), `--teleport "S:x,y,z,area[,heading];..."` (the script
       function PlayerSetPosXYZArea's request), `--seconds N` (a watchdog line every 10 s:
       frames, game time, geometry draws, camera / player, visible entities; a frame grab of
       the back buffer at Present, build64/x64_shot.png). The display mode is never changed
       (re::SetForceWindowed). What it took, by class (each fixed where it's shared, mostly
       by a tool): re::Field missed `T* const`; function-pointer slots read as 8 bytes
       (re::GameFnAt / re::GameVirtual, tools/gamefn_slots.py); game COM objects handed to the
       system (re::NativeInterface: D3DX's state manager); system out-pointers into 4-byte
       slots (re::OutPtr); sizeof(T*) over game arrays; XACT's structs in the shim
       (runtime/streaming params, notifications) translated; anonymous local structs passed
       to game code (Ptr32 members); dummy-edx `__fastcall` declarations (now `__thiscall`);
       32-bit ints passed where the body takes a pointer (tools/x64_width_fix.py: 656
       declaration parameters -> uintptr_t; and the adapters now normalise stack-slot
       arguments, reversed.h StackArg); a vector passed as a struct one side and floats the
       other (tools/x64_vector_args.py: 112 declarations get the body's shape plus an inline
       wrapper; a static_assert in re::Arg rejects structs by value through variadic calls);
       stdcall bodies whose callers are thiscall (x64_abi_check.py --fix-shift; it now
       recognises every registration macro); Lua 5.0's loader reading the game's 32-bit
       chunks (size_t 4, third_party/lua-5.0.2/src/lundump.c); the OpenMP imports (vcomp:
       the scene update's parallel loop, run on the calling thread); the sound banks' and
       stream channels' OVERLAPPED (re::SystemOverlapped). Still to do: the shutdown path
       (NiMainSDM's and the static exit functions' bodies), longer play (missions, vehicles,
       the front end's other screens), bullygfx for x64, and a 64-bit trial that compares
       against the 32-bit build.

The exe has 20,695 functions; a good part of them are Gamebryo, the C runtime, Lua
5.0 and XACT, which are replaced as libraries rather than function by function.
