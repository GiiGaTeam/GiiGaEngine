module;

export module RenderSystemSettings;
#include <OCIdl.h>

namespace GiiGa
{
    export struct RenderSystemSettings
    {
        // default value 3, can be changed 
        // on engine startup and settings parse
        static inline UINT8 NUM_FRAMES_IN_FLIGHT = 3;
    };
}