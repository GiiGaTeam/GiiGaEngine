module;
#include <vector>
#include <memory>

export module RenderPass;
import IRenderable;
import RenderTypes;
import SceneVisibility;

namespace GiiGa
{

    export class RenderPass
    {
    public:
        RenderPass(const std::weak_ptr<SceneVisibility>& scene_visibility) : scene_visibility_(scene_visibility) {};

        virtual ~RenderPass() = default;

        void Draw()
        {
            if (scene_visibility_.expired()) return;
            const auto scene_vis = scene_visibility_.lock();
            
            std::vector<std::weak_ptr<IRenderable>> drawables_objects;
            scene_vis->Extract(default_filter_type_, drawables_objects);
            
            for (const auto& object : drawables_objects)
            {
                //TODO: PSO
                object.lock()->Draw();
            }
        }

    protected:
        int32_t default_filter_type_ = Static | Dynamic | Opacity | Transparency;
        std::weak_ptr<SceneVisibility> scene_visibility_;

    };
}