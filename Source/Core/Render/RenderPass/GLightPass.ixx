module;

#include <vector>
#include <memory>

export module GLightPass;
import RenderPass;
import IRenderable;

namespace GiiGa
{
    export class GLightPass : public RenderPass
    {
        void Draw(RenderContext& context) override
        {
            std::vector<std::weak_ptr<IRenderable>> drawables_objects;
            //scene_vis->Extract(default_filter_type_, drawables_objects);

            for (const auto& object : drawables_objects)
            {
                //TODO: PSO
                object.lock()->Draw();
            }
        }
    };
}