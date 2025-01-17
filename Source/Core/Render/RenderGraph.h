#pragma once
#include<vector>
#include<memory>

#include<RenderPass.h>
#include<Viewport.h>

namespace GiiGa
{
    //class Viewport;

    class RenderGraph
    {
    public:
        virtual ~RenderGraph() = default;

        virtual void Draw(RenderContext& context)
        {
            for (auto&& pass : passes_)
            {
                pass->Draw(context);
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