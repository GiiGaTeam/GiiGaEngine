# AGENTS.md — Source/Core/Render/RenderPass/

The deferred render passes, each a `RenderPass` subclass drawn in sequence by a `RenderGraph`. The
per-frame order is:

```
ShadowPass -> GBufferPass -> GLightPass -> ForwardPass -> DebugPass -> PostProcessPass -> EditorSwapChainPass
```

`EditorSwapChainPass` is the editor's root pass (owns the dockspace + ImGui windows and executes the
`EditorViewport`s).

All in the `GiiGa` namespace.

## Files

| File | Class | Purpose |
|------|-------|---------|
| `RenderPass.h` | `class RenderPass` | Abstract base for all passes: declares the `Draw(context)` contract; provides a protected `GetPsoFromMapByMask` helper to select the matching PSO for a given `ObjectMask` from a mask-keyed map. |
| `ShadowPass.h` | `class ShadowPass : public RenderPass` | **Cascaded** shadow-map generation for directional lights: vertex + geometry shader (`CascadeShadowShader` / `CascadeShadowGeomShader`) with depth bias, rendering shadow-casting opaque/masked DefaultLit geometry from each light's cascade views into the light's shadow DSV; restores viewport/scissor afterward. |
| `GBufferPass.h` | `class GBufferPass : public RenderPass` | **Geometry** pass of the deferred pipeline: renders DefaultLit opaque solid geometry (+ a wireframe variant) into the GBuffer RTs (LightAccumulation, Diffuse, Material, NormalWS) with depth/stencil; binds base color/metallic/specular/roughness/anisotropy/emissive/normal textures. |
| `GLightPass.h` | `class GLightPass : public RenderPass` | **Deferred lighting** pass using a two-phase stencil-volume algorithm: an "unmark" pass decrements stencil for light volumes, then point-light and directional-light "shade" PSOs (additive blending) read the GBuffer SRVs and accumulate lighting into LightAccumulation; directional lights also bind cascade-shadow SRVs. |
| `ForwardPass.h` | `class ForwardPass : public RenderPass` | **Forward** pass for unlit-opaque and translucent/masked (blended) geometry — used alongside the deferred pipeline for non-lit materials; builds two PSOs, binds material/emissive descriptors. |
| `DebugPass.h` | `class DebugPass : public RenderPass` | **Wireframe debug** pass: draws all PNTBT geometry (any shading/fill mode, no directional lights) with a `BufferWireframeShader` pixel shader, no culling, after frustum extraction. |
| `PostProcessPass.h` | `class PostProcessPass : public RenderPass` | Final **post-processing** pass: gamma-corrects by drawing a full-screen quad sampling the GBuffer's LightAccumulation SRV into the viewport's RTV (later displayed via `ImGui::Image`). |
| `EditorSwapChainPass.h` | `class EditorSwapChainPass final : public RenderPass` | The **editor root** pass: initializes ImGui (DX12 + SDL2 backends), owns the dockspace, instantiates all editor ImGui windows (Hierarchy/Inspector/AssetEditor/ContentBrowser — see [`../ImGuiWindows/`](../ImGuiWindows/)), ticks scene visibility, executes the `EditorViewport`(s), and renders ImGui draw data onto the swap-chain back-buffer. `friend class EditorRenderSystem`. |

## Notes for agents

- Passes are added to a `RenderGraph` ([`../RenderGraph.h`](../RenderGraph.h)) which calls `Draw` on
  each in order; the `EditorViewport` ([`../ImGuiWindows/EditorViewport.h`](../ImGuiWindows/EditorViewport.h))
  assembles the pre-composition graph.
- Each pass typically owns a `mask_to_pso` (or PSO members) keyed by `ObjectMask`
  ([`../ObjectMask.h`](../ObjectMask.h)) and a filter `ObjectMask` selecting which renderables it
  processes (collected by `SceneVisibility`).
- Pipelines + binding layouts are built via the `PSO` builder in [`../PSO.h`](../PSO.h) using
  `nvrhi::ShaderHandle`s from [`../ShaderManager.h`](../ShaderManager.h) and the `/Shaders` HLSL
  sources. At draw time a pass assembles an `nvrhi::BindingSetDesc` (pass items + per-object CB +
  filtered `IObjectShaderResource` items), gets a cached binding set from the `RenderContext`, and
  sets pipeline/framebuffer/viewport on the context before calling each renderable's `Draw`.
