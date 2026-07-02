# AGENTS.md — Source/Core/Render/Nvrhi/

The **NVRHI-backed rendering foundation**, being introduced to replace the hand-rolled
D3D12 backend in the parent [`Render/`](../AGENTS.md) directory. This is an in-progress
migration; these files compile alongside the legacy backend and are not yet wired into the
live render path.

All types are in the `GiiGa::rhi` namespace. They deliberately avoid wrapping NVRHI's own
types in pass-through classes — the engine uses `nvrhi::DeviceHandle` and
`nvrhi::CommandListHandle` directly. Only things that represent genuine stateful logic (or
own objects whose lifetime NVRHI does not manage) get a class here.

## Files

| File | Type(s) | Purpose |
|------|---------|---------|
| `NvrhiCommon.h` | `MessageCallback`, `ThrowIfFailed` | Shared includes (`<nvrhi/nvrhi.h>`, `<nvrhi/d3d12.h>`, `<nvrhi/utils.h>`). `MessageCallback` implements `nvrhi::IMessageCallback`, routing NVRHI diagnostics to stderr + the debug output window. `ThrowIfFailed` is a self-contained HRESULT check (so the foundation does not depend on the legacy `DirectXUtils.h`). |
| `NvrhiDevice.h` | `struct Rhi`, `DeviceDesc`, `createRhi()` | NVRHI does **not** create the D3D12 device/queues — the app owns them. `Rhi` is the once-per-process owner (`IDXGIFactory6`, adapter, `ID3D12Device`, graphics `ID3D12CommandQueue`, `MessageCallback`, and the `nvrhi::DeviceHandle` that wraps them). `createRhi()` is the bootstrap factory that enables the debug layer, creates the device + graphics queue, and calls `nvrhi::d3d12::createDevice` (optionally wrapped with `nvrhi::validation` in debug). Returns `std::unique_ptr<Rhi>` because `IMessageCallback` is non-copyable/non-movable. **No forwarding methods** — consumers use `rhi->device->...` directly. |
| `NvrhiSwapChain.h` | `class SwapChain` | DXGI flip-model swap chain (`IDXGISwapChain4`, frame-latency waitable object) whose back buffers are exposed as NVRHI textures + framebuffers. Each back buffer is wrapped via `createHandleForNativeTexture` with `keepInitialState=true` / `initialState=Present` so NVRHI transitions them correctly for flip-discard presentation. Provides `beginFrame`, `getCurrentTexture`/`getCurrentFramebuffer`, `present`, `resize`, `getFrameLatencyWaitableObject`. Accepts a raw `HWND` + `nvrhi::IDevice*` + `ID3D12CommandQueue*` (decoupled from the engine `Window` type). |
| `NvrhiShader.h` | `class ShaderCache` | NVRHI's `createShader` takes already-compiled bytecode, so HLSL compilation is still the app's job. `ShaderCache` compiles the engine's `.hlsl` files with `D3DCompileFromFile` and hands the bytecode to `nvrhi::IDevice::createShader`, caching the resulting `nvrhi::ShaderHandle` by (file, entry). Replaces the legacy `ShaderManager.h` (which produced `D3D12_SHADER_BYTECODE`s for the hand-rolled PSO builder). |
| `NvrhiResources.h` | free functions | Convenience helpers — no classes: `createVertexBuffer`/`createIndexBuffer`/`createConstantBuffer`/`createVolatileConstantBuffer` (committed buffers, uploaded via `ICommandList::writeBuffer`, 256-aligned CB size), plus `makeVertexAttributes<VertexPosition\|VertexPNTBT\|VertexBoned>()` and `createInputLayout`. These replace the `GPULocalResource` + `UploadBuffer` + `BufferView` + `Mesh` buffer-creation boilerplate and the `D3D12_INPUT_ELEMENT_DESC` arrays in `VertexTypes.h`. |
| `NvrhiConstants.h` | `ObjectConstants`, `ViewConstants`, `ScreenDimensions` | Plain constant-buffer payload structs for the NVRHI path (per-object world matrices, per-view camera matrices, screen dims). Uploaded into constant buffers via `ICommandList::writeBuffer` and bound through `nvrhi::BindingSet`. Replace the `alignas(256)` structs in legacy `PerObjectData.h` / `RenderPassViewData.h`. |
| `NvrhiGBuffer.h` | `class GBuffer` | NVRHI-backed geometry buffer: four `RGBA32_FLOAT` color textures (LightAccumulation/Diffuse/Material/NormalWS) + a `D24S8` depth texture, bundled into one `nvrhi::FramebufferHandle`; `resize` + `clearAll(commandList)`. NVRHI creates views lazily and barriers automatically, so the legacy manual `RTV`/`SRV`/`DSV` descriptor views and `D3D12_RESOURCE_STATES` transitions vanish. Replaces legacy `GBuffer.h`. |
| `NvrhiFrame.h` | `struct FrameContext`, `struct IFrameUpdater` | Per-frame context (`nvrhi::IDevice*` + `nvrhi::ICommandList*`) handed to each pass — replaces the legacy `RenderContext` bundle. `IFrameUpdater::updateGPUData(device, cmd)` replaces `IUpdateGPUData(RenderContext&)`. |
| `NvrhiRenderPass.h` | `class RenderPass`, `class RenderGraph` | Minimal pass abstraction: `execute(FrameContext)` called once per frame in order; `RenderGraph` is the ordered pass list. Mirrors the legacy `RenderPass`/`RenderGraph` shape but on nvrhi handles. |
| `NvrhiClearPass.h` | `class ClearPass : public RenderPass` | First real pass: clears the swap-chain back buffer. Exercises the whole foundation (device → command list → wrapped back-buffer → barrier → clear → present) and is the template for the deferred passes. |
| `NvrhiTrianglePass.h` | `class TrianglePass : public RenderPass` | **Smoke-test-only** validation pass (not part of the final renderer): draws one orange triangle. Proves the full NVRHI graphics pipeline — HLSL compile (`ShaderCache`), input layout, `GraphicsPipelineDesc`, static vertex-buffer upload (`createVertexBuffer` with `keepInitialState=VertexBuffer`), `setGraphicsState`, and `draw`. Paired with the `--nvrhi-smoke` mode in `main.cpp`. Driven by `Shaders/NvrhiTriangleVS.hlsl` + `NvrhiTrianglePS.hlsl`. |
| `NvrhiTexturedQuadPass.h` | `class TexturedQuadPass : public RenderPass` | **Smoke-test-only** validation pass: draws a textured quad with an animated tint. Proves the NVRHI *binding-set model* — a volatile constant buffer (`b0`) updated every frame via `writeBuffer`, a generated texture (`t0`) uploaded via `writeTexture` (`keepInitialState=ShaderResource`), a sampler (`s0`), all wired through one `BindingLayoutDesc` + `BindingSetDesc` added to `GraphicsState.bindings`. Driven by `Shaders/NvrhiTexturedVS.hlsl` + `NvrhiTexturedPS.hlsl`. |
| `NvrhiRenderSystem.h` | `class RenderSystem` | Owns the backbone and drives the frame loop: `wait(frame-latency) → beginFrame → open(cmd) → update GPU data → execute passes → close → executeCommandList → present → runGarbageCollection`. Owns `Rhi`, `SwapChain`, one `nvrhi::CommandListHandle`, `ShaderCache`, the `RenderGraph`, and the `IFrameUpdater` registry. Replaces the legacy `RenderSystem`/`RenderContext` pair. |

