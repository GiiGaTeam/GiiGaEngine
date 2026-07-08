# AGENTS.md — Source/Core/

The engine core: the application/engine layer, the entity-component world model, windowing & input,
UUID/variant/event utilities, and project/level/prefab management. Rendering, assets, physics,
scripting, and GOAP each have their own subdirectories (linked below).

All files are header-only and under the `GiiGa` namespace.

## Engine & application layer

| File | Class(es) | Purpose |
|------|-----------|---------|
| `Engine.h` | `class Engine` | Abstract base engine owning core subsystems (window, render system, resource manager, asset database, input, script system). Singleton via `Instance()`; `Initialize`/`DeInitialize` lifecycle; declares pure-virtual `Run()`. |
| `EditorEngine.h` | `class EditorEngine : public Engine` | Concrete engine for **editor mode**. Runs the main loop, initializes `EditorAssetDatabase` with a file watcher and all loaders/savers, ticks World and Physics, and processes the asset hot-reload update queue. |
| `GameEngine.h` | `class GameEngine : public Engine` | Concrete engine for standalone/**runtime** game builds. Simpler loop ticking active levels' root GameObjects and render system (partially stubbed). |
| `Project.h` | `class Project` | Loads/saves project settings from a `project.giga` JSON file (notably the default-level UUID). `CreateOrOpen` optionally seeds from a template folder. |
| `Timer.h` | `class Timer` (static) | `std::chrono`-based timing: tracks start time, computes per-frame delta time (with time-scale), exposes `GetRunTime()`/`GetDeltaTime()`. |

## Entity-component world model

| File | Class(es) | Purpose |
|------|-----------|---------|
| `World.h` | `class World : public WorldQuery`, `enum class WorldState` | Top-level scene manager. Holds the list of `Level`s (PersistentLevel first), drives per-frame init/begin-play queues and level ticking, manages Edit/Play state transitions (physics begin/end play + level save/restore), and offers static level-loading helpers. |
| `Level.h` | `class Level : public ILevelRootGameObjects` | A scene level. JSON (de)serialization of root GameObjects, active state, BeginPlay/EndPlay, builds from `LevelAsset`, creates prefab instances with applied modifications, can produce/replace its `LevelAsset`. |
| `GameObject.h` | `class GameObject final : public IGameObject`, `struct SpawnParameters` | Central `final` entity class. Parent/child transform hierarchy, component list, level-root attachment, JSON (de)serialization, and extensive prefab-instance logic (cloning, modification diffing/applying, removed-component tracking). |
| `IGameObject.h` | `struct IGameObject : public ITickable` | Pure abstract interface for game objects: UUID access, JSON serialization, component removal, destruction, collision-overlap forwarding. |
| `Component.h` | `class Component : public IComponent` | Base concrete component. UUID ownership, JSON (de)serialization, world registration, owner handling, prefab cloning, default no-op collision callbacks. |
| `IComponent.h` | `struct IComponent : public ITickable` | Pure abstract component interface: lifecycle (Init/BeginPlay/EndPlay/Destroy), JSON serialization, cloning, prefab-modification query/apply, UUID access, owner binding, overlap callbacks. |
| `IGameObject.h` / `IComponent.h` | — | (Interfaces — see above.) |
| `ITickable.h` | `class ITickable`, `enum TickType` | Minimal per-frame update interface: pure-virtual `Tick(float dt)` and a `TickType` toggle. Base of `IComponent`/`IGameObject`. |
| `ICollision.h` | `struct ICollision : public TransformComponent`, `struct CollideInfo`, enums `EMotionType`/`ColliderType`/`Layer`, `struct CollisionData` | Abstract interface (extending `TransformComponent`) for physics/collision components: contact callbacks, force/impulse/velocity, motion/collider types, collision layers, and the `CollideInfo` payload. |
| `ILevelRootGameObjects.h` | `class ILevelRootGameObjects` | Abstract base for anything holding a level's root GameObjects (i.e. `Level`). Protected vector + `AddRootGameObject`/`RemoveRootGameObject` helpers. |
| `IWorldQuery.h` | `class WorldQuery` | UUID registry/lookup singleton base: add/remove entities & components, query by UUID or type, enqueue components for init/begin-play, obtain the persistent level. Subclassed by `World`. |
| `CreateComponentsForGameObject.h` | `struct CreateComponentsForGameObject` | Factory helper that deserializes a GameObject's components from JSON by mapping the stored `Type` string to a concrete component type. Two `Create` overloads: level loading (fixed IDs) and prefab instantiation (rolled IDs with UUID remap). |
| `PrefabInstanceModifications.h` | `PrefabInstanceModifications`, `PrefabPropertyModifications`, `PrefabPropertyValue`, `PropertyModificationKey` | Data structures capturing how a prefab instance differs from its source: UUID remap, per-key property modifications, and removed-GO/component sets. JSON (de)serialization. |

## Windowing & input

| File | Class(es) | Purpose |
|------|-----------|---------|
| `Window.h` | `class Window`, enums `KeyCode`/`MouseButton`/`GamepadCode`/`GamepadAxis`/`GamepadStick`, event structs | Wraps an SDL2 window + ImGui integration. `ProcessEvents()` polls SDL, translates scancodes/buttons to engine codes, and dispatches typed events (key/mouse/gamepad/resize/drop-file/quit) via `EventDispatcher`. Exposes the native HWND. |
| `WindowManager.h` | `class WindowManager` | Static factory: initializes SDL (video + game controller), creates a `Window` from `WindowSettings` via `SDL_CreateWindow`. |
| `WindowSettings.h` | `struct WindowSettings` | POD window params: title, width, height (with defaults). |
| `Input.h` | `class Input`, `struct MousePosition`/`MouseDelta`/`MouseWheel` | Static input manager for keyboard/mouse/gamepad. Subscribes to Window events, tracks held/pressed/released states, mouse pos/delta/wheel, analog sticks (with dead-zone); query helpers (`IsKeyHeld`, `IsKeyDown`, `Get2DAxis`, ...). |

## Utilities

| File | Class(es) | Purpose |
|------|-----------|---------|
| `Uuid.h` | `class Uuid` | `stduuid` wrapper: `New`, `Null`, parsing, string conversion, hashing, comparison; `std::hash<GiiGa::Uuid>` specialization. |
| `Variant.h` | `class Variant`, `struct ComponentName` | Discriminated value type over `std::variant` of Empty/bool/int64_t/float/string/ComponentName. Typed optional accessors (`GetBool`, `GetInt`, ...). |
| `EventSystem.h` | `template<class T> EventHandle`, `template<class T> EventDispatcher` | Generic type-safe pub/sub: `Register` returns an `EventHandle`, `Invoke` fans out to all handlers. |
| `DXMathUtils.h` | free functions, `constexpr float Pi` | DirectXMath/SimpleMath <-> Jolt math bridge: deg/rad conversions, `Vector3` JSON serialization, frustum extraction (corners/center/planes), DX<->Jolt vector/quaternion conversions. |
| `unique_any.h` | `class unique_any` | Move-only type-erased container (like `std::any` but non-copyable) via Concept/Model hierarchy. `cast<T>()`, `has_value()`, `type()`, `reset()`. |
| `Logger.h` | `const std::string` log channel names | Named logger identifier constants (easylogging++) per subsystem (`LogWorld`, `LogRendering`, `LogPhysics`, `LogEditor`, `LogPyScripting`, ...). |
| `Misc.h` | `template<class T> T Todo()`, `void Todo()` | Scratch utility: `Todo()` throws `std::logic_error("Not implemented")` to mark unfinished code. *(Also contains a bogus `#define PI 5` — treat as a known wart.)* |
| `DefaultAssetsHandles.h` | `namespace DefaultAssetsHandles` | Well-known fixed `AssetHandle` constants (reserved UUIDs) for engine built-in default textures, materials, and primitive meshes (Cube/Sphere/Quad). |

## Subdirectories

| Directory | Contents |
|-----------|----------|
| [`Assets/`](Assets/AGENTS.md) | Asset framework: `AssetType`/`AssetHandle`/`AssetMeta`, `AssetBase`, `AssetLoader`, databases, `ProjectWatcher`, `ResourceManager`, and concrete asset types. |
| [`AssetLoaders/`](AssetLoaders/AGENTS.md) | Concrete importers/exporters (DDS/PNG textures, Assimp meshes, material, level, prefab, Python script). |
| [`cppGOAP/`](cppGOAP/AGENTS.md) | Goal-Oriented Action Planning AI planner (A*/Dijkstra over `WorldState` + `Action`s). |
| [`Physics/`](Physics/AGENTS.md) | Jolt Physics singleton facade (bodies, layers, contact events, shape casts). |
| [`PySripting/`](PySripting/AGENTS.md) | Embedded Python via pybind11: `GiiGaPy` module bindings, `ScriptSystem`, script asset & property persistence. |
| [`Render/`](Render/AGENTS.md) | NVRHI (D3D12 backend) deferred renderer: device/context, meshes/materials, render graph & passes, editor ImGui windows. |

## Notes for agents

- This is the structural heart of the engine; changes here ripple widely (ticking, UUID registry,
  serialization, prefab system are all interconnected).
- The prefab system (`PrefabInstanceModifications` + `GameObject` + `Component` + `Level`) is the
  most intricate part — read carefully before modifying cloning/diff/apply logic.
- Some files are work-in-progress (`GameEngine.h`, `Misc.h`); follow existing patterns when fixing.
