export module IImGuiWindow;

import <imgui.h>;

namespace GiiGa
{
    export struct IImGuiWindow
    {
    public:
        virtual ~IImGuiWindow() = default;
        virtual void RecordImGui() = 0;
    };
}
