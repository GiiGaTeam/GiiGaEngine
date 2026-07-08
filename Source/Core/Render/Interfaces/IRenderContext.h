#pragma once
#include<memory>
#include<span>
#include<cstdint>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>

namespace GiiGa
{
    struct IRenderContext
    {
        virtual ~IRenderContext() = default;

        // Queues a buffer upload that is flushed at the start of the next frame.
        // Safe to call while no command list is recording (e.g. during asset loading).
        virtual void WriteBufferDeffered(nvrhi::BufferHandle buffer, const std::span<const uint8_t>& data) = 0;

        // Writes a buffer right now; requires an open (recording) command list.
        virtual void WriteBufferImmediate(nvrhi::IBuffer* buffer, const std::span<const uint8_t>& data) = 0;

        virtual nvrhi::ICommandList* CommandList() = 0;
    };
}
