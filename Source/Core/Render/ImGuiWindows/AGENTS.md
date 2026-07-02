# AGENTS.md — Source/Core/Render/ImGuiWindows/

The editor's Dear ImGui panels (docking branch, DX12 + SDL2 backends, with ImGuizmo). Each panel
implements `IImGuiWindow` (`RecordImGui()`). They share selection/manipulation state via
`EditorContext`.

All in the `GiiGa` namespace.

## Files

| File | Class(es) | Purpose |
|------|-----------|---------|
| `IImGuiWindow.h` | `struct IImGuiWindow` | Abstract interface all editor panels implement: a single pure-virtual `RecordImGui()` draw entry point. |
| `EditorContext.h` | `struct EditorContext` | Shared editor state passed to all panels: `selectedGameObject` (weak_ptr), `selectedAsset` (shared_ptr<AssetBase>), ImGuizmo `currentOperation_` (translate/rotate/scale) and `currentMode_` (world/local). |
| `EditorViewport.h` | `class EditorViewport : public Viewport, std::enable_shared_from_this<EditorViewport>` | The interactive 3D viewport. Builds the render graph (Shadow -> GBuffer -> GLight -> Forward -> Debug -> PostProcess), owns a spectator-camera GameObject, and overlays an ImGuizmo toolbar + Play/Edit toggle. |
| `ImGuiSceneHierarchy.h` | `class ImGuiSceneHierarchy : public IImGuiWindow` | Tree-view of the World's Levels/GameObject hierarchy: select, add/remove GameObjects, reparent via drag-drop, instantiate/drag prefabs, Save Level / Save-as-Prefab / Unpack Prefab, and delete-key handling. |
| `ImGuiInspector.h` | `class ImGuiInspector : public IImGuiWindow` | The largest panel. Inspects the selected GameObject: name, transform, and every component (Camera, StaticMesh+Material, Point/Directional Light, PyBehaviour with JSON-backed property editing, Collision). Add/remove components. Reusable `DrawVec3Control` widget. |
| `ImGuiContentBrowser.h` | `class ImGuiContentBrowser : public IImGuiWindow`, `LoadEditorIcon`, `enum class CreateAssetState` | File/asset browser showing the asset directory tree with per-type thumbnail icons; folder navigation, drag-drop file import, drag-drop source for asset handles, create folder/material, and create-prefab-from-dropped-GameObject. |
| `ImGuiAssetEditor.h` | `class ImGuiAssetEditor : public IImGuiWindow` | Editor for standalone assets (currently focused on Materials): shading model, blend mode, texture slots, tint, via combos/drag-drop/color editors + a Save button. |

## Notes for agents

- All panels are instantiated and driven by `EditorSwapChainPass` (in [`../RenderPass/`](../RenderPass/)).
- Icons are loaded from `EditorData/` (see `LoadEditorIcon`).
- The inspector's PyBehaviour property editing cooperates with `ScriptPropertyModifications`s
  (see [`../../PySripting/`](../../PySripting/)) and `PyBehaviourSchemeComponent`.
