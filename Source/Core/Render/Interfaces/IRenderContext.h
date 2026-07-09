#pragma once

#include <RenderTypes.h>

namespace GiiGa
{
    // Minimal frame-recording context abstraction over an nvrhi command list.
    // Barriers/uploads are handled by nvrhi (automatic state tracking + writeBuffer),
    // so the old deferred-upload/barrier surface is gone.
    struct IRenderContext
    {
        virtual ~IRenderContext() = default;

        virtual nvrhi::ICommandList* GetCommandList() = 0;
    };
}
