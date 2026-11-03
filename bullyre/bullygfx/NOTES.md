# bullygfx: replacing Bully SE's renderer

Goal: every pixel of Bully: Scholarship Edition drawn by our own renderer on a
modern API, with Direct3D 9 gone. This file is the map of what has to be replaced,
measured in the running game (stage 1 captures, 2026-09-27: title screen, loading,
gameplay in Jimmy's dorm room, pause menu, then outdoors at the bus yard and the
main campus with students around; 38,196 frames in all). Outdoors used the same
draw paths as indoors, at about twice the draws: ~250 DrawIndexedPrimitive and ~17
DrawPrimitive per frame. The only renderer method new outdoors was slot 50, used rarely.

## How the game draws a frame

The game draws through three separate paths. The new renderer has to take over all three.

### 1. Gamebryo 2.x (most of the world)

- One renderer: `BullyNiDX9Renderer` (Gamebryo `NiDX9Renderer` subclass), vtable
  **0x8FEFE4**, 105 methods. There are no other `NiRenderer` implementations in the exe
  (the D3D10 strings are leftovers from D3DX and the shader library).
- It loads `D3D9.DLL` by name (LoadLibraryA at 0x878D46) and keeps the IDirect3D9 at
  0x20E6D04. That is why a `d3d9.dll` in the game folder gets loaded, and it is how
  this project gets in.
- Device: created once, `CreateDevice(adapter 0, HAL, HARDWARE_VERTEXPROCESSING |
  PUREDEVICE)`.
- Materials: Gamebryo's shader system, `NiD3DXEffectShader` / NiD3DPass
  (0x746xxx–0x74Dxxx), shaders from `ShaderBinaries\{Low,Med,High}`.
- Draws: renderer slot 100 (0x402E90, a Bully override of the mesh draw), and
  `DrawSkinnedPrimitive` (0x87E7D0) for skinned characters.

Renderer methods the game uses every frame (calls per frame, from the capture):

| slot | offset | function | /frame | what it is (from the counts and callers) |
|---:|---:|---|---:|---|
| 2 | +0x008 | 0x401B00 | ~1000 | GetRTTI (returns 0xBCBBCC; IsKindOf checks) |
| 29 | +0x074 | 0x879540 | 3.2 | getter (`[this+0x680]`) |
| 58 | +0x0E8 | 0x87BF10 | 3.7 | |
| 80 / 81 | +0x140/+0x144 | 0x49A650 | 6.6 | no-ops (`ret`), bracketing each render pass |
| 82 / 83 / 84 | +0x148–0x150 | 0x882260 / 0x877D00 / 0x87C7B0 | 1.0 | BeginFrame / EndFrame / DisplayFrame |
| 85 | +0x154 | 0x87BB30 | 8.3 | ClearBuffer (probably) |
| 86 | +0x158 | 0x879730 | 5.9 | |
| 87 | +0x15C | 0x879CD0 | 40 | SetCameraData (probably) |
| 88 | +0x160 | 0x877D40 | 3.6 | SetScreenSpaceCameraData (probably) |
| 89 / 90 | +0x164/+0x168 | 0x87B540 / 0x87B790 | 5.4 | Begin/EndUsingRenderTargetGroup (5 passes per frame) |
| 95 | +0x17C | → 0x8811A0 | 81 | RenderShape (tri shapes) |
| 96 | +0x180 | → 0x8811A0 | 0.3 | RenderTristrips |
| 100 | +0x190 | 0x402E90 | 75 | Bully's mesh draw (DrawIndexedPrimitive) |
| 101 | +0x194 | → 0x87E7D0 | 6 | DrawSkinnedPrimitive |

Called rarely (creation, resources, capabilities): 19, 22, 23, 28, 30–32, 39, 40, 42,
44, 46–49, 52, 53, 65, 67–69, 71, 72, 76 (shader version caps), 77, 103.
Never called in the capture: the other ~60 slots.

### 2. Rockstar's 2D drawing module: HUD, radar, menus (Direct3D directly)

- Code 0x738DD0–0x73A5xx: its own vertex/index buffers, vertex declarations and
  shaders (compiles `vs_2_0` "main"), DrawIndexedPrimitive at 0x73959D and 0x739F3C,
  DrawPrimitive at 0x73A3F3.
- Callers: radar (0x54C570: `radar%02d`, `1stradar%02d`…), HUD/front end
  (0x52Bxxx–0x549xxx), and fullscreen quads for the post effects.
- About 150k indexed draws and 70k DrawPrimitive in the capture (~25 + 12 per frame).

