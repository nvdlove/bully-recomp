# Bully Recomp

A 64-bit full rewrite of Bully: Scholarship Edition, including its Gamebryo engine layer.

🌐 **Website:** [https://scholarshipedition.com/](https://scholarshipedition.com/)

---

## Overview

This repository houses the native 64-bit source re-implementation and tooling for **Bully: Scholarship Edition**:

- **`bullyre/bullyre/`** — Native C++ 64-bit re-implementation of the game engine, Gamebryo runtime (`NiCamera`, `NiStream`, `NiMaterial`, `NiShadowManager`), streaming, and game subsystems. Features function hooking (MinHook) and side-by-side shadow parity execution against the original 32-bit game. See detailed documentation in [`bullyre/bullyre/README.md`](bullyre/bullyre/README.md).
- **`bullyre/bullygfx/`** — Rust graphics layer and D3D proxy (`bullygfx64.dll`) providing custom rendering pipelines and replay facilities.
- **`bullyre/bullymenu/`** — Rust desktop launcher (`tao` + `wry`) responsible for orchestrating, patching, and launching the remaster payload.
- **`bullyre/bully_unpacked.7z`** — Reference unpacked PE sections for binary alignment and verification.

---

## Technical Architecture

```
┌──────────────────────────────────────────────┐
│                  bullymenu                   │  (Rust: launcher & bundle unpacker)
└──────────────────────┬───────────────────────┘
                       │ launches
┌──────────────────────▼───────────────────────┐
│              bullyre_x64.exe                 │  (Native 64-bit Game Core)
│  ┌──────────────────────┬─────────────────┐  │
│  │   Gamebryo Engine    │  Game Logic     │  │  (NiStream, NiCamera, Renderers)
│  └──────────────────────┴─────────────────┘  │
└──────────────────────┬───────────────────────┘
                       │ hooks / renders via
┌──────────────────────▼───────────────────────┐
│                 bullygfx                     │  (Rust D3D proxy: bullygfx64.dll)
└──────────────────────────────────────────────┘
```

For complete build instructions, function registry specs, and reverse engineering progress, refer to [`bullyre/bullyre/README.md`](bullyre/bullyre/README.md).