#pragma once


#include<cstdint>

namespace GiiGa
{
    struct RenderSystemSettings
    {
        // default value 3, can be changed
        // on engine startup and settings parse
        static inline uint8_t NUM_FRAMES_IN_FLIGHT = 4;
        static inline uint8_t NUM_BACK_BUFFERS = 3;

        // Shader-visible SRV heap used exclusively by Dear ImGui
        // (fonts, editor icons, viewport textures).
        static inline uint32_t IMGUI_SRV_HEAP_SIZE = 1024;
    };
}
