#include<d3d12.h>

export module ForwardPass;

import <memory>;
import <vector>;
import <functional>;

import RenderPass;
//import ShaderManager;
import PSO;
import IRenderable;

namespace GiiGa
{
    export class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(const std::function<D3D12_GPU_DESCRIPTOR_HANDLE()>& getCamDescFn):
            getCamDescFnFunction_(getCamDescFn)
        {
        }

        void Draw(RenderContext& context) override
        {
            auto pso = PSO();
            //pso.set_vs(ShaderManager::GetShaderByName(L"Shaders/SimpleVertexShader.hlsl"));

            std::vector<IRenderable> renderables;
            // Getting renderables

            for (auto& renderable : renderables)
            {
                renderable.Draw(context);
            }
        }

    private:
        std::function<D3D12_GPU_DESCRIPTOR_HANDLE()> getCamDescFnFunction_;
    };
}
