#pragma once

// Replaces the old PSO.h (root-signature + D3D12_GRAPHICS_PIPELINE_STATE_DESC builder).
// Under NVRHI a graphics pipeline is created from shaders + render state + binding
// layouts against a target framebuffer; resource binding happens per-draw via
// BindingSets rather than descriptor-table root parameters.

#include <memory>
#include <functional>

#include <nvrhi/nvrhi.h>
#include <nvrhi/utils.h>

#include <RenderDevice.h>
#include <PerObjectData.h>
#include <IObjectShaderResource.h>
#include <RenderContext.h>

namespace GiiGa
{
    // Default render-state helpers mirroring the previous CD3DX12(D3D12_DEFAULT) values.
    inline nvrhi::RasterState DefaultRasterState()
    {
        return nvrhi::RasterState()
               .setFillSolid()
               .setCullBack()
               .setFrontCounterClockwise(false)
               .setDepthClipEnable(true);
    }

    inline nvrhi::DepthStencilState DefaultDepthState()
    {
        return nvrhi::DepthStencilState()
               .enableDepthTest()
               .enableDepthWrite()
               .setDepthFunc(nvrhi::ComparisonFunc::Less)
               .disableStencil();
    }

    // A fluent builder producing an nvrhi::GraphicsPipelineHandle. Callers supply the
    // binding layout(s) that match their shader register usage and a target framebuffer
    // (which defines the RT/DS formats the pipeline is compatible with).
    class Pipeline
    {
    public:
        Pipeline& VS(nvrhi::ShaderHandle s) { desc_.setVertexShader(s); return *this; }
        Pipeline& PS(nvrhi::ShaderHandle s) { desc_.setPixelShader(s); return *this; }
        Pipeline& GS(nvrhi::ShaderHandle s) { desc_.setGeometryShader(s); return *this; }
        Pipeline& HS(nvrhi::ShaderHandle s) { desc_.setHullShader(s); return *this; }
        Pipeline& DS(nvrhi::ShaderHandle s) { desc_.setDomainShader(s); return *this; }

        Pipeline& InputLayout(nvrhi::InputLayoutHandle l) { desc_.setInputLayout(l); return *this; }
        Pipeline& PrimType(nvrhi::PrimitiveType t) { desc_.setPrimType(t); return *this; }

        Pipeline& Raster(const nvrhi::RasterState& r) { desc_.renderState.rasterState = r; return *this; }
        Pipeline& Depth(const nvrhi::DepthStencilState& d) { desc_.renderState.depthStencilState = d; return *this; }
        Pipeline& Blend(const nvrhi::BlendState& b) { desc_.renderState.blendState = b; return *this; }

        Pipeline& AddBindingLayout(nvrhi::BindingLayoutHandle layout)
        {
            desc_.addBindingLayout(layout);
            return *this;
        }

        nvrhi::GraphicsPipelineHandle Build(RenderDevice& device, nvrhi::IFramebuffer* fb)
        {
            return device.Device()->createGraphicsPipeline(desc_, fb);
        }

        nvrhi::GraphicsPipelineDesc& Desc() { return desc_; }

    private:
        nvrhi::GraphicsPipelineDesc desc_;
    };

    // What a pass stores per ObjectMask: the compiled pipeline plus the binding layout
    // used to assemble per-draw binding sets.
    struct MaskPipeline
    {
        nvrhi::GraphicsPipelineHandle pipeline;
        nvrhi::BindingLayoutHandle bindingLayout;
    };
}
