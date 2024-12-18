module;
#include <memory>
#include <vector>

export module ForwardPass;

import RenderPass;
//import ShaderManager;
import PSO;
import IRenderable;
import Viewport;

namespace GiiGa
{

    export class ForwardPass : public RenderPass
    {
    public:
        void Draw(RenderContext& context, const std::weak_ptr<Viewport>& viewport) override
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
    };
}
