#pragma once

// NvrhiTrianglePass.h
//
// Self-contained NVRHI graphics pass that draws a single orange triangle into the swap
// chain's current back buffer. It exists to validate the *full* NVRHI draw pipeline —
// HLSL compile (ShaderCache), input layout, pipeline state, static vertex-buffer upload,
// and setGraphicsState + draw — before the real deferred passes and ECS components are
// wired in. It is not part of the final renderer.

#include "NvrhiCommon.h"
#include "NvrhiFrame.h"
#include "NvrhiRenderPass.h"
#include "NvrhiSwapChain.h"
#include "NvrhiShader.h"
#include "NvrhiResources.h"

#include <vector>
#include <directxtk12/SimpleMath.h>

namespace GiiGa::rhi
{
    class TrianglePass : public RenderPass
    {
    public:
        TrianglePass(nvrhi::IDevice* device, ShaderCache& shaders, SwapChain* swap)
            : device_(device), swap_(swap)
        {
            std::vector<VertexPosition> verts = {
                VertexPosition(DirectX::SimpleMath::Vector3( 0.0f,  0.5f, 0.0f)),
                VertexPosition(DirectX::SimpleMath::Vector3( 0.5f, -0.5f, 0.0f)),
                VertexPosition(DirectX::SimpleMath::Vector3(-0.5f, -0.5f, 0.0f)),
            };

            // Upload the vertex buffer in a one-shot command list, then lock its state so
            // every subsequent (frame) command list treats it as a permanent vertex buffer.
            nvrhi::CommandListHandle cl = device_->createCommandList();
            cl->open();
            vertexBuffer_ = createVertexBuffer<VertexPosition>(device_, cl.Get(), verts, "triangle vb");
            cl->close();
            device_->executeCommandList(cl.Get());
            device_->waitForIdle();

            nvrhi::ShaderHandle vs = shaders.getOrCreate(device_, L"Shaders/NvrhiTriangleVS.hlsl",
                                                         "VSMain", nvrhi::ShaderType::Vertex);
            nvrhi::ShaderHandle ps = shaders.getOrCreate(device_, L"Shaders/NvrhiTrianglePS.hlsl",
                                                         "PSMain", nvrhi::ShaderType::Pixel);

            inputLayout_ = createInputLayout(device_, makeVertexAttributes<VertexPosition>(), vs);

            nvrhi::GraphicsPipelineDesc pd;
            pd.setVertexShader(vs).setPixelShader(ps).setInputLayout(inputLayout_);

            nvrhi::RenderState renderState;
            renderState.rasterState.setCullNone();
            renderState.depthStencilState.disableDepthTest(); // swap-chain framebuffer has no depth
            pd.setRenderState(renderState);

            pipeline_ = device_->createGraphicsPipeline(pd, swap_->getCurrentFramebuffer());
        }

        void execute(const FrameContext& ctx) override
        {
            nvrhi::IFramebuffer* fb = swap_ ? swap_->getCurrentFramebuffer() : nullptr;
            if (!fb || !pipeline_ || !ctx.cmd) return;

            ctx.cmd->clearTextureFloat(swap_->getCurrentTexture(), nvrhi::AllSubresources,
                                       nvrhi::Color{0.02f, 0.02f, 0.03f, 1.0f});

            nvrhi::GraphicsState state;
            state.setPipeline(pipeline_).setFramebuffer(fb);

            nvrhi::Viewport vp(static_cast<float>(swap_->getWidth()), static_cast<float>(swap_->getHeight()));
            state.setViewport(nvrhi::ViewportState().addViewportAndScissorRect(vp));

            nvrhi::VertexBufferBinding vb;
            vb.buffer = vertexBuffer_.Get();
            vb.slot = 0;
            vb.offset = 0;
            state.addVertexBuffer(vb);

            ctx.cmd->setGraphicsState(state);
            ctx.cmd->draw(nvrhi::DrawArguments().setVertexCount(3));
        }

    private:
        nvrhi::IDevice* device_ = nullptr;
        SwapChain* swap_ = nullptr;
        nvrhi::BufferHandle vertexBuffer_;
        nvrhi::InputLayoutHandle inputLayout_;
        nvrhi::GraphicsPipelineHandle pipeline_;
    };
}
