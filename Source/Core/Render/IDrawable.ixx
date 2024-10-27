module;

export module IDrawable;
import RenderSystem;

namespace GiiGa
{
    export class IDrawable
    {
    public:
        virtual RenderData GetRenderData() = 0;
    };
}
