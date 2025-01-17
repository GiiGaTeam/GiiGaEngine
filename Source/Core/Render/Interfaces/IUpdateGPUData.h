#pragma once

namespace GiiGa
{
    struct IUpdateGPUData
    {
        virtual ~IUpdateGPUData() = default;

        virtual void UpdateGPUData(RenderContext& Context) =0;
    };
}
