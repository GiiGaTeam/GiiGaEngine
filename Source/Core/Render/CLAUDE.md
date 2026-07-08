# AGENTS.md — Source/Core/Render/

The renderer: **NVRHI** (D3D12 backend) drives all GPU work — resources, barriers, descriptors,
pipelines and draw submission. A deferred-rendering pipeline runs as a `RenderGraph` of
`RenderPass`es, with an octree visibility/culling system and editor ImGui windows. Only a thin
native-D3D12 edge remains for what NVRHI does not cover: device/queue creation, the DXGI swap
chain, DirectXTK12 texture loading, and the Dear ImGui DX12 backend.

All in the `GiiGa` namespace.

## Device & context

| File | Class(es) | Purpose |
|------|-----------|---------|
| `RenderDevice.h` | `class RenderDevice : public IRenderDevice`, `struct ImGuiSrvHandle` | Creates the native `ID3D12Device` + graphics queue and wraps them into an `nvrhi::DeviceHandle` (validation layer in debug). Also owns the dedicated shader-visible SRV heap used by Dear ImGui (`AllocateImGuiDescriptor`/`FreeImGuiDescriptor(Deferred)`, `CreateImGuiTextureSrv` produces `ImTextureID`s for nvrhi textures). |
| `NvrhiUtils.h` | `NvrhiMessageCallback`, free fns | NVRHI diagnostics -> engine log; `DxgiToNvrhiFormat`; `WrapNativeTexture` (native resource -> `nvrhi::TextureHandle`, used for DirectXTK-loaded textures); `CreateConstantBuffer`/`CreateStructuredBuffer` helpers (GPU-local, `keepInitialState`, updated via `writeBuffer`). |
| `RenderContext.h` | `class RenderContext : public IRenderContext` | The frame-recording context: owns the graphics `nvrhi::CommandListHandle`, frame pacing (swap-chain latency waitable), a deferred-upload queue (`WriteBufferDeffered`, flushed at `StartFrame`; thread-safe for async asset loads), a binding-set cache (`GetOrCreateBindingSet`, canonical item order), and the current graphics state (pipeline/framebuffer/viewport/binding set) that `DrawIndexed` assembles into an `nvrhi::GraphicsState` per draw. `EndFrame` executes the command list + runs NVRHI garbage collection. |
| `SwapChain.h` | `class SwapChain` | DXGI flip-model swap chain with frame-latency waitable object. Back buffers stay **native** (small dedicated RTV heap, manual PRESENT<->RENDER_TARGET barriers in `BeginRender`/`EndRender`) because only the ImGui composition pass touches them, via raw D3D12 commands. |
| `DirectXUtils.h` | `ThrowIfFailed`, `DXDeleter` | The only native-D3D12 utilities left: HRESULT check + COM deleter for `std::shared_ptr`. |
| `RenderSystem.h` | `class RenderSystem` | Abstract base of the rendering system. Owns `RenderDevice`, `RenderContext`, `SwapChain`, the root `RenderGraph`, and an `IUpdateGPUData` registry. Drives the per-frame tick (stale cleanup -> start frame -> update GPU data -> draw graph -> end frame -> present); initializes `ShaderManager` with the nvrhi device. |
| `EditorRenderSystem.h` | `class EditorRenderSystem : public RenderSystem` | Concrete `RenderSystem` for the editor: wires up an `EditorSwapChainPass` and `EditorViewport`s as the render-graph root. |
| `RenderSystemSettings.h` | `struct RenderSystemSettings` | Static renderer-wide constants: frames-in-flight, back-buffer count, ImGui SRV heap size. |

## Resources & frame data

| File | Class(es) | Purpose |
|------|-----------|---------|
| `Mesh.h` | `template<class VertexType> Mesh` | Static mesh: nvrhi vertex/index buffers filled via the deferred-upload queue; `Draw` issues `RenderContext::DrawIndexed` against the current pass state; AABB + object mask. |
| `SkeletalMesh.h` | `class SkeletalMesh`, `struct Joint`, `Skeleton` | Skinned/animated mesh (uses `VertexBoned`): same nvrhi buffer scheme as `Mesh`; defines `Joint`/`Skeleton`. |
| `PerObjectData.h` | `struct WorldMatrices`, `class PerObjectData` | Per-object constant buffer (world + inverse-world matrices from a `TransformComponent`): one GPU-local nvrhi buffer per object, written with `writeBuffer` (deferred at creation, immediate per frame). |
| `GBuffer.h` | `class GBuffer`, `enum GBufferOrder` | The geometry-buffer set: four RGBA32F color targets (LightAccumulation, Diffuse, Material, NormalWS) + D24S8 depth as nvrhi textures, exposed as three cached framebuffers (geometry: 4 RT + depth; accum: 1 RT + depth; depth-only) plus clear helpers. |
| `Material.h` | `class Material : AssetBase, IUpdateGPUData`, `MaterialShaderResource`, enums `TexturesOrder`/`TexturesIndex` | Renderable material: tint/scalar props in a GPU-local constant buffer, up to 8 `TextureAsset` slots, blend/shading modes. `MaterialShaderResource` returns `nvrhi::BindingSetItem`s (CB at b2, textures at t0..t7). Serializes to JSON. |
| `RenderPassViewData.h` | `RenderPassViewMatricies`, `ScreenDimensions`, `RenderPassViewData`, `RTVData` | Per-camera/view constant-buffer structs and a container with camera info + the view/screen-dims nvrhi buffers; `RTVData` bundles the viewport result texture with its framebuffer. |
| `Viewport.h` | `class Viewport` | Abstract renderable viewport: owns a `GBuffer`, a result texture + framebuffer (`RTVData`), a size, and a render graph; pure-virtual camera binding + per-frame execution. |
| `IObjectShaderResource.h` | `struct IObjectShaderResource` | Abstract per-object/per-material bindings: returns `nvrhi::BindingSetItem`s at canonical slots (material CB b2 + textures t0..; light CB b3); passes filter them against each pipeline's binding layout. |

