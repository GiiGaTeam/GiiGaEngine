# AGENTS.md — Source/

This directory contains **all of the engine's C++ source**. The project is **header-only**
(C++20 modules were attempted and abandoned), so almost every file is a `.h` header included
directly into a single translation unit. The only `.cpp` is the program entry point.

## Files

| File | Purpose |
|------|---------|
| `main.cpp` | Program entry point. Bootstraps logging (easylogging++), opens/creates a `GiiGa::Project`, constructs and runs the `GiiGa::EditorEngine`, and reports live D3D12 objects on exit for leak detection (`DxReportLiveObjects`). Wraps everything in a try/catch. |
| `main_gui.h` | A **reference/sample** ImGui + DirectX 12 + Win32 loop (`int mainG(...)`), rewritten with C++20 module imports. **Not** the real entry point — kept as scaffolding showing how a DX12 device/swap-chain/ImGui loop is set up. |

## Subdirectories

| Directory | Contents |
|-----------|----------|
| [`Components/`](../Components/AGENTS.md) | ECS-style gameplay components: `Transform`, `Camera`, lights, `StaticMesh`, `Collision`, spectator movement, and the Python `PyBehaviour` trampoline/scheme. |
| [`Core/`](../Core/AGENTS.md) | Engine core: `Engine`/`World`/`Level`/`GameObject`, windowing/input, assets, rendering, physics, scripting, and GOAP AI. |
| [`SimpleMeshes/`](../SimpleMeshes/AGENTS.md) | Procedural primitive mesh generators (currently disabled/commented out). |

> Each subdirectory above has its own `AGENTS.md` with a per-file breakdown.

## Notes for agents

- Do **not** add new `.cpp` files; extend existing headers instead.
- `main.cpp` is the only translation unit — adding headers here effectively adds them to the build.
- All code is under the `GiiGa` namespace.
