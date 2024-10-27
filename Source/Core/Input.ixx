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
        SDL_Event event;
        ProcessedEvents return_event;
        RefreshButtons();
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
            {
                return_event.quit = true;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                return_event.close_window = true;
                return_event.close_window_id = event.window.windowID;
            }
            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            {
                HandleSDLKeyboard(event.key);
            }
            if(event.type == SDL_MOUSEMOTION)
            {
                mouse_position = MousePosition(event.motion.x, event.motion.y);
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
        ButtonState state;
    };

    static void HandleSDLKeyboard(const SDL_KeyboardEvent& keyboard_event)
    {
        ButtonInfo button_info = button_map[keyboard_event.keysym.sym];
        if (keyboard_event.state == SDL_PRESSED && (button_info.state == ButtonState::None || button_info.state == ButtonState::Released))
        {
            button_info.state = ButtonState::Pressed;
        }
        else if (keyboard_event.state == SDL_PRESSED)
        {
            button_info.state = ButtonState::Held;
        }
        else if (keyboard_event.state == SDL_RELEASED && (
                     button_info.state == ButtonState::Pressed || button_info.state == ButtonState::Held))
        {
            button_info.state = ButtonState::Released;
        }
        else
        {
            button_info.state = ButtonState::None;
        }
        SetButtonInfo(keyboard_event.keysym.sym, std::move(button_info));
    }

    static ButtonState GetButtonState(Button button)
    {
        auto sdl_key = ConvertButtonToSDLKey(button);
        return GetButtonInfo(sdl_key).state;
    }

    static ButtonInfo& GetButtonInfo(SDL_Keycode keycode)
    {
        if (!button_map.contains(keycode))
        {
            button_map.emplace(keycode, ButtonInfo());
        }
        return button_map[keycode];
    }

    static void SetButtonInfo(SDL_Keycode keycode, ButtonInfo&& button_info)
    {
        if (!button_map.contains(keycode))
        {
            button_map.emplace(keycode, ButtonInfo());
        }
        button_map[keycode] = std::move(button_info);
    }

    static void RefreshButtons()
    {
        for (auto i : button_map)
        {
            ButtonInfo button_info = i.second;
            if (button_info.state == ButtonState::Released)
            {
                button_info.state = ButtonState::None;
                SetButtonInfo(i.first, std::move(button_info));
            }
        }
    }

    static inline std::unordered_map<SDL_Keycode, ButtonInfo> button_map;
    static inline MousePosition mouse_position;
};
}