﻿module;

#include <vector>
#include <memory>

export module GLightPass;
import RenderPass;
import IRenderable;
import Viewport;

namespace GiiGa
{
    export class GLightPass : public RenderPass
    {
        void Draw(RenderContext& context, const std::weak_ptr<Viewport>& viewport) override
        {
            std::vector<std::weak_ptr<IRenderable>> drawables_objects;
            //scene_vis->Extract(default_filter_type_, drawables_objects);

            for (const auto& object : drawables_objects)
            {
                //TODO: PSO
                object.lock()->Draw(context);
            }
        }
    };
}