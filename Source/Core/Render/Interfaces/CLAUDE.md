# AGENTS.md — Source/Core/Render/Interfaces/

Abstract renderer interfaces that decouple engine gameplay/asset code from the concrete D3D12
implementation in the parent [`Render/`](../AGENTS.md) directory. Components and renderables
program against these; the concrete `RenderDevice`/`RenderContext` implement them.

All in the `GiiGa` namespace.

## Files

| File | Class(es) | Purpose |
|------|-----------|---------|
| `IRenderable.h` | `class IRenderable`, `struct SortData`, `struct CommonResourceGroup`, `struct DrawPacket` | Core abstraction for anything drawable. Defines sorting/grouping data used by the visibility & render-pass systems to batch draw calls by shared resources: `Draw`, `GetSortData`, `GetPerObjectData`; `DrawPacket` is a per-`ObjectMask` map of resource groups. |
| `IRenderContext.h` | `struct IRenderContext`, `struct DefferedUploadQuery` | Interface for GPU context/command operations needed for resource uploading & barriers, abstracting the D3D12 command list behind buffer-copy, barrier, and upload-allocation calls (`CreateDefferedUpload`, `CreateAndAllocateUploadBufferInCurrentFrame`, `CopyBufferRegion`, `ResourceBarrier`). |
| `IRenderDevice.h` | `struct IRenderDevice` | Interface for the GPU device: descriptor-heap + committed-resource creation, descriptor increment sizes, and a frame-in-flight deferred-deletion queue (`KeepAliveForFramesInFlight` / `DeleteStaleObjects`) for safe GPU resource lifetime management. |
| `IUpdateGPUData.h` | `struct IUpdateGPUData` | Minimal marker interface for objects that push per-frame data updates to the GPU: pure-virtual `UpdateGPUData(RenderContext&)`. |
| `RenderTypes.h` | — | Empty placeholder (empty `namespace GiiGa {}`); reserved for shared render type definitions. |

## Implementations

| Interface | Concrete implementation (parent dir) |
|-----------|----------------------------------------|
| `IRenderable` | `StaticMeshComponent`, `CollisionComponent`, `LightComponent` subclasses (`../../../Components/`) |
| `IRenderContext` | `RenderContext` (`../RenderContext.h`) |
| `IRenderDevice` | `RenderDevice` (`../RenderDevice.h`) |
| `IUpdateGPUData` | `StaticMeshComponent`, `CollisionComponent`, lights, `Material` |
