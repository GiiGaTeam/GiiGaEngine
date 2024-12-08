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
    export class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Draw(RenderContext& context)
        {
            for (auto&& child : children_)
            {
                child->Draw(context);
            }
        }

        void AddChild(std::unique_ptr<RenderPass> pass)
        {
            children_.push_back(std::move(pass));
        }

    protected:
        int32_t default_filter_type_ = Static | Dynamic | Opacity | Transparency;
        std::vector<std::unique_ptr<RenderPass>> children_;
    };
}
