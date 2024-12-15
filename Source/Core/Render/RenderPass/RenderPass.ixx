module;
#include <memory>
#include <vector>

export module RenderPass;
import IRenderable;
import RenderTypes;
import SceneVisibility;
export import RenderContext;

namespace GiiGa
{
    export class Viewport;

    export class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Draw(RenderContext& context, const std::weak_ptr<Viewport>& viewport)=0;

    protected:
        int32_t default_filter_type_ = Static | Dynamic | Opacity | Transparency;
    };
}
