# AGENTS.md — Source/Core/AssetLoaders/

Concrete `AssetLoader` subclasses (`Source/Core/Assets/AssetLoader.h`). Each loader binds to a
file-extension regex and a hardcoded loader UUID `id_`, implementing the
`Preprocess` / `Load` / `Save` / `Update` lifecycle for a specific asset format. The loaders are
registered into the asset database and dispatched on demand by `ResourceManager`.

All classes are in the `GiiGa` namespace.

## Files

| File | Class | Purpose |
|------|-------|---------|
| `MeshAssetLoader.h` | `template <typename VertexType> class MeshAssetLoader : public AssetLoader` | Template loader for 3D models (`.fbx`, `.obj`, `.gltf`) via **Assimp**. `Preprocess` imports the scene, recursively builds a `GameObject` hierarchy with `StaticMeshComponent`s, auto-creates derived `.mat`/texture assets, and emits a `.prefab`. `Load` extracts a specific subresource mesh (vertices/indices/AABB) into a `MeshAsset<VertexType>`. |
| `MaterialLoader.h` | `class MaterialLoader : public AssetLoader` | Loads `.mat` materials from JSON: resolves each referenced texture's `AssetHandle` via `ResourceManager`, reads shading/blend properties, assembles texture-component mappings. Save delegates to `Material::SaveToAbsolutePath`. |
| `DDSAssetLoader.h` | `class DDSAssetLoader : public AssetLoader` | Loads `.dds` textures via DirectX Toolkit 12's `CreateDDSTextureFromFile` (with a `ResourceUploadBatch`) into a `TextureAsset`. Saving unsupported. |
| `ImageAssetLoader.h` | `class ImageAssetLoader : public AssetLoader` | Loads standard images (`.png`, `.jpg`, `.jpeg`) via DirectX Toolkit 12's WIC loader (`CreateWICTextureFromFile`) into a `TextureAsset`. Saving unsupported. |
| `LevelAssetLoader.h` | `class LevelAssetLoader : public AssetLoader` | Reads/writes `.level` files as JSON. `Load` parses the JSON (JsonCpp) into a `LevelAsset`; `Save` serializes the `LevelAsset`'s stored JSON back to disk. |
| `PrefabAssetLoader.h` | `class PrefabAssetLoader : public AssetLoader` | Loads `.prefab` JSON: deserializes each `GameObjects` entry, recreates components via `CreateComponentsForGameObject`, remaps in-prefab UUIDs to world UUIDs, returns a `PrefabAsset` rooted at the designated root GameObject. Save serializes the hierarchy back to JSON. |
| `ScriptAssetLoader.h` | `class ScriptAssetLoader : public AssetLoader` | Imports Python `.py` scripts as `Behaviour` assets via pybind11: converts the relative path to a dotted module name, imports it, extracts the `Component` subclass name via `ScriptHelpers`, wraps it in a `ScriptAsset`. `Update` hot-reloads the module and fires `OnUpdate`. |

## How they fit together

- Loaders are **registered** into `BaseAssetDatabase`/`EditorAssetDatabase` (see
  [`../Assets/AGENTS.md`](../Assets/AGENTS.md)) and **dispatched** by `ResourceManager.GetAsset<T>`.
- Loaders produce **concrete asset types** in
  [`../Assets/ConcreteAsset/`](../Assets/ConcreteAsset/AGENTS.md): `TextureAsset`, `MeshAsset<T>`,
  `LevelAsset`, `PrefabAsset`, and `ScriptAsset` (in `../PySripting/`).
- The `MeshAssetLoader` is the most complex: a single model import fans out into multiple
  sub-meshes, auto-generated materials/textures, and an enclosing prefab.
