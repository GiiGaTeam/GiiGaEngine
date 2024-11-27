module;

export module RenderTypes;

namespace GiiGa
{
    export typedef enum
    {
        Static = 0x1L,
        Dynamic = 0x2L,
        Opacity = 0x4L,
        Transparency = 0x8L,
        Unknown = 0x10L,
        Unknown2 = 0x20L
    } RenderFilterType;
}