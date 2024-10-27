module;

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <windows.h>  // Для типа HWND

export module Window;

import WindowSettings;
class WindowManager;

namespace GiiGa
{
export class Window
{
public:
    Window(SDL_Window* window, WindowSettings&& settings) 
        : window_(window)
        , settings_(settings) 
    {}

    SDL_Window* GetSdlWindow() const {
        return window_;
    }

    HWND GetHandle() const { 
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);

        // TODO: Добавить проверку результата функции на успешность
        SDL_GetWindowWMInfo(window_, &wmInfo);
        return wmInfo.info.win.window;
    }

    ~Window() { 
        SDL_DestroyWindow(window_);
    }
private:
    SDL_Window* window_;
    WindowSettings settings_;
};
}