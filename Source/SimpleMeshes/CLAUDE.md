# AGENTS.md — Source/SimpleMeshes/

Procedural primitive mesh generators. These build geometry on the CPU and upload it as GPU
vertex/index buffers (deriving from `Mesh`).

## Files

| File | Class(es) | Purpose |
|------|-----------|---------|
| `Cone.h` | `class Cone : public Mesh` | **Currently disabled** — the entire file is commented out (`/* ... */`). When active it was a C++20 module (`export module Cone`) generating a procedural cone from an apex, direction, height, and half-angle (16-segment circular base). |

## Notes for agents

- This directory is effectively **dead code** right now. The procedural-mesh approach may be
  revisited, but nothing here is currently compiled.
- If re-enabling, port the module syntax back to a plain header and ensure it builds under the
  header-only convention used by the rest of the engine.
