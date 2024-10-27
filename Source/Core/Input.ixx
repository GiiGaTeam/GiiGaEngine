module;

#include <array>
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
    KeyB,
    KeyC,
    KeyD,
    KeyE,
    KeyF,
    KeyG,
    KeyH,
    KeyI,
    KeyJ,
    KeyK,
    KeyL,
    KeyM,
    KeyN,
    KeyO,
    KeyP,
    KeyQ,
    KeyR,
    KeyS,
    KeyT,
    KeyU,
    KeyV,
    KeyW,
    KeyX,
    KeyY,
    KeyZ,

    Key0,
    Key1,
    Key2,
    Key3,
    Key4,
    Key5,
    Key6,
    Key7,
    Key8,
    Key9,

    KeyF1,
    KeyF2,
    KeyF3,
    KeyF4,
    KeyF5,
    KeyF6,
    KeyF7,
    KeyF8,
    KeyF9,
    KeyF10,
    KeyF11,
    KeyF12,

    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,

    KeyShift,
    KeyControl,
    KeyAlt,
    KeySuper,
    KeyCapsLock,
    KeyTab,

    KeySpace,
    KeyEnter,
    KeyBackspace,
    KeyEscape,
    KeyComma,
    KeyPeriod,
    KeySlash,
    KeySemicolon,
    KeyApostrophe,
    KeyLeftBracket,
    KeyRightBracket,
    KeyBackslash,
    KeyMinus,
    KeyEqual,

    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    NumpadDecimal,
    NumpadDivide,
    NumpadMultiply,
    NumpadSubtract,
    NumpadAdd,
    NumpadEnter,
    NumpadEqual,

    KeyInsert,
    KeyDelete,
    KeyHome,
    KeyEnd,
    KeyPageUp,
    KeyPageDown,
    KeyPrintScreen,
    KeyScrollLock,
    KeyPause,

    MouseLeft,
    MouseRight,
    MouseMiddle,

    GamepadButtonA,
    GamepadButtonB,
    GamepadButtonX,
    GamepadButtonY,
    GamepadButtonBack,
    GamepadButtonGuide,
    GamepadButtonStart,
    GamepadButtonLeftStick,
    GamepadButtonRightStick,
    GamepadButtonLeftShoulder,
    GamepadButtonRightShoulder,
    GamepadDPadUp,
    GamepadDPadDown,
    GamepadDPadLeft,
    GamepadDPadRight,
};

