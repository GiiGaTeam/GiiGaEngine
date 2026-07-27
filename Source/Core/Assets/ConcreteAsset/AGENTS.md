# AGENTS.md — Source/Core/Assets/ConcreteAsset/

Concrete `AssetBase` subclasses — the runtime in-memory representation of each asset category,
produced by the loaders in [`../../AssetLoaders/`](../../AssetLoaders/AGENTS.md) and cached by
[`../ResourceManager.h`](../ResourceManager.h).

All in the `GiiGa` namespace.

## Files

| File | Class | Purpose |
|------|-------|---------|
| `TextureAsset.h` | `class TextureAsset : public AssetBase` | GPU-resident texture: an `nvrhi::TextureHandle` wrapped (via `WrapNativeTexture`) around the native resource loaded by DirectXTK12, which it also keeps alive. Reports `AssetType::Texture2D`; non-copyable. |
| `MeshAsset.h` | `template <typename VertexType> class MeshAsset : public AssetBase, public Mesh<VertexType>` | Templated renderable mesh turned into an engine asset. Constructed either fully (vertices, indices, AABB bound to a render context/device) or as an empty "dummy" placeholder. Reports `AssetType::Mesh`; non-copyable. |
| `LevelAsset.h` | `class LevelAsset : public AssetBase` | Loaded level asset holding its raw `Json::Value`. Exposes level name getter/setter; reports `AssetType::Level`. Friends `Level` and `LevelAssetLoader` so they can manipulate its private JSON. |
| `PrefabAsset.h` | `class PrefabAsset final : public AssetBase, std::enable_shared_from_this<PrefabAsset>` | Reusable prefab: a tree of GameObjects rooted at `root`. JSON serialization (`ToJson`), instantiation into the world via `GameObject::Clone` with UUID remap + removed-component filtering, and prefab-instance-with-overrides serialization (`GameObjectAsPrefabInstance`). Destroys its root on destruction. Reports `AssetType::Prefab`. |

## Relationship to loaders

| Asset type | Produced by |
|-----------|-------------|
| `TextureAsset` | `DDSAssetLoader`, `ImageAssetLoader` |
| `MeshAsset<T>` | `MeshAssetLoader<T>` |
| `LevelAsset` | `LevelAssetLoader` |
| `PrefabAsset` | `PrefabAssetLoader` |
| `ScriptAsset` (in `../../PySripting/`) | `ScriptAssetLoader` |
