# AGENTS.md — Source/Core/Assets/

The asset-management framework: type enumeration, lightweight asset references/handles, the loader
interface, asset databases (with live filesystem watching), and the caching `ResourceManager`.
Concrete asset types live in [`ConcreteAsset/`](ConcreteAsset/AGENTS.md) and the format-specific
importers live in [`../AssetLoaders/`](../AssetLoaders/AGENTS.md).

All in the `GiiGa` namespace.

## Core types & interfaces

| File | Class(es) | Purpose |
|------|-----------|---------|
| `AssetType.h` | `enum class AssetType`, conversion fns | Enumeration of asset kinds (`Mesh`, `SkeletalMesh`, `Texture2D`, `Level`, `Prefab`, `Behaviour`, `Audio`, `Material`) with bidirectional string conversion (`AssetTypeToString`, `StringToAssetType`, ...). |
| `AssetHandle.h` | `struct AssetHandle`, `std::hash<AssetHandle>` | Lightweight value-type asset reference: a `Uuid` + a `subresource` index (one file can expose multiple assets). JSON (de)serialization, validity checks, equality, and `std::hash` so it can key unordered containers. |
| `AssetMeta.h` | `struct AssetMeta` | Metadata describing a registered asset: `AssetType`, relative file path, owning loader's `Uuid`, display name. JSON serialization (`ToJson`) / validated deserialization (`FromJson`) for the `database.json` registry format. |
| `AssetBase.h` | `class AssetBase` | Abstract base for all assets. Holds the asset's `AssetHandle` id, exposes pure-virtual `GetType()`, and provides `OnDestroy`/`OnUpdate` event dispatchers. Non-copyable, movable. |
| `AssetLoader.h` | `class AssetLoader`, `LoadCallback` | Abstract importer/exporter interface: stores a file-pattern regex, an `AssetType`, a loader `Uuid`; declares the `Preprocess`/`Load`/`Save`/`Update` contract. Provides `LoadAsync` (detached thread), `MatchesPattern`, and type/id/name accessors. |

## Databases & filesystem

| File | Class(es) | Purpose |
|------|-----------|---------|
| `BaseAssetDatabase.h` | `class BaseAssetDatabase`, `concept IsAssetLoader` | Shared asset-registry foundation. Owns `database.json` and three lookup indices (by handle / by path / by type), loader & saver registries, lookup helpers, registry (de)serialization. Loader registration is guarded by the `IsAssetLoader` concept. `friend` of `ResourceManager`. |
| `EditorAssetDatabase.h` | `class EditorAssetDatabase : public BaseAssetDatabase`, `concept IsAssetBase` | Editor-side database. Integrates a `ProjectWatcher` reacting to file add/modify/remove/rename/move events; supports import/create/save/remove/path-update of assets, scans the Assets folder, prunes missing entries, and exposes a thread-safe update queue for hot reload. |
| `RuntimeAssetDatabase.h` | `class RuntimeAssetDatabase : public BaseAssetDatabase` | Stub/placeholder subclass for the runtime (in-game) context — currently empty. |
| `ProjectWatcher.h` | `class ProjectWatcher` | Wraps the `filewatch` library to monitor directories and emit `OnFileAdded`/`OnFileRemoved`/`OnFileModified`/`OnFileRenamed`/`OnFileMoved`. Disambiguates rename vs. move/remove via timestamps (`filewatch` does not distinguish them natively). |
| `ResourceManager.h` | `class ResourceManager` | Central asset cache + loader dispatcher. Holds a `BaseAssetDatabase` and `AssetHandle -> weak_ptr<AssetBase>` map. Templated `GetAsset<T>` returns a cached asset or loads on demand through the matching loader, registering an `OnDestroy` callback for cache eviction. `UpdateAsset` for hot reload. |

## Subdirectory

| Directory | Contents |
|-----------|----------|
| [`ConcreteAsset/`](ConcreteAsset/AGENTS.md) | Concrete `AssetBase` subclasses produced by loaders: `LevelAsset`, `MeshAsset<T>`, `PrefabAsset`, `TextureAsset`. |

## Notes for agents

- The registry is persisted as `database.json` (per `AssetMeta`); never hand-edit it while the
  editor runs — go through `EditorAssetDatabase`.
- Hot reload flows: `ProjectWatcher` event -> `EditorAssetDatabase` update queue ->
  `ResourceManager.UpdateAsset` -> `AssetBase::OnUpdate`.
- All asset references in components/levels are by `AssetHandle` (UUID), not by path.
