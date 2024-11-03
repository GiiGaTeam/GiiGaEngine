module;

export module RenderSystemSettings;
#include <OCIdl.h>

namespace GiiGa
{
    export struct RenderSystemSettings
    {
        // default value 3, can be changed 
        // on engine startup and settings parse
        static inline UINT8 NUM_FRAMES_IN_FLIGHT = 4;
        static inline UINT8 NUM_BACK_BUFFERS = 3;
        static inline UINT32 NUM_CBV_SRV_UAVs = 1 << 16;
    };
}