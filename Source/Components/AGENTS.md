# AGENTS.md — Source/Components/

ECS-style gameplay components that attach to `GameObject`s. All classes derive (directly or
indirectly) from `Component` (`Core/Component.h` -> `IComponent`), and most additionally implement
render interfaces (`IRenderable`, `IUpdateGPUData`) defined under `Core/Render/Interfaces/`.

Everything is in the `GiiGa` namespace and is header-only.

## Files

| File | Class(es) | Purpose |
|------|-----------|---------|
| `TransformComponent.h` | `TransformComponent : Component`, `struct Transform` | Foundational hierarchical transform (local + world matrices, parent/child via `AttachTo`/`Detach`). Fires `OnUpdateTransform` events to dependents. `Transform` holds location (Vector3), rotation (Quaternion), scale, with matrix decomposition and JSON I/O. |
| `CameraComponent.h` | `CameraComponent : Component`, `struct Camera`, `enum CameraType` | Holds projection params (perspective/orthographic) and computes view / projection / view-projection matrices each tick from the owner's transform. `GetSubProjAndDistanceToFar()` supports frustum subdivision for cascaded shadows. |
| `LightComponent.h` | `LightComponent : Component, IRenderable, IUpdateGPUData` | Abstract base for all lights. Provides shared shadow-map infrastructure (depth-stencil resource, DSV/SRV, viewport/scissor) and declares the pure-virtual `InitShadowData()`. |
| `DirectionalLightComponent.h` | `DirectionalLightComponent : LightComponent`, `DirectionLightShaderResource`, `struct DirectionLightData` | Directional (sun) light with **4-cascade** shadow mapping; computes per-cascade view/proj from the camera frustum and uploads direction/color/intensity to the GPU. |
| `PointLightComponent.h` | `PointLightComponent : LightComponent`, `PointLightShaderResource`, `struct PointLightData` | Omnidirectional point light (color, radius, intensity, falloff) rendered with a sphere mesh. Radius driven by owner transform scale. Shadows not yet implemented (`InitShadowData()` empty). |
| `StaticMeshComponent.h` | `StaticMeshComponent : Component, IRenderable, IUpdateGPUData` | Renders a static mesh + material. Manages mesh/material asset handles, registers with the visibility (octree culling) system, uploads per-object GPU data, supports prefab serialization/diffing. |
| `CollisionComponent.h` | `CollisionComponent : IRenderable, IUpdateGPUData, ICollision`, `CollisionShaderResource` | Physics collision/trigger component. Builds a debug wireframe mesh (cube/sphere), registers with Jolt, handles contact events (begin/persist/end overlap), exposes force/impulse/velocity APIs. |
| `SpectatorMovementComponent.h` | `SpectatorMovementComponent : Component` | First-person "fly-camera" movement reading WASDQE + mouse, with wheel-adjustable speed and mouse sensitivity. Toggleable via `active_`. |
| `PyBehaviourSchemeComponent.h` | `PyBehaviourSchemeComponent : Component` | Authoring/"scheme" component that references a `ScriptAsset`. On `BeginPlay` it instantiates the actual `PyBehaviourTrampoline`, applies serialized property values, then removes/replaces itself. Bridges editor-stored script refs to runtime Python behaviour components. |
| `PyBehaviourTrampoline.h` | `PyBehaviourTrampoline : Component, pybind11::trampoline_self_life_support` | C++ trampoline that forwards virtual lifecycle/collision callbacks (`Init`, `BeginPlay`, `Tick`, `OnBeginOverlap`, `OnOverlapping`, `OnEndOverlap`) into Python overrides via `PYBIND11_OVERRIDE`. `SetProperties()` injects deserialized values (handling GO/Component refs by UUID). |
| `ConsoleComponent.h` | `ConsoleComponent final : Component` | Minimal test/example component printing an accumulated delta-time counter to `std::cout` each tick; demonstrates the `Component` lifecycle/serialization interface. |

## Relationships

- **Inheritance backbone:** `Component` (with rendering components also implementing `IRenderable`
  and/or `IUpdateGPUData`).
- **Lights:** `LightComponent` (abstract) <- `DirectionalLightComponent`, `PointLightComponent`.
- **Python:** `PyBehaviourSchemeComponent` spawns a `PyBehaviourTrampoline` from a `ScriptAsset`.
- **Spatial dependency:** most components rely on `TransformComponent` for world matrices.
