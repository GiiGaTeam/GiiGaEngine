export module RenderGraph;

import <vector>;
import <memory>;

import RenderPass;
import Viewport;

namespace GiiGa
{
    //class Viewport;

    export class RenderGraph
    {
    public:
        virtual ~RenderGraph() = default;

        virtual void Draw(RenderContext& context, const std::weak_ptr<Viewport>& viewport)
        {
            for (auto&& pass : passes_)
            {
                pass->Draw(context, viewport);
            }
        }

        void AddPass(const std::shared_ptr<RenderPass>&& pass)
        {
            passes_.push_back(std::move(pass));
        }
        
    protected:
        std::vector<std::shared_ptr<RenderPass>> passes_;
    };
}