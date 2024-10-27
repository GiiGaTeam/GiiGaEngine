module;

#include <memory>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <SDL2/SDL_events.h>

export module Input;
import Window;

namespace GiiGa
{
export enum class Button
{
    KeyA,
    KeyW,
    KeyS,
    KeyD,
    KeySpace,

    MouseLeft,
    MouseRight,

    GamepadButtonA,
    GamepadButtonB,
    GamepadButtonX,
    GamepadButtonY
};

export class Input
{
public:
    struct ProcessedEvents
    {
        bool quit = false;
        bool close_window = false;
        Uint32 close_window_id = 0;
    };

    struct MousePosition
    {
        Sint32 x;
        Sint32 y;
    };

    static void Init(std::shared_ptr<Window> window)
    {
        ImGui_ImplSDL2_InitForD3D(window->GetSdlWindow());
    }

    static ProcessedEvents ProcessEvents()
    {
        RefreshButtons();

        SDL_Event event;
        ProcessedEvents return_event;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type)
            {
                case SDL_QUIT: 
                    return_event.quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    {
                        return_event.close_window = true;
                        return_event.close_window_id = event.window.windowID;
                    }
                    break;
                case SDL_KEYUP:
                case SDL_KEYDOWN: 
                    HandleSDLKeyboard(event.key);
                    break;
                case SDL_MOUSEMOTION: 
                    mouse_position = MousePosition(event.motion.x, event.motion.y);
                    break;
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN: 
                    HandleSDLMouseButton(event.button);
                    break;
            }
        }

        return return_event;
    }

    static bool IsKeyHeld(Button button) { return GetButtonState(button) == ButtonState::Held; }
    static bool IsKeyDown(Button button) { return GetButtonState(button) == ButtonState::Pressed; }
    static bool IsKeyUp(Button button) { return GetButtonState(button) == ButtonState::Released; }

    static bool IsKeyPressed(Button button)
    {
        return GetButtonState(button) == ButtonState::Pressed || GetButtonState(button) == ButtonState::Held;
    }

    static float Get1DAxis(Button positive, Button negative)
    {
        return (IsKeyHeld(positive) ? 1.0f : 0.0f) - (IsKeyHeld(negative) ? 1.0f : 0.0f);
    }

    static std::tuple<float, float> Get2DAxis(Button positiveX, Button negativeX, Button positiveY, Button negativeY)
    {
        float x = Get1DAxis(positiveX, negativeX);
        float y = Get1DAxis(positiveY, negativeY);

        return std::make_tuple(x, y);
    }

    static MousePosition GetMousePosition()
    {
        return mouse_position;
    }

private:
    static SDL_Keycode ConvertButtonToSDLKey(Button button)
    {
        switch (button)
        {
            case Button::KeyA: return SDLK_a;
            case Button::KeyW: return SDLK_w;
            case Button::KeyS: return SDLK_s;
            case Button::KeyD: return SDLK_d;
            case Button::KeySpace: return SDLK_SPACE;
            default: return SDLK_UNKNOWN;
        }
    }

    static Uint8 ConvertButtonToSDLMouseButtonIndex(Button button)
    {
        switch (button)
        {
            case Button::MouseLeft: return SDL_BUTTON_LEFT;
            case Button::MouseRight: return SDL_BUTTON_RIGHT;
            default: return SDLK_UNKNOWN;
        }
    }

    enum class ButtonState
    {
        None,
        Pressed,
        Released,
        Held
    };

    struct ButtonInfo
    {
        ButtonState state = ButtonState::None;
    };

    static void ChangeButtonState(ButtonInfo& info, bool pressed)
    {
        if (pressed && (info.state == ButtonState::None || info.state == ButtonState::Released))
        {
            info.state = ButtonState::Pressed;
        }
        else if (pressed)
        {
            info.state = ButtonState::Held;
        }
        else if (!pressed && (
                     info.state == ButtonState::Pressed || info.state == ButtonState::Held))
        {
            info.state = ButtonState::Released;
        }
        else
        {
            info.state = ButtonState::None;
        }
    }

    static void HandleSDLKeyboard(const SDL_KeyboardEvent& keyboard_event)
    {
        ButtonInfo button_info = key_button_infos[keyboard_event.keysym.sym];
        ChangeButtonState(button_info, keyboard_event.state == SDL_PRESSED);
        SetButtonInfo(keyboard_event.keysym.sym, std::move(button_info));
    }

    static void HandleSDLMouseButton(const SDL_MouseButtonEvent& mouse_button_event)
    {
        ButtonInfo button_info = GetButtonInfo(mouse_button_event.button);
        ChangeButtonState(button_info, mouse_button_event.state == SDL_PRESSED);
        SetButtonInfo(mouse_button_event.button, std::move(button_info));
    }

    static ButtonState GetButtonState(Button button)
    {
        auto sdl_key = ConvertButtonToSDLKey(button);
        if (sdl_key != SDLK_UNKNOWN)
        {
            return GetButtonInfo(sdl_key).state;
        }
        auto sdl_mouse_button = ConvertButtonToSDLMouseButtonIndex(button);
        if (sdl_mouse_button != SDLK_UNKNOWN)
        {
            return GetButtonInfo(sdl_mouse_button).state;
        }
        return ButtonState::None;
    }

    static ButtonInfo& GetButtonInfo(SDL_Keycode keycode)
    {
        if (!key_button_infos.contains(keycode))
        {
            key_button_infos.emplace(keycode, ButtonInfo());
        }
        return key_button_infos[keycode];
    }

    static void SetButtonInfo(SDL_Keycode keycode, ButtonInfo&& button_info)
    {
        if (!key_button_infos.contains(keycode))
        {
            key_button_infos.emplace(keycode, ButtonInfo());
        }
        key_button_infos[keycode] = std::move(button_info);
    }

    static ButtonInfo& GetButtonInfo(Uint8 mouse_button_index)
    {
        if (!mouse_button_infos.contains(mouse_button_index))
        {
            mouse_button_infos.emplace(mouse_button_index, ButtonInfo());
        }
        return mouse_button_infos[mouse_button_index];
    }

    static void SetButtonInfo(Uint8 mouse_button_index, ButtonInfo&& button_info)
    {
        if (!mouse_button_infos.contains(mouse_button_index))
        {
            mouse_button_infos.emplace(mouse_button_index, ButtonInfo());
        }
        mouse_button_infos[mouse_button_index] = std::move(button_info);
    }

    static void RefreshButtons()
    {
        for (auto i : key_button_infos)
        {
            ButtonInfo button_info = i.second;
            if (button_info.state == ButtonState::Released)
            {
                button_info.state = ButtonState::None;
                SetButtonInfo(i.first, std::move(button_info));
            }
        }
        for (auto i : mouse_button_infos)
        {
            ButtonInfo button_info = i.second;
            if (button_info.state == ButtonState::Released)
            {
                button_info.state = ButtonState::None;
                SetButtonInfo(i.first, std::move(button_info));
            }
        }
    }

    static inline std::unordered_map<SDL_Keycode, ButtonInfo> key_button_infos;
    static inline std::unordered_map<Uint8, ButtonInfo> mouse_button_infos;
    static inline MousePosition mouse_position;
};
}