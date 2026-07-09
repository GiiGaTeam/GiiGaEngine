#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cstdint>
#include <dxgiformat.h>

namespace GiiGa
{
    struct RenderSystemSettings
    {
        // default value 3, can be changed
        // on engine startup and settings parse
        static inline uint8_t NUM_FRAMES_IN_FLIGHT = 4;
        static inline uint8_t NUM_BACK_BUFFERS = 3;

        // Swap-chain back-buffer format (kept identical to the pre-NVRHI renderer).
        static inline DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    };
}