## Shaders, pipelines & visibility

| File | Class(es) | Purpose |
|------|-----------|---------|
| `ShaderManager.h` | `class ShaderManager` (nested `Shader`), `enum ShaderTypes` | Singleton that pre-compiles the fixed set of engine HLSL shaders with FXC (`D3DCompileFromFile`, SM 5.1) and wraps the bytecode into `nvrhi::ShaderHandle`s; lookup by name/name+type. Must be `Initialize`d with the nvrhi device (done by `RenderSystem`). |
| `PSO.h` | `class PSO` | Fluent builder for an nvrhi graphics pipeline + binding layout: shader stages, blend/raster/depth-stencil state, vertex attributes, explicit binding-layout declaration (`add_constant_buffer`/`add_texture_srv`/`add_structured_buffer_srv`/`add_sampler`), per-object CB slot. Pipelines are created lazily per framebuffer-info (`GetOrCreatePipeline`); draw-time helpers assemble `BindingSetDesc`s (`AppendSamplers`/`AppendPerObjectData`/`AppendShaderResources`). |
| `VertexTypes.h` | `VertexPosition`, `VertexPNTBT`, `VertexBoned`, `Index16` | Engine vertex layouts with matching `nvrhi::VertexAttributeDesc` arrays; 16-bit index type. |
| `ObjectMask.h` | `class ObjectMask`, enums `VertexTypes`/`FillMode`/`LightType`/`BlendMode`/`ShadingModel` | Bitset classification combining vertex type/blend/shading/fill/light type into one 64-bit value — used to bucket/sort renderables for batched pipeline selection. |
| `SceneVisibility.h` | `class SceneVisibility`, `class VisibilityEntry` | Singleton culling/visibility system backed by an `OrthoTree` octree. Registers renderables with AABBs, rebuilds the octree each tick, frustum-culls, groups visible renderables into `DrawPacket`s keyed by `ObjectMask`. |
| `RenderGraph.h` | `class RenderGraph` | Simple ordered container/executor of `RenderPass`es. Root of the frame pipeline. |
| `IDrawable.h` / `RenderData.h` | — | Stubs/placeholders. |

## Subdirectories

| Directory | Contents |
|-----------|----------|
| [`RenderPass/`](RenderPass/AGENTS.md) | The deferred render passes: `ShadowPass`, `GBufferPass`, `GLightPass`, `ForwardPass`, `DebugPass`, `PostProcessPass`, plus the `EditorSwapChainPass` root and the `RenderPass` base. |
| [`ImGuiWindows/`](ImGuiWindows/AGENTS.md) | Editor ImGui panels: scene hierarchy, inspector, content browser, asset editor, editor viewport, and the shared `EditorContext`. |
| [`Interfaces/`](Interfaces/AGENTS.md) | Renderer abstractions: `IRenderable`, `IRenderContext`, `IRenderDevice`, `IUpdateGPUData`, `RenderTypes`. |

## Pipeline order (per frame)

```
ShadowPass -> GBufferPass -> GLightPass -> ForwardPass -> DebugPass -> PostProcessPass -> EditorSwapChainPass
```

The `EditorSwapChainPass` (in [`RenderPass/`](RenderPass/)) is the editor root: it owns the dockspace,
instantiates all ImGui windows, ticks visibility, executes the `EditorViewport`(s), and renders
ImGui draw data onto the swap-chain back-buffer with **native** D3D12 commands recorded on the same
command list after all NVRHI work (followed by `clearState()`).

## NVRHI conventions

- **Register slots are canonical**: b0 view matrices, b1 per-object world matrices (b0 in the
  shadow pass), b2 material/screen-dims, b3 light data; material textures t0..t7; samplers s0/s1.
  Each pass declares exactly the registers its shaders use on its `PSO`s.
- **Constant/structured buffers** are GPU-local with `keepInitialState`, updated via
  `writeBuffer` (`WriteBufferImmediate` mid-frame, `WriteBufferDeffered` from asset loads) —
  no volatile buffers, so dirty-flag update patterns keep working.
- **Binding sets** are assembled per draw as `BindingSetDesc`s and deduplicated by
  `RenderContext::GetOrCreateBindingSet`; items and layouts share one canonical sort order.
- **State tracking/barriers** are NVRHI's job; the only manual transitions left are the swap-chain
  back buffers (native) inside `SwapChain::BeginRender`/`EndRender`.
- **ImGui textures** (`ImTextureID`) are GPU handles in `RenderDevice`'s dedicated SRV heap;
  create them with `RenderDevice::CreateImGuiTextureSrv(nvrhi_texture)` and free viewport-sized
  ones with `FreeImGuiDescriptorDeferred` (frames-in-flight safe).
- **Texture SRV component mappings** (`Shader4ComponentMapping`) are not supported by NVRHI;
  material texture channel swizzles are parsed from JSON but ignored.
