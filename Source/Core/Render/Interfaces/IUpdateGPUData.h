#pragma once

// IUpdateGPUData.h — objects that push per-frame data to the GPU.
// NVRHI cutover: takes the nvrhi device + command list directly instead of the legacy
// RenderContext bundle.

namespace nvrhi
{
    class IDevice;
    class ICommandList;
}

namespace GiiGa
{
    struct IUpdateGPUData
    {
        virtual ~IUpdateGPUData() = default;

        virtual void UpdateGPUData(nvrhi::IDevice* device, nvrhi::ICommandList* cmd) = 0;
    };
}
