#pragma once

#include <RenderTypes.h>

namespace GiiGa
{
    // Minimal device abstraction. Under NVRHI the device owns descriptor heaps,
    // upload staging and resource lifetime (ref-counted + deferred), so the old
    // frames-in-flight delete queue is gone.
    struct IRenderDevice
    {
        virtual ~IRenderDevice() = default;

        virtual nvrhi::IDevice* GetNvrhiDevice() = 0;
    };
}
