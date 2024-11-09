module;
#include <memory>
#include <SDL2/SDL.h>

export module WindowManager;

export import Window;
export import WindowSettings;

namespace GiiGa
{
export class WindowManager
{
public:
    static std::shared_ptr<Window> CreateWindow(WindowSettings& window_settings)
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        {
            throw std::exception(SDL_GetError());
        }

        // TODO: Добавить проверку результата функции на успешность
        auto window = SDL_CreateWindow(
            window_settings.title.c_str(), 
            SDL_WINDOWPOS_CENTERED, 
            SDL_WINDOWPOS_CENTERED, 
            window_settings.width, 
            window_settings.height, 
            SDL_WINDOW_RESIZABLE 
        );

        return std::make_shared<Window>(window, std::move(window_settings));
    }
};
}