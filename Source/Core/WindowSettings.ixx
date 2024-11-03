module;

#include <string>

export module WindowSettings;

namespace GiiGa
{
export struct WindowSettings
{
    std::string title;
    int width = 800;
    int height = 640;
};
}