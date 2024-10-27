module;
#include <memory>

export module WindowManager;

export import Window;
export import WindowSettings;

namespace GiiGa
{
export class WindowManager
{
public:
    static std::shared_ptr<Window> CreateWindow(const WindowSettings& window_settings)
    {
        
    }
};
}