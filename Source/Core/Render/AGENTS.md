# AGENTS.md — Source/Core/Render/

The DirectX 12 renderer (NVRHI + DirectX-Headers/Toolkit). A deferred-rendering pipeline driven by
a `RenderGraph` of `RenderPass`es, with Diligent-inspired descriptor-heap machinery, an octree
visibility/culling system, and editor ImGui windows.

All in the `GiiGa` namespace.

## Device, heaps & memory

| File | Class(es) | Purpose |
|------|-----------|---------|
| `RenderDevice.h` | `class RenderDevice : public IRenderDevice` | Central D3D12 device wrapper. Creates/owns the `ID3D12Device`, four CPU descriptor heaps and two shader-visible GPU heaps; factory methods for command queues, fences, command lists/allocators, committed resources, root signatures, PSOs, and all view types (allocating descriptors and copying CPU->GPU). |
| `DescriptorHeap.h` | `IDescriptorAllocator`, `DescriptorHeapAllocation`, `DescriptorHeapAllocationManager`, `CPUDescriptorHeap`, `GPUDescriptorHeap`, `DynamicSuballocationsManager`, `DesciptorHandles` | Descriptor-heap subsystem (modeled on Diligent's D3D12 backend): suballocation of CPU/GPU descriptor heaps for CBV/SRV/UAV/Sampler/RTV/DSV with deferred freeing tied to frames-in-flight. |
| `VariableSizeAllocationsManager.h` | `class VariableSizeAllocationsManager` | Generic free-block allocator (offset/size cross-referenced ordered maps) for best-fit, alignment-aware suballocation within a contiguous range — backs the descriptor-heap managers. |
| `UploadBuffer.h` | `class UploadBuffer` | Linear page-based allocator for the D3D12 upload heap (CPU-visible, persistently mapped); pages pooled/reused; returns CPU write span + GPU virtual address — for staging GPU copies. |
| `Align.h` | template fns | Low-level `IsPowerOfTwo`/`AlignUp`/`AlignDown` (and non-power-of-two variants) for scalars and pointers. |
| `DirectXUtils.h` | free fns / functors | `std::hash`+`operator==` for D3D12 view descriptions (so they key unordered_maps), `DXDeleter`/`DXDelayedDeleter` COM deleters, and `ThrowIfFailed`. |

## Resources & views

| File | Class(es) | Purpose |
|------|-----------|---------|
| `ViewTypes.h` | tag structs `Constant`/`ShaderResource`/`UnorderedAccess`/`RenderTarget`/`DepthStencil`/`Vertex`/`Index` | Bind a logical view category to its descriptor-holder + descriptor-desc types — the template params for `BufferView`. |
| `BufferView.h` | `template<class ViewType> BufferView` | Generic move-only wrapper pairing a descriptor holder with the creation descriptor for a resource view — a typed handle to a view. |
| `GPULocalResource.h` | `class GPULocalResource` | Owns a default-heap (GPU-local) `ID3D12Resource`. Tracks state, performs deferred/immediate uploads + transitions, lazily creates & caches all view types (CBV/SRV/UAV/RTV/DSV/Vertex/Index) keyed by their descriptors. |
| `FrameContext.h` | `class FrameContext` | Per-frame GPU resources for recording a frame: command allocator, dynamic descriptor suballocation managers (CBV/SRV/UAV + Sampler), upload-buffer pool. `Reset`, `CreateUploadBuffer`, `AllocateDynamicConstantView`, `AllocateDynamicShaderResourceView`. |

## Frame pipeline

| File | Class(es) | Purpose |
|------|-----------|---------|
| `CommandQueue.h` | `class CommandQueue` | Wraps a D3D12 command queue with fence-based GPU sync + a pool/recycler of command allocators & lists for reuse across frames (`GetCommandList`, `ExecuteCommandList`, `Signal`, `WaitForFenceValue`, `Flush`). |
| `SwapChain.h` | `class SwapChain` | DXGI flip-model `IDXGISwapChain4` with a frame-latency waitable object; owns back-buffer render targets (`GPULocalResource`s + RTVs); create/reset/present transitions/resize. |
| `RenderContext.h` | `class RenderContext : public IRenderContext`, `struct DefferedUploadEntry` | The frame-recording context: owns the graphics command queue/list, per-frame `FrameContext`s, fence-based frame sync (with the swap chain's waitable object), deferred upload processing, and helpers for barriers/copies/clears/PSO+root-signature+descriptor binding. |
| `RenderSystem.h` | `class RenderSystem` | Abstract base of the rendering system. Owns `RenderDevice`, `RenderContext`, `SwapChain`, `ShaderManager`, the root `RenderGraph`, and an `IUpdateGPUData` registry. Drives the per-frame tick (stale cleanup -> start frame -> update GPU data -> draw graph -> end frame -> present); abstract `Initialize`/`SetCamera`. |
| `EditorRenderSystem.h` | `class EditorRenderSystem : public RenderSystem` | Concrete `RenderSystem` for the editor: wires up an `EditorSwapChainPass` and `EditorViewport`s as the render-graph root. |
| `RenderSystemSettings.h` | `struct RenderSystemSettings` | Static renderer-wide constants: frames-in-flight, back-buffer count, CPU/GPU descriptor-heap sizes (per type + dynamic region). |
| `RenderGraph.h` | `class RenderGraph` | Simple ordered container/executor of `RenderPass`es — `Draw` iterates each pass in sequence; passes added dynamically. Root of the frame pipeline. |
| `RenderPassViewData.h` | `RenderPassViewMatricies`, `ScreenDimensions`, `RenderPassViewData`, `RTVData` | Per-camera/view constant-buffer structures and a container holding camera info + GPU descriptor handles for view matrices/screen dims; `RTVData` bundles a render-target resource with its RTV/SRV. |
| `Viewport.h` | `class Viewport` | Abstract base for a renderable viewport: owns a `GBuffer`, a result RTV (`RTVData`), a size, and a render graph; pure-virtual camera binding + per-frame execution. |
| `GBuffer.h` | `class GBuffer`, `enum GBufferOrder` | The geometry-buffer set for deferred rendering: multiple RGBA32F RTs (LightAccumulation, Diffuse, Material, NormalWS) + depth/stencil, with RTV/SRV views, resize/clear/bind/state transitions. |

## Renderables, materials & meshes

| File | Class(es) | Purpose |
|------|-----------|---------|
| `Material.h` | `class Material : AssetBase, IUpdateGPUData`, `MaterialShaderResource`, enums `TexturesOrder`/`TexturesIndex` | Renderable material: tint/scalar props, up to 8 textures (base color, metallic, specular, roughness, ...), blend/shading modes, constant buffer; loads required default textures, updates GPU data, serializes to JSON. |
| `Mesh.h` | `template<class VertexType> Mesh` | Static mesh container: creates GPU vertex/index buffers (`GPULocalResource`) from CPU arrays, their views, deferred uploads, indexed drawing, AABB + object mask. |
| `SkeletalMesh.h` | `class SkeletalMesh`, `struct Joint`, `Skeleton` | Skinned/animated mesh (uses `VertexBoned`): GPU vertex/index buffers + views, deferred uploads, indexed drawing; defines `Joint`/`Skeleton`. |
| `PerObjectData.h` | `struct WorldMatrices`, `class PerObjectData` | Per-object constant buffer (world + inverse-world matrices from a `TransformComponent`); static (GPU-local) and dynamic (per-frame upload) paths. |
| `ObjectMask.h` | `class ObjectMask`, enums `VertexTypes`/`FillMode`/`LightType`/`BlendMode`/`ShadingModel` | Bitset classification combining vertex type/blend/shading/fill/light type into one 64-bit value — used to bucket/sort renderables for batched PSO selection; `CoversMask`, bitwise ops, `std::hash`. |
| `VertexTypes.h` | `VertexPosition`, `VertexPNTBT`, `VertexBoned`, `Index16` | Engine vertex layouts with matching `D3D12_INPUT_ELEMENT_DESC`/input-layout descriptions; 16-bit index type. |
| `IDrawable.h` | `class IDrawable` | Minimal abstract drawable interface (returns `RenderData` — currently a stub). |
| `RenderData.h` | `struct RenderData` | Placeholder/stub data struct for rendering an object (currently empty). |
| `IObjectShaderResource.h` | `struct IObjectShaderResource` | Abstract per-object/per-material shader-resource bindings: returns the GPU descriptor handles to bind. |

## Shaders & visibility

| File | Class(es) | Purpose |
|------|-----------|---------|
| `ShaderManager.h` | `class ShaderManager` (nested `Shader`), `enum ShaderTypes` | Singleton that pre-compiles & caches a fixed set of engine HLSL shaders (`D3DCompileFromFile`) into `D3D12_SHADER_BYTECODE`s keyed by (file, type); lookup by name/name+type/vertex type. Global shader-path constants. |
| `Shader.h` | — | Empty placeholder (`#pragma only`); the real `Shader` class is nested in `ShaderManager.h`. |
| `SceneVisibility.h` | `class SceneVisibility`, `class VisibilityEntry` | Singleton culling/visibility system backed by an `OrthoTree` octree. Registers renderables with AABBs, rebuilds the octree each tick, frustum-culls against a view-proj matrix, and groups visible, filter-matching renderables into `DrawPacket`s keyed by `ObjectMask`. RAII `VisibilityEntry` helper. |

## PSO

| File | Class(es) | Purpose |
|------|-----------|---------|
| `PSO.h` | `class PSO` | Fluent builder for a D3D12 graphics Pipeline State Object + root signature: setters for all shader stages, blend/rasterizer/depth-stencil states, input layout, RTV/DSV formats, static samplers, per-object/shader-resource binding callbacks; generates descriptor-table root parameters from CBV/SRV/UAV counts. `GeneratePSO`, `GetState`, `GetSignature`. |

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
ImGui draw data onto the swap-chain back-buffer.