static std::array<Button, SDL_NUM_SCANCODES> SCANCODE_TO_BUTTON_MAP;
void InitializeScancodeMap();

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

    struct MouseDelta
    {
        float dx;
        float dy;
    };

     struct MouseWheel
    {
        float x;
        float y;
    };

    static void Init(std::shared_ptr<Window> window)
    {
        ImGui_ImplSDL2_InitForD3D(window->GetSdlWindow());
        InitializeScancodeMap();
    }

    static ProcessedEvents ProcessEvents()
    {
        ResetState();

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
                {
                    float dx = event.motion.x - mouse_position_.x;
                    float dy = event.motion.y - mouse_position_.y;

                    float len = sqrtf(dx * dx + dy * dy);

                    dx /= len;
                    dy /= len;

                    mouse_position_ = MousePosition(event.motion.x, event.motion.y);
                    mouse_delta_ = MouseDelta(dx, dy);

                    break;
                } 
                case SDL_MOUSEWHEEL: 
                    mouse_wheel_ = MouseWheel(event.wheel.x, event.wheel.y);
                    break;
                case SDL_MOUSEBUTTONDOWN: 
                    ChangeButtonState(SDLButtonToButton(event.button.button), true); 
                    break;
                case SDL_MOUSEBUTTONUP:
                    ChangeButtonState(SDLButtonToButton(event.button.button), false); 
                    break;
                case SDL_CONTROLLERBUTTONDOWN: 
                    ChangeButtonState(SDLControllerButtonToButton((SDL_GameControllerButton)event.cbutton.button), true); 
                    break;
                case SDL_CONTROLLERBUTTONUP: 
                    ChangeButtonState(SDLControllerButtonToButton((SDL_GameControllerButton)event.cbutton.button), false); 
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

    static MouseDelta GetMouseDelta() 
    { 
        return mouse_delta_; 
    }

    static MouseWheel GetMouseWhell() 
    {
        return mouse_wheel_; 
    }

private:
    struct ButtonState
    {
        bool is_held = false;
        bool pressed = false;
        bool released = false;
    };

    static Button SDLButtonToButton(Uint8 sdl_button)
    {
        switch (sdl_button)
        {
            case SDL_BUTTON_LEFT: return Button::MouseLeft;
            case SDL_BUTTON_RIGHT: return Button::MouseRight;
            case SDL_BUTTON_MIDDLE: return Button::MouseMiddle;
            default: return Button::Unknown;
        }
    }
    
    
    static Button SDLScancodeToButton(SDL_Scancode scancode)
    {
        if (scancode >= 0 && scancode < SDL_NUM_SCANCODES)
        {
            return SCANCODE_TO_BUTTON_MAP[scancode];
        }
        return Button::Unknown;
    }

    static Button SDLControllerButtonToButton(SDL_GameControllerButton button)
    {
        switch (button)
        {
            case SDL_CONTROLLER_BUTTON_A: return Button::GamepadButtonA;
            case SDL_CONTROLLER_BUTTON_B: return Button::GamepadButtonB;
            case SDL_CONTROLLER_BUTTON_X: return Button::GamepadButtonX;
            case SDL_CONTROLLER_BUTTON_Y: return Button::GamepadButtonY;
            case SDL_CONTROLLER_BUTTON_BACK: return Button::GamepadButtonBack;
            case SDL_CONTROLLER_BUTTON_GUIDE: return Button::GamepadButtonGuide;
            case SDL_CONTROLLER_BUTTON_START: return Button::GamepadButtonStart;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: return Button::GamepadButtonLeftStick;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return Button::GamepadButtonRightStick;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return Button::GamepadButtonLeftShoulder;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return Button::GamepadButtonRightShoulder;
            case SDL_CONTROLLER_BUTTON_DPAD_UP: return Button::GamepadDPadUp;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return Button::GamepadDPadDown;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return Button::GamepadDPadLeft;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return Button::GamepadDPadRight;
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

    static void ResetState()
    {
        for (auto& [button, state] : button_states_)
        {
            state.pressed = false;
            state.released = false;
        }

        mouse_delta_ = MouseDelta(0.0f, 0.0f);
        mouse_wheel_ = MouseWheel(0.0f, 0.0f);
    }

    static inline MouseWheel mouse_wheel_;
    static inline std::unordered_map<Button, ButtonState> button_states_;
    static inline MouseDelta mouse_delta_;
    static inline MousePosition mouse_position_;
};

void InitializeScancodeMap()
{
    SCANCODE_TO_BUTTON_MAP.fill(Button::Unknown);

    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_A] = Button::KeyA;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_B] = Button::KeyB;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_C] = Button::KeyC;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_D] = Button::KeyD;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_E] = Button::KeyE;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F] = Button::KeyF;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_G] = Button::KeyG;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_H] = Button::KeyH;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_I] = Button::KeyI;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_J] = Button::KeyJ;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_K] = Button::KeyK;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_L] = Button::KeyL;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_M] = Button::KeyM;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_N] = Button::KeyN;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_O] = Button::KeyO;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_P] = Button::KeyP;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_Q] = Button::KeyQ;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_R] = Button::KeyR;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_S] = Button::KeyS;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_T] = Button::KeyT;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_U] = Button::KeyU;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_V] = Button::KeyV;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_W] = Button::KeyW;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_X] = Button::KeyX;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_Y] = Button::KeyY;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_Z] = Button::KeyZ;

    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_1] = Button::Key1;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_2] = Button::Key2;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_3] = Button::Key3;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_4] = Button::Key4;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_5] = Button::Key5;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_6] = Button::Key6;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_7] = Button::Key7;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_8] = Button::Key8;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_9] = Button::Key9;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_0] = Button::Key0;

    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F1] = Button::KeyF1;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F2] = Button::KeyF2;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F3] = Button::KeyF3;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F4] = Button::KeyF4;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F5] = Button::KeyF5;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F6] = Button::KeyF6;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F7] = Button::KeyF7;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F8] = Button::KeyF8;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F9] = Button::KeyF9;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F10] = Button::KeyF10;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F11] = Button::KeyF11;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F12] = Button::KeyF12;
}

}