#pragma once

namespace GiiGa
{
    struct IImGuiWindow
    {
    public:
        virtual ~IImGuiWindow() = default;
        virtual void RecordImGui() = 0;
    };
}
