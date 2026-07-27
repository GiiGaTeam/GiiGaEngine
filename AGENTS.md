# AGENTS.md — GiiGaEngine (repository root)

## Overview

GiiGaEngine is an **educational game engine** written in **C++20**, designed to provide a
Unity/Unreal-like editor experience. It features a deferred-rendering pipeline built on **NVRHI**
(D3D12 backend), an extensible asset system with live file watching, Jolt Physics integration, and
Python scripting (via pybind11) that can mutate and extend C++ engine objects.

The codebase is **header-only** (modules were attempted but abandoned), so nearly all logic lives
in `.h` files that are included directly; `Source/main.cpp` is the only translation unit.

## Tech stack

| Area | Technology |
|------|-----------|
| Language | C++20 (header-only) |
| Build | Visual Studio 2022 (`GiiGaEngine.sln` / `.vcxproj`), MSBuild |
| Package manager | vcpkg (manifest mode, see `vcpkg.json`) |
| Graphics | NVRHI (D3D12 backend) + DirectXTK12 for texture loading |
| Windowing/Input | SDL2 |
| Editor UI | Dear ImGui (docking, DX12 + SDL2 backends) + ImGuizmo |
| Physics | Jolt Physics |
| Asset import | Assimp |
| Serialization | JsonCpp |
| Scripting | Python via pybind11 (embedded `GiiGaPy` module) |
| Logging | easylogging++ (`logging.conf`) |
| UUIDs | stduuid |

## Repository layout

```
.
├── Source/                  # All engine C++ code (header-only)
│   ├── main.cpp             # Program entry point (bootstraps logging, project, EditorEngine)
│   ├── main_gui.h           # Reference ImGui+DX12 sample loop (not the real entry point)
│   ├── Components/          # ECS-style gameplay components (see its AGENTS.md)
│   ├── Core/                # Engine core: world/level/GO, assets, render, physics, scripting
│   │   ├── Assets/          # Asset framework + databases + ResourceManager
│   │   ├── AssetLoaders/    # Concrete importers (mesh, texture, material, level, prefab, script)
│   │   ├── cppGOAP/         # Goal-Oriented Action Planning AI planner
│   │   ├── Physics/         # Jolt Physics system wrapper
│   │   ├── PySripting/      # Python/pybind11 bindings and script asset support
│   │   └── Render/          # NVRHI (D3D12) renderer, render passes, editor ImGui windows
│   └── SimpleMeshes/        # Procedural primitive meshes (currently disabled)
├── Shaders/                 # HLSL shader sources (vertex, pixel, geometry, GBuffer, shadow)
├── ThirdParty/              # Vendored single-header libs (FileWatch, any_shared_ptr)
├── vendor/Octree/           # Git submodule: Octree (attcs/Octree) for scene culling
├── EditorData/              # Editor icons, helper Python scripts for scripting subsystem
├── TemplateProject/         # Starter project (project.giga + Assets + sample scripts)
├── docs/                    # Documentation assets (game demo gifs)
├── vcpkg_custom_overlay/    # Custom vcpkg port overlay (e.g. nvrhi)
├── vcpkg.json               # vcpkg manifest of dependencies
├── GiiGaEngine.sln          # Visual Studio solution
├── GiiGaEngine.vcxproj      # VS project
├── .clang-format            # Code style (Allman braces, 4-space, CamelCase)
└── README.md
```

Build output lands under `x64/` (runtime DLLs/EXE) and `GiiGaEngine/x64/` (intermediate). Both are
gitignored.

## Architecture at a glance

- **Engine bootstrap** (`Source/main.cpp`): initializes logging, opens/creates a `Project`, runs an
  `EditorEngine` (editor mode) or `GameEngine` (runtime), and reports live D3D objects on exit.
- **Entity-Component** (`Source/Core` + `Source/Components`): `GameObject` + `Component` (via
  `IGameObject`/`IComponent` interfaces) live in `World` -> `Level`s, all addressable by `Uuid`
  through the `WorldQuery` registry.
- **Rendering** (`Source/Core/Render`): an NVRHI (D3D12 backend) deferred pipeline (Shadow ->
  GBuffer -> GLight -> Forward -> Debug -> PostProcess -> EditorSwapChain) driven by a
  `RenderGraph` of `RenderPass`es, with ImGuizmo/ImGui editor windows.
- **Assets** (`Source/Core/Assets` + `AssetLoaders`): a `database.json` registry, hot-reload via
  `ProjectWatcher`, and a caching `ResourceManager`. Concrete assets: mesh, texture, material,
  level, prefab, behaviour (script).
- **Scripting** (`Source/Core/PySripting`): embedded Python exposing the `GiiGaPy` module; user
  Python classes extend `Component` via a `PyBehaviourTrampoline`.
- **Physics** (`Source/Core/Physics`): Jolt singleton managing bodies, layers, contact events, and
  shape casts.

## Code style

Enforced via `.clang-format` (BasedOnStyle: OpenAI):
- **Allman** braces, 4-space indentation, no tabs.
- `CamelCase` classes/functions, `camelCase` variables, `UPPER_CASE` constants.
- `NamespaceIndentation: All`.
- `SortIncludes: true` with the priority order: system `<...>` (1), local `.*` (0).

## How to build

Per `README.md`: build with the **vcpkg manifest** (`vcpkg.json`). Python must be installed with a
`PYTHON_PATH` environment variable pointing to its root (needed by the pybind11 scripting
subsystem). Open `GiiGaEngine.sln` in Visual Studio 2022 and build for `x64`.

## Conventions for agents editing this repo

- Prefer editing existing headers; nearly all behavior is header-only — do not introduce `.cpp`
  files unless explicitly required.
- Follow the existing `.clang-format` style (Allman braces, CamelCase, `GiiGa` namespace).
- Everything lives under the `GiiGa` namespace (with `goap::` for the GOAP planner).
- Subdirectories under `Source/` contain their own `AGENTS.md` describing their contents in detail.
