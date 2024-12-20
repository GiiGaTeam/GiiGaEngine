export module ShadowPass;

import <memory>;

import RenderPass;
import Viewport;

namespace GiiGa
{
    export class ShadowPass : public RenderPass
    {
    public:
        void Draw(RenderContext& context, const std::weak_ptr<Viewport>& viewport) override
        {
        }
    };
}
