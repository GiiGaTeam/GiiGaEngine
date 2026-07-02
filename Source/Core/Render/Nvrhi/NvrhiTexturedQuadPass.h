#pragma once

// NvrhiTexturedQuadPass.h
//
// Smoke-test pass (not part of the final renderer): draws a textured quad with a
// time-varying tint. Validates the NVRHI *binding-set model* end-to-end — the last
// unvalidated mechanic the real deferred passes depend on:
//   - a volatile constant buffer at b0, updated every frame via writeBuffer;
//   - a generated texture at t0, uploaded via writeTexture (keepInitialState=ShaderResource);
//   - a sampler at s0;
//   - all wired through one binding layout + binding set, added to GraphicsState.bindings.

#include "NvrhiCommon.h"
#include "NvrhiFrame.h"
#include "NvrhiRenderPass.h"
#include "NvrhiSwapChain.h"
#include "NvrhiShader.h"
#include "NvrhiResources.h"

#include <cmath>
#include <vector>
#include <directxtk12/SimpleMath.h>

namespace GiiGa::rhi
{
    class TexturedQuadPass : public RenderPass
    {
    public:
        TexturedQuadPass(nvrhi::IDevice* device, ShaderCache& shaders, SwapChain* swap)
            : device_(device), swap_(swap)
        {
            // Two-triangle quad in clip space, with UVs.
            using namespace DirectX;
            const SimpleMath::Vector3 zero{0, 0, 0};
            std::vector<VertexPNTBT> verts = {
                VertexPNTBT({-0.5f,  0.5f, 0.f}, zero, zero, zero, {0.f, 0.f}),
                VertexPNTBT({ 0.5f,  0.5f, 0.f}, zero, zero, zero, {1.f, 0.f}),
                VertexPNTBT({ 0.5f, -0.5f, 0.f}, zero, zero, zero, {1.f, 1.f}),
                VertexPNTBT({-0.5f,  0.5f, 0.f}, zero, zero, zero, {0.f, 0.f}),
                VertexPNTBT({ 0.5f, -0.5f, 0.f}, zero, zero, zero, {1.f, 1.f}),
                VertexPNTBT({-0.5f, -0.5f, 0.f}, zero, zero, zero, {0.f, 1.f}),
            };

            // 4x4 RGBA8 checkerboard (2x2 red/white blocks).
            const uint32_t tw = 4, th = 4;
            std::vector<uint8_t> texData(size_t(tw) * th * 4);
            for (uint32_t y = 0; y < th; ++y)
                for (uint32_t x = 0; x < tw; ++x)
                {
                    const bool red = ((x / 2) + (y / 2)) % 2 == 0;
                    uint8_t* p = &texData[(size_t(y) * tw + x) * 4];
                    p[0] = 255;
                    p[1] = red ? 0 : 255;
                    p[2] = red ? 0 : 255;
                    p[3] = 255;
                }

            // One-shot upload: vertex buffer + texture.
            nvrhi::CommandListHandle cl = device_->createCommandList();
            cl->open();
            vertexBuffer_ = createVertexBuffer<VertexPNTBT>(device_, cl.Get(), verts, "quad vb");

            nvrhi::TextureDesc td;
            td.setWidth(tw).setHeight(th).setFormat(nvrhi::Format::RGBA8_UNORM)
              .setInitialState(nvrhi::ResourceStates::ShaderResource).setKeepInitialState(true)
              .setDebugName("smoke checkerboard");
            texture_ = device_->createTexture(td);
            cl->writeTexture(texture_, 0, 0, texData.data(), tw * 4);
            cl->close();
            device_->executeCommandList(cl.Get());
            device_->waitForIdle();

            sampler_ = device_->createSampler(nvrhi::SamplerDesc{});

            // Volatile constant buffer for the per-frame tint (float4 = 16 bytes).
            vcb_ = createVolatileConstantBuffer(device_, 16, 16, "smoke tint vcb");

            nvrhi::ShaderHandle vs = shaders.getOrCreate(device_, L"Shaders/NvrhiTexturedVS.hlsl",
                                                         "VSMain", nvrhi::ShaderType::Vertex);
            nvrhi::ShaderHandle ps = shaders.getOrCreate(device_, L"Shaders/NvrhiTexturedPS.hlsl",
                                                         "PSMain", nvrhi::ShaderType::Pixel);
            inputLayout_ = createInputLayout(device_, makeVertexAttributes<VertexPNTBT>(), vs);

            // Binding layout: b0 VCB, t0 texture, s0 sampler (pixel-only).
            nvrhi::BindingLayoutDesc bld;
            bld.setVisibility(nvrhi::ShaderType::Pixel);
            bld.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0));
            bld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(0));
            bld.addItem(nvrhi::BindingLayoutItem::Sampler(0));
            bindingLayout_ = device_->createBindingLayout(bld);

            nvrhi::BindingSetDesc bsd;
            bsd.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, vcb_));
            bsd.addItem(nvrhi::BindingSetItem::Texture_SRV(0, texture_));
            bsd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler_));
            bindingSet_ = device_->createBindingSet(bsd, bindingLayout_);

            nvrhi::GraphicsPipelineDesc pd;
            pd.setVertexShader(vs).setPixelShader(ps).setInputLayout(inputLayout_);
            pd.addBindingLayout(bindingLayout_);

            nvrhi::RenderState renderState;
            renderState.rasterState.setCullNone();
            renderState.depthStencilState.disableDepthTest();
            pd.setRenderState(renderState);

            pipeline_ = device_->createGraphicsPipeline(pd, swap_->getCurrentFramebuffer());
        }

        void execute(const FrameContext& ctx) override
        {
            nvrhi::IFramebuffer* fb = swap_ ? swap_->getCurrentFramebuffer() : nullptr;
            if (!fb || !pipeline_ || !ctx.cmd) return;

            ctx.cmd->clearTextureFloat(swap_->getCurrentTexture(), nvrhi::AllSubresources,
                                       nvrhi::Color{0.02f, 0.02f, 0.03f, 1.0f});

            // Per-frame tint — animate to prove the volatile CB is updated every frame.
            const float t = static_cast<float>(frame_) * 0.02f;
            float tint[4] = {
                0.5f + 0.5f * std::sin(t),
                0.5f + 0.5f * std::sin(t + 2.094f),
                0.5f + 0.5f * std::sin(t + 4.188f),
                1.0f,
            };
            ctx.cmd->writeBuffer(vcb_, tint, sizeof(tint));

            nvrhi::GraphicsState state;
            state.setPipeline(pipeline_).setFramebuffer(fb);
            nvrhi::Viewport vp(static_cast<float>(swap_->getWidth()), static_cast<float>(swap_->getHeight()));
            state.setViewport(nvrhi::ViewportState().addViewportAndScissorRect(vp));
            state.addBindingSet(bindingSet_);

            nvrhi::VertexBufferBinding vb;
            vb.buffer = vertexBuffer_.Get();
            vb.slot = 0;
            vb.offset = 0;
            state.addVertexBuffer(vb);

            ctx.cmd->setGraphicsState(state);
            ctx.cmd->draw(nvrhi::DrawArguments().setVertexCount(6));
            ++frame_;
        }

    private:
        nvrhi::IDevice* device_ = nullptr;
        SwapChain* swap_ = nullptr;
        uint64_t frame_ = 0;
        nvrhi::BufferHandle vertexBuffer_;
        nvrhi::TextureHandle texture_;
        nvrhi::SamplerHandle sampler_;
        nvrhi::BufferHandle vcb_;
        nvrhi::InputLayoutHandle inputLayout_;
        nvrhi::BindingLayoutHandle bindingLayout_;
        nvrhi::BindingSetHandle bindingSet_;
        nvrhi::GraphicsPipelineHandle pipeline_;
    };
}
