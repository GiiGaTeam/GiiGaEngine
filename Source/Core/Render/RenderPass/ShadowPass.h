#pragma once

// ShadowPass.h — cascaded shadow-map generation for directional lights (NVRHI).
//
// The intended pipeline: VertexPNTBTShader(VSMain) + CascadeShadowGeomShader(GSMain, geometry
// shader amplifying to 4 cascades) with depth bias, rendering shadow-casting opaque/masked
// DefaultLit solid geometry into the light's D32_FLOAT shadow depth texture array.
//
// NOTE: DirectionalLightComponent's shadow resources (shadow DSV/SRV + cascade structured
// buffer) are still created via the legacy D3D12 path (BufferView/GPULocalResource) and are
// NOT yet NVRHI-owned, so they cannot be bound through an NVRHI binding set / framebuffer.
// Until that conversion happens, execute() is a no-op stub. The filter masks and depth-bias
// parameters below document the faithful intent.
// TODO(shadow): convert DirectionalLightComponent shadow resources to NVRHI and implement
// execute() (depth-only framebuffer from the cascade array + per-cascade view constants).

#include <memory>
#include <functional>

#include <Core/Render/Nvrhi/NvrhiRenderPass.h>
#include <Core/Render/Nvrhi/NvrhiFrame.h>
#include <Core/Render/Nvrhi/NvrhiShader.h>

#include <RenderPassViewData.h>
#include <ObjectMask.h>
#include <VertexTypes.h>

namespace GiiGa
{
    class ShadowPass : public rhi::RenderPass
    {
    public:
        ShadowPass(nvrhi::IDevice* /*device*/, rhi::ShaderCache& /*shaders*/,
                   std::function<RenderPassViewData()> getCamInfo)
            : getCamInfoDataFunction_(std::move(getCamInfo))
        {
        }

        void execute(const rhi::FrameContext& /*ctx*/) override
        {
            // TODO(shadow): stubbed — see file header. Shadow DSV/SRV + cascade data still live
            // on DirectionalLightComponent via the legacy D3D12 path and are not yet NVRHI-bound.
            (void)getCamInfoDataFunction_;
        }

    private:
        ObjectMask filter_objects_ = ObjectMask().SetVertexType(VertexTypes::VertexPNTBT)
                                                 .SetBlendMode(BlendMode::Opaque | BlendMode::Masked)
                                                 .SetShadingModel(ShadingModel::DefaultLit)
                                                 .SetFillMode(FillMode::Solid);

        ObjectMask filter_lights_ = ObjectMask().SetLightType(LightType::Directional)
                                                .SetShadingModel(ShadingModel::All)
                                                .SetVertexType(VertexTypes::All)
                                                .SetFillMode(FillMode::All);

        std::function<RenderPassViewData()> getCamInfoDataFunction_;

        int DepthBias = -15000;
        float DepthBiasClamp = 0;
        float SlopeScaledDepthBias = 8.000f;
    };
}
