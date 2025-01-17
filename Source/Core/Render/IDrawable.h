#pragma once

namespace GiiGa
{
    class IDrawable
    {
    public:
        virtual RenderData GetRenderData() = 0;
    };
}
