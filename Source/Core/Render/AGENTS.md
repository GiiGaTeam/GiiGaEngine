# AGENTS.md — Source/Core/Render/

The renderer, built on **NVRHI** (DX12 backend). A deferred-rendering pipeline driven by a
`RenderGraph` of `RenderPass`es, an octree visibility/culling system, and editor ImGui windows.
NVRHI owns descriptor heaps, upload staging, command-allocator recycling, resource-state tracking
(automatic barriers), and resource lifetime (ref-counted + deferred), so the previous hand-written
D3D12 machinery for those concerns is gone.

All in the `GiiGa` namespace.

## Device, context & memory

| File | Class(es) | Purpose |
|------|-----------|---------|
| `RenderDevice.h` | `class RenderDevice : public IRenderDevice`, `NvrhiMessageCallback` | Owns the native `ID3D12Device` + graphics queue (still needed for the DXGI swap chain and `imgui_impl_dx12`) and the `nvrhi::DeviceHandle` created on top (`Device()`), with the validation layer enabled in Debug. NVRHI owns all descriptor heaps / upload staging. |
| `RenderContext.h` | `class RenderContext : public IRenderContext` | Frame-recording context wrapping one `nvrhi::CommandListHandle`: `OpenFrame`/`CloseAndExecute`, `GetCommandList`, debug markers. Barriers/uploads are handled by NVRHI (automatic state tracking + `writeBuffer`). |
| `DirectXUtils.h` | free fns / functors | `ThrowIfFailed`, COM deleters for the native D3D12 objects still created directly (device, queue, swap chain, ImGui). |

## Resources & views

| File | Class(es) | Purpose |
|------|-----------|---------|
| `GBuffer.h` | `class GBuffer`, `enum GBufferOrder` | Deferred G-buffer: 4 RGBA32F colour `nvrhi::TextureHandle`s (LightAccumulation, Diffuse, Material, NormalWS) + a D24S8 depth texture, plus `FramebufferHandle`s (full, LightAccumulation-only, depth-only). Clears via `clearTextureFloat`/`clearDepthStencilTexture`. |
| `Material.h` | `class Material : AssetBase, IUpdateGPUData`, enums `TexturesOrder`/`TexturesIndex` | Renderable material: tint/scalar props + up to 8 textures + a constant buffer; `AddBindings` contributes the material CB (b2) + texture SRVs; serializes to JSON. |
| `Mesh.h` | `template<class VertexType> Mesh` | Static mesh: `nvrhi::BufferHandle` vertex/index buffers (uploaded via a one-shot command list at load time). `Bind(GraphicsState&)` adds the VB/IB; `GetIndexCount()`. |
| `SkeletalMesh.h` | `class SkeletalMesh`, `Joint`, `Skeleton` | Skinned mesh (uses `VertexBoned`); NVRHI vertex/index buffers. |
| `PerObjectData.h` | `struct WorldMatrices`, `class PerObjectData` | Per-object world+inverse-world constant buffer (b1), backed by a **volatile** NVRHI CB re-written each frame. |
| `VertexTypes.h` | `VertexPosition`, `VertexPNTBT`, `VertexBoned`, `Index16` | Engine vertex layouts with `nvrhi::VertexAttributeDesc[]` (defined out-of-class) + `CreateInputLayout`. |
| `IObjectShaderResource.h` | `struct IObjectShaderResource` | Per-object/material shader-resource bindings: `AddBindings(nvrhi::BindingSetDesc&)` appends items (CBs/textures/samplers) to the set a pass is assembling. |

## Frame pipeline

| File | Class(es) | Purpose |
|------|-----------|---------|
| `SwapChain.h` | `class SwapChain` | DXGI flip-model swap chain; wraps each back-buffer via `createHandleForNativeTexture` → `nvrhi::TextureHandle` + owns a small native RTV heap (for ImGui's `OMSetRenderTargets`). Create/resize/present. |
| `RenderSystem.h` | `class RenderSystem` | Abstract base. Owns `RenderDevice`, `RenderContext`, `SwapChain`, the root `RenderGraph`, and an `IUpdateGPUData` registry. Per-frame tick: open command list → update GPU data → draw graph → execute → present → `runGarbageCollection`. |
| `EditorRenderSystem.h` | `class EditorRenderSystem : public RenderSystem` | Concrete editor render system: wires an `EditorSwapChainPass` + `EditorViewport`(s). |
| `Pipeline.h` | `class Pipeline`, `struct MaskPipeline`, `DefaultRasterState`/`DefaultDepthState` | Fluent builder producing `nvrhi::GraphicsPipelineHandle` from shaders + render state + binding layouts against a target framebuffer. Replaces the old `PSO.h`. |
| `RenderGraph.h` | `class RenderGraph` | Ordered container/executor of `RenderPass`es. |
| `RenderPassViewData.h` | `RenderPassViewMatricies`, `ScreenDimensions`, `RenderPassViewData`, `RTVData` | Per-camera view CBs (view matrices b0, screen dims) + `RTVData` (result texture + framebuffer). |
| `Viewport.h` | `class Viewport` | Abstract renderable viewport: owns a `GBuffer`, a result `RTVData`, a size, and a render graph. |

## Shaders & visibility

| File | Class(es) | Purpose |
|------|-----------|---------|
| `ShaderManager.h` | `class ShaderManager` (nested `Shader`) | Singleton that compiles engine HLSL via `D3DCompileFromFile` and lazily wraps the DXBC into `nvrhi::ShaderHandle`s (`GetShaderByName(device, ...)`). Global shader-path constants. |
| `SceneVisibility.h` | `class SceneVisibility`, `VisibilityEntry` | Octree culling/visibility; groups visible renderables into `DrawPacket`s keyed by `ObjectMask`. Backend-neutral (unchanged by the migration). |
| `ObjectMask.h` | `class ObjectMask`, enums | 64-bit renderable classification (vertex type/blend/shading/fill/light) for bucketing/PSO selection. |

## Subdirectories

| Directory | Contents |
|-----------|----------|
| [`RenderPass/`](RenderPass/AGENTS.md) | The deferred passes: `ShadowPass`, `GBufferPass`, `GLightPass`, `ForwardPass`, `DebugPass`, `PostProcessPass`, the `EditorSwapChainPass` root, and the `RenderPass` base. Each builds `nvrhi` graphics pipelines + binding layouts and, per draw, assembles a `BindingSetDesc` + `GraphicsState`. |
| [`ImGuiWindows/`](ImGuiWindows/AGENTS.md) | Editor ImGui panels + the shared `EditorContext`. ImGui uses `imgui_impl_dx12` driven off NVRHI native handles. |
| [`Interfaces/`](Interfaces/AGENTS.md) | Renderer abstractions: `IRenderable`, `IRenderContext`, `IRenderDevice`, `IUpdateGPUData`. |

## Pipeline order (per frame)

```
ShadowPass -> GBufferPass -> GLightPass -> ForwardPass -> DebugPass -> PostProcessPass -> EditorSwapChainPass
```

The `EditorSwapChainPass` is the editor root: it owns the dockspace, instantiates all ImGui windows,
ticks visibility, executes the `EditorViewport`(s), and renders ImGui draw data onto the swap-chain
back-buffer (via the native D3D12 command list obtained from the NVRHI command list).
