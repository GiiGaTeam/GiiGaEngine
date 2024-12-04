module;
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
        RenderPass(const std::weak_ptr<SceneVisibility>& scene_visibility) : scene_visibility_(scene_visibility)
        {
        };

        virtual ~RenderPass() = default;

        virtual void Draw() =0;

    protected:
        int32_t default_filter_type_ = Static | Dynamic | Opacity | Transparency;
        std::weak_ptr<SceneVisibility> scene_visibility_;

    };
}