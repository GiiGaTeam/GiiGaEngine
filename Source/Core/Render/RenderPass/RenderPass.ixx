export module RenderPass;

import <memory>;
import <vector>;

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

        virtual void Draw(RenderContext& context)=0;

    protected:
        int32_t default_filter_type_ = Static | Dynamic | Opacity | Transparency;
    };
}
