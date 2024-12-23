export module IUpdateGPUData;

import RenderContext;

namespace GiiGa
{
    export struct IUpdateGPUData
    {
        virtual ~IUpdateGPUData() = default;

        virtual void UpdateGPUData(RenderContext& Context) =0;
    };
}
