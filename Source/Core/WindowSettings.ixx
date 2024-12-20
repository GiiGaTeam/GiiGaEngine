export module WindowSettings;

import <string>;

namespace GiiGa
{
export struct WindowSettings
{
    std::string title;
    int width = 800;
    int height = 640;
};
}