## Notes for agents

- There is **no** `rhi::CommandList` and **no** `rhi::Device` wrapper — NVRHI already provides
  `nvrhi::CommandListHandle` (a smart pointer) and `nvrhi::DeviceHandle`. Do not reintroduce
  pass-through wrappers; that was tried and rejected.
- Command lists are created with `rhi->device->createCommandList()` and used directly
  (`->open()`, `->setGraphicsState(...)`, `->drawIndexed(...)`, `->close()`,
  `rhi->device->executeCommandList(cl.Get())`).
- **No wrappers for descriptors/pipelines/bindings either.** NVRHI manages descriptor heaps
  internally, so the legacy `DescriptorHeap`/`BufferView`/`ViewTypes`/`PSO` builder are gone:
  use `nvrhi::GraphicsPipelineDesc` + `createGraphicsPipeline`, and `nvrhi::BindingLayoutDesc`/
  `BindingSetDesc` + `createBindingLayout`/`createBindingSet` (or `nvrhi::utils::CreateBindingSetAndLayout`)
  directly. Resources are bound via `nvrhi::BindingSetItem::Texture_SRV(...)` etc., not descriptor handles.
- Per the NVRHI programming guide: call `rhi->device->runGarbageCollection()` once per frame
  to release GPU resources whose reference count hit zero.
- Legacy backend (`../RenderDevice.h`, `../GPULocalResource.h`, `../DescriptorHeap.h`,
  `../PSO.h`, `../RenderContext.h`, `../SwapChain.h`, ...) is scheduled for deletion in the
  final migration phase once all consumers move off D3D12 types.
