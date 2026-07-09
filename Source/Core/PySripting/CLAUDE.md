# AGENTS.md — Source/Core/PySripting/

> Note: the directory name `PySripting` is an intentional (pre-existing) typo of "PyScripting".
> Keep the path as-is; do not rename without updating all include sites.

The Python scripting subsystem: an embedded interpreter (pybind11), the `GiiGaPy` C++->Python
binding module, and support for loading user-authored Python behaviour scripts as engine assets and
persisting their properties.

All in the `GiiGa` namespace.

## Files

| File | Class(es) | Purpose |
|------|-----------|---------|
| `py_module.h` | `PYBIND11_EMBEDDED_MODULE(GiiGaPy, m)` | The central binding registration. Builds the entire `GiiGaPy` module exposing the engine's C++ API to Python: `Uuid`, `JsonValue`, `RenderSystem`, `MeshAsset`, `Material`, `AssetHandle`, `ResourceManager`, `Engine`, `Input`, `Vector3`, `Transform`, `SpawnParameters`, `GameObject`, `Component` (with `PyBehaviourTrampoline`), `TransformComponent`, `CameraComponent`, `CollideInfo`/`CollisionData`/`ICollision`/`CollisionComponent`, `ShapeCastResult`, `StaticMeshComponent`, `PointLightComponent`, enums (`MouseButton`, `KeyCode`, `EMotionType`, `Layer`), free functions (`Vector3ToJson/FromJson`, `ShapeCast`), and a `GOAP` submodule. Uses `pybind11::classh` (smart-holder) for types subclassable from Python. |
| `ScriptSystem.h` | `class ScriptSystem` | Owns the embedded Python interpreter lifecycle (`initialize_interpreter` on construction, `finalize_interpreter` on destruction). Configures `sys.path` (project `Assets` + `EditorData/ScriptHelpers`), optionally generates `GiiGaPy.pyi` via an `sgen` module, and classifies Python types as reference types (Component/GameObject backed by Uuid) vs. value types. Exposes `GetReferenceTypeHolder`, `IsTypeReference`/`IsTypeGameObject`/`IsTypeComponent`. |
| `ScriptAsset.h` | `class ScriptAsset : public AssetBase` | Asset wrapper for a user-authored Python behaviour script. Holds the loaded pybind11 module + the user class name. `CreateBehaviourComponent(Uuid)` instantiates the Python class and casts to `shared_ptr<PyBehaviourTrampoline>`; `GetPropertyAnnotaions()` introspects `__annotations__` into a `name -> PyProperty` map. Reports `AssetType::Behaviour`. |
| `ScriptHelpers.h` | `namespace ScriptHelpers` | Thin C++ utilities delegating to a companion Python helper module (`GiiGaPyHelpers`) for Python introspection/serialization: `GetComponentSubclassNameInModule`, `IsEqOrSubClass`, `TypeIsSubClassComponent`, `EncodeToJSONValue`, `GetTypeWithNameInModule`, `GetBuiltinType`. |
| `ScriptPropertyModifications.h` | `struct PyProperty`, `class ScriptPropertyModifications` | Persistence layer for Python script component properties. `PyProperty` holds a `script_type` + `value_or_holder` and deserializes from JSON (`Set`). `ScriptPropertyModifications` stores a per-property map, loads/saves JSON, merges with fresh annotations (so edits survive script reload), and provides an `Iterator` + `begin()`/`end()`. |

## How it fits together

```
ScriptSystem (interpreter)  --owns-->  py_module.h (GiiGaPy bindings)  <--used by--  user .py scripts
        |
ScriptAssetLoader (../AssetLoaders) --produces--> ScriptAsset --CreateBehaviourComponent--> PyBehaviourTrampoline
                                                              (../../../Components/PyBehaviourTrampoline.h)
ScriptPropertyModifications <--> PyBehaviourSchemeComponent (../../../Components/PyBehaviourSchemeComponent.h)
```

- `ScriptSystem` is constructed by `Engine` and owned as a singleton (`Engine::ScriptSystem()`).
- Python classes subclass `GiiGaPy.Component`; the C++ `PyBehaviourTrampoline` forwards virtual
  callbacks into Python via `PYBIND11_OVERRIDE`.
- The `GOAP` submodule of `GiiGaPy` exposes the planner in [`../cppGOAP/`](../cppGOAP/).
- Companion `.pyi` stubs live in `TemplateProject/Assets/GiiGaPy/`.