### 3. Rockstar's post-processing (D3DX effects, directly)

- Manager at 0x55E2C0 (called from the frame at 0x43CCA6), D3DX effect helper at
  0x55F5B0 (`DefaultTechnique`, `gSceneTexture`, `gTime`, `gViewProjection`).
- Effects: heat haze (`HeatRipple_01/02`, 0x55F910/0x55F9C0), motion blur
  (0x55FDE0), blur/bloom glow (`gGlowThreshold`, 0x5601F0/0x560380), tunnel vision and
  wobble (0x5608D0).
- Also direct: SetRenderState from 0x406630 (called by 0x519xxx) and SetSamplerState
  from 0x5EE1D0 (the frame at 0x43Cxxx).

## What the renderer methods get (stage 2 capture)

Arguments sampled in the game; object classes come from Gamebryo's own RTTI
(vtable slot 2 is GetRTTI, compiled as `mov eax, &NiRTTI; ret`; the NiRTTI starts with
the class name). The exe has 283 NiRTTI objects, and 279 vtables can be named
statically (they're built by `push name; mov ecx, &rtti; call 0x765410`).

| slot | /frame | arguments | method |
|---:|---:|---|---|
| 82 / 83 / 84 | 1 | none | BeginFrame / EndFrame / DisplayFrame |
| 85 | 8.3 | NiRect* (NULL), mode (7 all, 4 depth, 0) | ClearBuffer |
| 86 | 5.9 | eye, dir, up, right (NiPoint3&), … | SetCameraData |
| 87 | 40 | NiRect* (NULL) | SetScreenSpaceCameraData (the 2D module) |
| 88 | 3.6 | out parameters | GetCameraData |
| 89 / 90 | 5.4 | NiRenderTargetGroup*, clear mode 7 / none | Begin/EndUsingRenderTargetGroup |
| 95 / 96 | 81 / 0.3 | NiTriShape* / NiTriStrips* | RenderShape / RenderTristrips |
| 100 | 75 | NiTriShape, NiTriShapeData, NULL, NiTransform* (world) | mesh draw (Bully override, 0x402E90) |
| 101 | 6 | NiTriShape, NiTriShapeData, NiSkinInstance | skinned mesh draw (0x87E7D0) |
| 58 | 3.7 | Ni2DBuffer* src, Ni2DBuffer* dst | FastCopy (the StretchRects) |
| 48 / 49 / 50 / 67 | rare | NiGeometryData / NiMaterialProperty / NiTextureEffect / NiSourceTexture | precache / purge of renderer data |

Bully's own render pieces seen in the arguments: `MdBullyMainViewRenderClick`,
`MdBullyAccumulator`, `HUDRadar`.

### Geometry: where the vertices are

NiTriShapeData (vtable 0x953BC8, 0x58 bytes): +0x0C u16 vertex count, +0x10 bound
(center xyz, radius), +0x20 vertices (NiPoint3*), +0x24 normals, +0x28 colours
(NiColorA*), +0x2C UVs, +0x34 and +0x3C renderer-side pointers, +0x44 triangle-list
length (indices), +0x48 indices (u16*).

Only 159 of 928 live NiTriShapeData kept their CPU vertices (checked on campus):
Gamebryo drops them once they're in GPU buffers. So the new renderer has to take
geometry from the renderer data (the D3D9 vertex/index buffers, likely MANAGED pool
so readable), or copy it as it's uploaded.

### Materials: from a mesh to its textures (read in the game)

- NiGeometry (NiTriShape, 0xE0+ bytes): +0x18 parent NiNode, +0x94 property list
  (embedded list: +0x98 head node {next, prev, property}, +0xA0 count), +0xC4 the
  resolved NiPropertyState, +0xCC NiTriShapeData.
- NiPropertyState: 12 slots from +0x08, one per type: alpha, dither, fog, material,
  renderer-specific, shade, specular, stencil, **texturing (+0x28)**, vertex colour,
  wireframe, z-buffer (+0x34).
- NiTexturingProperty: +0x20 array of Map* (count u16 at +0x24). Gamebryo's slot order:
  0 base, 1 dark, 2 detail, 3 gloss, 4 glow, 5 bump, 6 normal, 7 parallax, 8 decal.
  A map keeps its NiSourceTexture at +8.
- NiSourceTexture (0x48 bytes): +0x08 file name, +0x24 renderer data.
- NiDX9TextureData: +0x08 back to the texture, +0x0C width, +0x10 height, +0x20
  format (FOURCC), **+0x5C the IDirect3DBaseTexture9**.
- Texture stages don't tell you which is which: the normal map is on stage 0 for the
  lit building shaders, and the base colour is on stage 2 for others. The material says.

## Where the new renderer is (2026-09-27)

The replay renderer (replay.rs) draws the whole game on wgpu/Vulkan at pixel parity
with D3D9: 12 test scenes (dorm room, pause menu, campus, bus yard, town, dusk, night,
interiors) differ by a mean of 0.03-0.52 out of 255; what's left is per-pixel
alpha-test jitter on leaf edges (the drivers' anisotropic sampling).

- **Capture** (capture.rs): every device Draw*/Clear/StretchRect/ColorFill is recorded
  with the device state read back through the Get* methods (the device is created
  without D3DCREATE_PUREDEVICE for that): shaders, declaration, streams, indices,
  256 VS + 224 PS constants, 16 textures with sampler states, render states,
  viewport, scissor, targets. Every object named keeps a reference (`Held`) until
  the replay has run, so nothing is freed and its address reused mid-frame. Dynamic
  vertex/index buffers are copied at the draw (the game rewrites them in the frame).
- **Translation** (dxso.rs): D3D9 vs/ps 2.0/3.0 bytecode to WGSL (all 613 game
  shaders translate and pass naga). D3D9's half-pixel: positions move half a pixel
  right and down. Alpha test is done in the pixel shader from the render states.
- **Replay** (replay.rs), at the game's Present: pipelines by (shaders, vertex layout,
  topology, blend/depth/stencil, targets); our own copy of each render target and
  depth surface; textures uploaded (DXT kept compressed); a render-target texture the
  replay hasn't drawn is read back once from D3D9. Then the back buffer is blitted to
  a child window over the game's.
- **Caches** are keyed by the game's object pointers and hold a reference on each;
  every 120 frames objects only we still reference are dropped. Before a device
  Reset all references go. Buffers are re-read only after the game locks them for
  writing (buffers.rs hooks Lock in the buffer vtables).
- Vertex elements reaching past the stream stride (D3D9 reads on into the next
  vertex) get their own buffer slot at their offset.
- **Exclusive**: with it on, D3D9 draws get 0 primitives, so only the replay draws.
  F8 (only while the game is in front) turns the new renderer on exclusive.
- **Threads**: the game thread only records (~0.7 ms a frame); the replay runs on
  its own thread ("bullygfx render", ~7 ms) while the game goes on with the next
  frame, one frame handed over at a time. The device is created
  D3DCREATE_MULTITHREADED since the render thread reads the game's resources; its
  window is resized/shown asynchronously (it belongs to the game thread).
- **Shadow state** (capture.rs): the device state is followed from the Set* calls
  instead of ~100 Get* calls a draw; read back in full only after a state block Apply
  (hooked in the state block vtable), a reset, or turning capture on. SetRenderTarget
  resets viewport and scissor (read back). Constants are shared between draws (Arc,
  copy on write).
- The per-call diagnostics (callers, argument kinds, the old mesh-level scene) are off
  unless `diag on`: at ~6000 calls a frame they cost several ms.
- Uncapped at the bus yard: D3D9 104.6 fps, the new renderer alone 112.7 fps.
- Buffers the game made WRITEONLY are made readable at creation (D3DUSAGE_WRITEONLY
  removed). Moving static DEFAULT buffers to MANAGED hangs the game at start-up
  (it locks them with DISCARD/NOOVERWRITE).

- **Address space**: the Vulkan driver next to the D3D9 one needs ~620 MB more; in
  2 GB the process ran out (~1.86 GB) and nvoglv32.dll crashed on a null pointer
  during an area change. The exe is now large-address-aware (bullydumper sets it by
  default, `--no-laa` to not; bullymenu sets it on an existing Bully_unpacked.exe).
- **Failure**: a panic in bullygfx turns the new renderer off (D3D9 draws again) and
  leaves bullygfx_panic.txt with a backtrace (d3d9.pdb next to the dll resolves it);
  a native crash leaves bullygfx_crash.txt (code, module+offset, stack).
- The game issues `DrawPrimitive(fan, 0, -1)` (a count underflow D3D9 refuses); such
  draws, and 0-primitive ones, are left out.

- **Customiser** (bullymenu's Renderer page): the menu writes
  `bullygfx_settings.txt` in the game folder (settings.rs reads it when it changes)
  and the DLL writes `bullygfx_status.txt` back. The new renderer on/off (exclusive),
  the picture pass on the finished frame (exposure, contrast, saturation, gamma, tint,
  vignette, sharpen; `shot final` captures it), texture modes (game, replaced,
  checkerboard, untextured), export and reload. The menu copies the newest d3d9.dll
  (and pdb) into the game folder at launch.
- **Texture names** (textures.rs): learnt as meshes are drawn: property state +0x28
  NiTexturingProperty, its maps' NiSourceTexture +0x08 file name, D3D texture via
  +0x24 → +0x5C. Textures no mesh names go by `unnamed_<content hash>`. Export writes
  PNGs (DXT decoded) to bullygfx_textures/export; bullygfx_textures/replace/<name>.png
  replaces (mipmapped on load).
- Exclusive mode only leaves out D3D9 draws to the screen: draws into render
  textures still happen in D3D9 too, because the game reads some back through D3DX
  (Jimmy's clothes are composited into X8R8G8B8 targets, then baked with
  D3DXLoadSurfaceFromSurface on the real device).
- Alpha-less targets (X8R8G8B8, R5G6B5, X1R5G5B5): D3D9 samples alpha as 1, so our
  copies keep alpha 1 and never write it (the missing-torso bug).
- A texture's surfaces count references apart from the texture: the caches hold the
  texture for surfaces we copy (GetContainer), or the sweep drops them while in use.

- **FOV and aspect** (camera.rs): SetCameraData's NiFrustum (NiCamera +0x104) is
  rewritten before the call, so projection and culling follow. The game's FOV
  (0xC3CD04, 85° horizontal) is at 16:9 (top = right * 9/16); the chosen FOV scales the
  vertical angle (the game's zooms still work) and Hor+ sets the width from the real
  aspect (RESW/RESH 0xBCBC60/64). The game's own values are remembered, so it isn't
  compounded frame to frame.
- **Any window size**: the game reset any resolution not in the adapter's mode list to
  800x600 (0x405AB0 fails -> 0x405A00). bullymenu patches the check to pass when
  windowed; bullygfx also adds the registry resolution to the modes D3D reports
  (modes.rs). The HUD is still drawn at 16:9 stretched on wider windows.
- **Wet streets** (wet.rs): on in rain (weather word 0xCE756C == 2; the `weather N`
  command calls WeatherSet's 0x5360E0) or always. The camera is recorded as a Cmd from
  SetCameraData; the pass runs after the world (far > 50 camera) is drawn into the back
  buffer and before the game clears depth for close-up 3D or copies the scene for its
  post effects. View positions from depth (D3D z = f*n/(f - d*(f-n)), x/y from the
  frustum tangents), normals from derivatives, up-facing surfaces wet, noise puddles,
  ripples, ray-marched screen-space reflections.
- **Live window resize**: the game's pending video mode at 0xBCBC78 (width, height, AA,
  then three more; a non-zero AA marks it) is applied by the main loop (0x406342 ->
  0x405F60: device reset and window, then saved; rolled back if it fails). bullymenu
  makes the window resizable and, once a drag ends and the size holds, writes the
  client size there, then snaps the window to exactly the applied resolution.
- **Wet streets over time**: wetness rises over ~30 s of rain and dries over ~90 s;
  the ground darkens first, then puddles (simplex noise, domain warped) grow as their
  threshold drops.
- **Live Data page** (bullymenu viewer.rs): readable_offsets.csv with values read live
  (address expressions `[x]+0x..`, struct fields from the player's ped or vehicle),
  pools walked for the nearest peds and vehicles.

Re-skinning goes on top of this: textures by NiSourceTexture file name, shaders
(replace a translated WGSL module by hash), materials/lighting (constants), models.

## The 64-bit game (bullyre_x64.exe, 2026-11-01)

The same source builds for both: `cargo build --release --target i686-pc-windows-msvc` (the
game's d3d9.dll, as before) and `--target x86_64-pc-windows-msvc`, copied to the game folder as
**bullygfx64.dll**: bullyre_x64 loads that for the renderer's "D3D9.DLL" when it's there
(the 32-bit d3d9.dll stays for the original game). First run 2026-11-01: the four-teleport
tour, 150 s, the replay renderer on (Vulkan), the UPDT mod in, ~40 fps, the process at 2.45 GB
(past the 32-bit game's wall), pictures right.

What's different there, all of it width-neutral or `cfg`'d so the 32-bit build is unchanged:
- **The game keeps 4-byte pointers** (bullyre's data layout is the original's). So what we
  hand it has to sit below 2 GB: lowmem.rs gets memory from the exe's low heap (its exports
  bullyre_low_alloc / bullyre_low_pages) for the wrapped device, the renderer vtable's thunks
  and jumps put in its import slots (`lowmem::slot_target`). d3d9's own objects come back low
  because bullyre turns d3d9.dll's heap imports to its low heap; `win::obj32` narrows a COM
  pointer to the u32 the capture keeps and logs one that doesn't fit.
- **COM calls**: every method type takes `this` as usize, interface out-parameters are
  `*mut usize` (narrowed after), D3DLOCKED_RECT is `win::LockedRect` (pitch, then the pointer:
  +8 on x64). Vtable slots of D3D's objects are patched 8 bytes wide.
- **Thunks** (hook.rs): x64 code that keeps rcx, rdx, r8, r9 in their home slots and xmm0-3,
  calls `stats::record64`, which builds the 32-bit frame `record` reads (renderer: `this` in
  the ecx slot, arguments from 9; device: `this` at 9) and writes what record changes (a
  buffer's usage, a draw's count) back to its register or stack slot.
- **The game's functions** are x64 code at their old addresses (bullyre's stubs): thiscall
  calls are `extern "C"` with `this` first there. LoadCdDirectory is hooked at its stub (the
  stub's jump pointed at ours; ours calls the body the stub went to).
- crash.rs reads the x64 CONTEXT (Rip 0xF8, Rsp 0x98).
- `stats::enabled()` takes bullyre_x64.exe as well as Bully_unpacked.exe.

Checked 2026-11-01 with bullyre_x64 `--stress-high` (everything that isn't the game's data
above 4 GB): no D3D object came back high. With the UPDT mod (its own .dds files) the 64-bit
game needed three bullyre fixes (its fopen through the game's CreateFileA slot, where our
hook is; d3d9's large blocks high; a missing NiCriticalSection::Unlock): then ~29 fps at the
school grounds against ~34 in the 32-bit game, the process at 4.6 GB.

## Testing (no desktop input needed)

- `bullygfx_cmd.txt` next to the game: commands run on the render thread (see
  control.rs): `key` (fed into the DirectInput keyboard, works unfocused), `wait`,
  `tp`, `where`, `clock H M`, `renderer on|off`, `exclusive on|off`,
  `shot old|new <file>` / `shot both <name>` (the same frame from the D3D9 back buffer
  and the replay, so never a screen grab), `frameinfo`, `dumpdraw N`, `samplers`,
  `replaylimit N`, `nocull`. Output goes to
  `bullygfx_log.txt`; wgpu's warnings and errors go there too.

## Game bugs found on the way (fixed in bullymenu)

- Focus loss takes the streaming mutex (0x405EEE, WaitForSingleObject [0x20DCDAC]):
  fine when the game minimised and paused itself, a permanent freeze once it keeps
  running unfocused (the next streaming wait never ends, and the handler that would
  release it runs on the stuck thread). This was the start-up "crash" at high FPS:
  clicking over to the menu while the game loaded. Patched to skip taking it.
- CdStreamSync (0x73A690) has a lost wake-up race (check sectorsToRead, then set
  locked, then wait forever). Patched to re-check in a loop with 1 ms waits.

## Stages

1. **Measure** (done): `d3d9.dll` proxy. Counts every renderer method and every
   device call with its caller. The device is wrapped rather than vtable-patched,
   because d3d9 swaps the real device's vtable after start-up. Output:
   `bullygfx_stats.txt` in the game folder.
2. **Name the ~25 per-frame methods** (done, table above): confirm each one's arguments by logging them
   (NiGeometry*, NiCamera*, render target groups) and reading the Gamebryo objects
   they pass.
3. **New backend alongside the old one**: our own device and swapchain on the game
   window (D3D12 or Vulkan), with a key to toggle between old and new. It starts by
   drawing clears and frame begin/end.
4. **Take over the three paths**, each one checked in-game against the old picture:
   2D/HUD module → world geometry (RenderShape, the Bully mesh draw, skinned) with
   Gamebryo materials → the post effects → shadows and render-target passes.
5. **Drop D3D9**: once nothing reaches the real device, stop creating it.

## Testing tools

- Teleport: the script function PlayerSetPosXYZArea (0x5D23B0) sets
  `[[0x9E4F4C]+0x8DC] = 1` and calls `0x415880(this = 0xBCC580, float* xyz, float
  heading, int* area)`. That only queues the request, which the main loop carries out,
  so it's safe to call from another thread. If the spot is invalid, the game moves the
  player to the nearest valid one. Known spots in area 0: bus yard (177.77, 4.51, 5.47)
  from mission 1_04; the request (311, 12, 26.06) lands at the infirmary door on
  campus (211.6, -95.1, 8.5).
- Player position: `[[0xC1AEA8]+0x14]+0x30` (x, y, z); area `0xBD1008`.
