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
    Unknown,

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
                case SDL_KEYDOWN: 
                    ChangeButtonState(SDLScancodeToButton(event.key.keysym.scancode), true); 
                    break;
                case SDL_KEYUP: 
                    ChangeButtonState(SDLScancodeToButton(event.key.keysym.scancode), false); 
                    break;
                case SDL_MOUSEMOTION: 
                    mouse_position_ = MousePosition(event.motion.x, event.motion.y);
                    break;
                case SDL_MOUSEBUTTONDOWN: 
                    ChangeButtonState(SDLButtonToButton(event.button.button), true); 
                    break;
                case SDL_MOUSEBUTTONUP:
                    ChangeButtonState(SDLButtonToButton(event.button.button), false); 
                    break;
            }
        }

        return return_event;
    }

    static bool IsKeyHeld(Button button) { 
        auto it = button_states_.find(button);
        return it != button_states_.end() && it->second.is_held;
    }

    static bool IsKeyDown(Button button) { 
        auto it = button_states_.find(button);
        return it != button_states_.end() && it->second.pressed;
    }

    static bool IsKeyUp(Button button) { 
        auto it = button_states_.find(button);
        return it != button_states_.end() && it->second.released;
    }

    static bool IsKeyPressed(Button button)
    { return IsKeyHeld(button) || IsKeyDown(button);
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
        return mouse_position_;
    }

private:
    struct ButtonState
    {
        bool is_held = false;
        bool pressed = false;
        bool released = false;
    };

     static Button SDLScancodeToButton(SDL_Scancode scancode)
    {
        switch (scancode)
        {
            case SDL_SCANCODE_A: return Button::KeyA;
            case SDL_SCANCODE_W: return Button::KeyW;
            case SDL_SCANCODE_S: return Button::KeyS;
            case SDL_SCANCODE_D: return Button::KeyD;
            case SDL_SCANCODE_SPACE: return Button::KeySpace;
            default: return Button::Unknown; 
        }
    }

    static Button SDLButtonToButton(Uint8 sdl_button)
    {
        switch (sdl_button)
        {
            case SDL_BUTTON_LEFT: return Button::MouseLeft;
            case SDL_BUTTON_RIGHT: return Button::MouseRight;
            default: return Button::Unknown;
        }
    }

    static void ChangeButtonState(Button button, bool pressed)
    {
        auto& state = button_states_[button];

        if (pressed)
        {
            if (!state.is_held)
            {
                state.pressed = true;
            }
            state.is_held = true;
        }
        else
        {
            if (state.is_held)
            {
                state.released = true;
            }
            state.is_held = false;
        }
    }

    static void RefreshButtons()
    {
        for (auto& [button, state] : button_states_)
        {
            state.pressed = false;
            state.released = false;
        }
    }

    static inline std::unordered_map<Button, ButtonState> button_states_;
    static inline MousePosition mouse_position_;
};
}