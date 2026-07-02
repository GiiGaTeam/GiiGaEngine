#pragma once

// NvrhiRenderSystem.h
//
// Owns the NVRHI rendering backbone and drives the per-frame loop:
//
//   wait(frame-latency) -> beginFrame -> open(cmd) -> update GPU data -> execute passes
//                       -> close(cmd) -> executeCommandList -> present -> runGarbageCollection
//
// This is real orchestration (frame pacing, GC, resize, pass execution), not a pass-through,
// so it earns a class — unlike device/command-list, which are used directly as nvrhi handles.
// It replaces the legacy RenderSystem/RenderContext pair. ImGui integration and the deferred
// passes plug in as RenderPass implementations.

#include "NvrhiCommon.h"
#include "NvrhiDevice.h"
#include "NvrhiSwapChain.h"
#include "NvrhiShader.h"
#include "NvrhiFrame.h"
#include "NvrhiRenderPass.h"

#include <algorithm>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace GiiGa::rhi
{
    class RenderSystem
    {
    public:
        RenderSystem(HWND hwnd, uint32_t width, uint32_t height, const DeviceDesc& desc = {})
        {
            rhi_ = createRhi(desc);
            swapChain_ = std::make_unique<SwapChain>(rhi_->device.Get(), rhi_->graphicsQueue.Get(), hwnd, width, height);
            cmd_ = rhi_->device->createCommandList();
        }

        ~RenderSystem()
        {
            if (rhi_) rhi_->device->waitForIdle();
        }

        RenderSystem(const RenderSystem&) = delete;
        RenderSystem& operator=(const RenderSystem&) = delete;
        RenderSystem(RenderSystem&&) = delete;
        RenderSystem& operator=(RenderSystem&&) = delete;

        void initialize()
        {
            // Caller adds real passes; if none, add a clear so the window isn't garbage.
            frameCtx_.device = rhi_->device.Get();
            frameCtx_.cmd = cmd_.Get();
        }

        // --- Pass / updater wiring ---

        void addPass(RenderPassHandle pass) { graph_.addPass(std::move(pass)); }

        void registerUpdater(IFrameUpdater* u)
        {
            updaters_.push_back(u);
        }
        void unregisterUpdater(IFrameUpdater* u)
        {
            auto it = std::find(updaters_.begin(), updaters_.end(), u);
            if (it != updaters_.end()) updaters_.erase(it);
        }

        // --- Frame ---

        void tick()
        {
            if (HANDLE h = swapChain_->getFrameLatencyWaitableObject())
            {
                WaitForSingleObject(h, INFINITE);
            }

            swapChain_->beginFrame();

            cmd_->open();

            for (IFrameUpdater* u : updaters_) u->updateGPUData(rhi_->device.Get(), cmd_.Get());

            graph_.execute(frameCtx_);

            cmd_->close();
            rhi_->device->executeCommandList(cmd_.Get());
            swapChain_->present();
            rhi_->device->runGarbageCollection();
        }

        void resize(uint32_t width, uint32_t height)
        {
            rhi_->device->waitForIdle();
            swapChain_->resize(width, height);
        }

        // --- Accessors (use the nvrhi handles directly) ---

        Rhi& rhi() { return *rhi_; }
        SwapChain& swapChain() { return *swapChain_; }
        nvrhi::IDevice* device() { return rhi_->device.Get(); }
        nvrhi::ICommandList* commandList() { return cmd_.Get(); }
        ShaderCache& shaderCache() { return shaderCache_; }

    private:
        std::unique_ptr<Rhi> rhi_;
        std::unique_ptr<SwapChain> swapChain_;
        nvrhi::CommandListHandle cmd_;
        ShaderCache shaderCache_;
        RenderGraph graph_;
        std::vector<IFrameUpdater*> updaters_;
        FrameContext frameCtx_;
    };
}
