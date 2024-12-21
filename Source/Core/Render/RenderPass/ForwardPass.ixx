#include<d3d12.h>

export module ForwardPass;

import <memory>;
import <vector>;
import <functional>;

import RenderPass;
//import ShaderManager;
import PSO;
import IRenderable;
import SceneVisibility;
import RenderPassViewData;
import Logger;

namespace GiiGa
{
    export class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(const std::function<RenderPassViewData()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
        }

        void Draw(RenderContext& context) override
        {
            auto pso = PSO();
            //pso.set_vs(ShaderManager::GetShaderByName(L"Shaders/SimpleVertexShader.hlsl"));

            auto cam_info = getCamInfoDataFunction_();

            const auto& renderables = SceneVisibility::Extract(renderpass_filter, cam_info.viewProjMatrix);
            // Getting renderables

            if (renderables.size() > 0)
                el::Loggers::getLogger(LogRendering)->debug("See some renderable");

            //for (auto& renderable : renderables)
            //{
            //    renderable.Draw(context);
            //}
        }

    private:
        ObjectMask renderpass_filter = ObjectMask().SetBlendMode(BlendMode::Opaque | BlendMode::Masked)
                                                   .SetShadingModel(ShadingModel::All)
                                                   .SetVertexType(VertexTypes::All);
        std::function<RenderPassViewData()> getCamInfoDataFunction_;
    };
}
