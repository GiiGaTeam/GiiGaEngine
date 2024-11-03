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
export enum class KeyCode
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
};

export enum class GamepadCode
{
    Unknown,
    ButtonA,
    ButtonB,
    ButtonX,
    ButtonY,
    ButtonBack,
    ButtonGuide,
    ButtonStart,
    ButtonLeftStick,
    ButtonRightStick,
    ButtonLeftShoulder,
    ButtonRightShoulder,
    DPadUp,
    DPadDown,
    DPadLeft,
    DPadRight,
};

export enum class MouseButton
{
    Unknown,
    MouseLeft,
    MouseRight,
    MouseMiddle,
};

static std::array<KeyCode, SDL_NUM_SCANCODES> SCANCODE_TO_BUTTON_MAP;
void InitializeScancodeMap();

export class Input
{
private:
    struct ButtonState
    {
        bool is_held = false;
        bool pressed = false;
        bool released = false;
    };

    
    struct GamepadInfo
    {
        SDL_GameController* controller = nullptr;
        std::tuple<float, float> left_trigger_axis;
        std::tuple<float, float> right_trigger_axis;
        std::unordered_map<GamepadCode, ButtonState> button_states;
    };

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

        for (int i = 0; i < SDL_NumJoysticks(); ++i)
        {
            if (SDL_IsGameController(i))
            {
                AddController(i);
            }
        }
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
                    ChangeButtonState(event.cdevice.which, SDLControllerButtonToButton((SDL_GameControllerButton)event.cbutton.button), true); 
                    break;
                case SDL_CONTROLLERBUTTONUP: 
                    ChangeButtonState(event.cdevice.which, SDLControllerButtonToButton((SDL_GameControllerButton)event.cbutton.button), false); 
                    break;
                case SDL_CONTROLLERAXISMOTION:
                {
                    auto& gamepad = gamepads_[event.cdevice.which];
                    if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
                    {
                        if (abs(event.caxis.value) < dead_zone_) continue;
                        std::get<0>(gamepad.left_trigger_axis) = event.caxis.value / 32767.0f;
                    }
                    else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
                    {
                        if (abs(event.caxis.value) < dead_zone_) continue;
                        std::get<1>(gamepad.left_trigger_axis) = event.caxis.value / 32767.0f;
                    }
                    else if (event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTX)
                    {
                        if (abs(event.caxis.value) < dead_zone_) continue;
                        std::get<0>(gamepad.right_trigger_axis) = event.caxis.value / 32767.0f;
                    }
                    else if (event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY)
                    {
                        if (abs(event.caxis.value) < dead_zone_) continue;
                        std::get<1>(gamepad.right_trigger_axis) = event.caxis.value / 32767.0f;
                    }
                    break;
                }
                case SDL_CONTROLLERDEVICEADDED: 
                    AddController(event.cdevice.which);
                    break;
                case SDL_CONTROLLERDEVICEREMOVED: 
                    RemoveController(event.cdevice.which);
                    break;
            }
        }

        return return_event;
    }

    static bool IsKeyHeld(KeyCode button) { 
        auto it = keyboard_states_.find(button);
        return it != keyboard_states_.end() && it->second.is_held;
    }

    static bool IsKeyDown(KeyCode button) { 
        auto it = keyboard_states_.find(button);
        return it != keyboard_states_.end() && it->second.pressed;
    }

    static bool IsKeyUp(KeyCode button) { 
        auto it = keyboard_states_.find(button);
        return it != keyboard_states_.end() && it->second.released;
    }

    static bool IsKeyPressed(KeyCode button)
    { 
        return IsKeyHeld(button) || IsKeyDown(button);
    }

    static bool IsKeyHeld(MouseButton button)
    {
        auto it = mouse_buttons_states_.find(button);
        return it != mouse_buttons_states_.end() && it->second.is_held;
    }

    static bool IsKeyDown(MouseButton button)
    {
        auto it = mouse_buttons_states_.find(button);
        return it != mouse_buttons_states_.end() && it->second.pressed;
    }

    static bool IsKeyUp(MouseButton button)
    {
        auto it = mouse_buttons_states_.find(button);
        return it != mouse_buttons_states_.end() && it->second.released;
    }

    static bool IsKeyPressed(MouseButton button) { 
        return IsKeyHeld(button) || IsKeyDown(button); 
    }

    static bool IsKeyHeld(int32_t index, GamepadCode button)
    {
        auto it = gamepads_[index].button_states.find(button);
        return it != gamepads_[index].button_states.end() && it->second.is_held;
    }

    static bool IsKeyDown(int32_t index, GamepadCode button)
    {
        auto it = gamepads_[index].button_states.find(button);
        return it != gamepads_[index].button_states.end() && it->second.pressed;
    }

    static bool IsKeyUp(int32_t index, GamepadCode button)
    {
        auto it = gamepads_[index].button_states.find(button);
        return it != gamepads_[index].button_states.end() && it->second.released;
    }

    static bool IsKeyPressed(int32_t index, GamepadCode button) {
        return IsKeyHeld(index, button) || IsKeyDown(index, button); 
    }

    static float Get1DAxis(KeyCode positive, KeyCode negative)
    {
        return (IsKeyHeld(positive) ? 1.0f : 0.0f) - (IsKeyHeld(negative) ? 1.0f : 0.0f);
    }

    static std::tuple<float, float> Get2DAxis(KeyCode positiveX, KeyCode negativeX, KeyCode positiveY, KeyCode negativeY)
    {
        float x = Get1DAxis(positiveX, negativeX);
        float y = Get1DAxis(positiveY, negativeY);

        return std::make_tuple(x, y);
    }

    static std::tuple<float, float> Get2DAxis(int32_t index, GamepadCode button)
    { 
        if (button == GamepadCode::ButtonLeftStick)
        {
            return gamepads_[index].left_trigger_axis;
        }  

        if (button == GamepadCode::ButtonRightStick)
        {
            return gamepads_[index].right_trigger_axis;
        }  

        return std::make_tuple(0.0f, 0.0f);
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

    ~Input()
    {
        for (auto& [id, info] : gamepads_)
        {
            if (info.controller)
            {
                SDL_GameControllerClose(info.controller);
            }
        }
    }

private:

    static MouseButton SDLButtonToButton(Uint8 sdl_button)
    {
        switch (sdl_button)
        {
            case SDL_BUTTON_LEFT: return MouseButton::MouseLeft;
            case SDL_BUTTON_RIGHT: return MouseButton::MouseRight;
            case SDL_BUTTON_MIDDLE: return MouseButton::MouseMiddle;
            default: return MouseButton::Unknown;
        }
    }
    
    
    static KeyCode SDLScancodeToButton(SDL_Scancode scancode)
    {
        if (scancode >= 0 && scancode < SDL_NUM_SCANCODES)
        {
            return SCANCODE_TO_BUTTON_MAP[scancode];
        }
        return KeyCode::Unknown;
    }

    static GamepadCode SDLControllerButtonToButton(SDL_GameControllerButton button)
    {
        switch (button)
        {
            case SDL_CONTROLLER_BUTTON_A: return GamepadCode::ButtonA;
            case SDL_CONTROLLER_BUTTON_B: return GamepadCode::ButtonB;
            case SDL_CONTROLLER_BUTTON_X: return GamepadCode::ButtonX;
            case SDL_CONTROLLER_BUTTON_Y: return GamepadCode::ButtonY;
            case SDL_CONTROLLER_BUTTON_BACK: return GamepadCode::ButtonBack;
            case SDL_CONTROLLER_BUTTON_GUIDE: return GamepadCode::ButtonGuide;
            case SDL_CONTROLLER_BUTTON_START: return GamepadCode::ButtonStart;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: return GamepadCode::ButtonLeftStick;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return GamepadCode::ButtonRightStick;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return GamepadCode::ButtonLeftShoulder;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return GamepadCode::ButtonRightShoulder;
            case SDL_CONTROLLER_BUTTON_DPAD_UP: return GamepadCode::DPadUp;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return GamepadCode::DPadDown;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return GamepadCode::DPadLeft;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return GamepadCode::DPadRight;
            default: return GamepadCode::Unknown;
        }
    }

    static void ChangeButtonState(KeyCode button, bool pressed)
    {
        auto& state = keyboard_states_[button];
        ChangeButtonState(state, pressed);
    }

    static void ChangeButtonState(MouseButton button, bool pressed)
    {
        auto& state = mouse_buttons_states_[button];
        ChangeButtonState(state, pressed);
    }

    static void ChangeButtonState(int32_t index, GamepadCode button, bool pressed)
    {
        auto& state = gamepads_[index].button_states[button];
        ChangeButtonState(state, pressed);
    }

    static void ChangeButtonState(ButtonState& state, bool pressed)
    {
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

    static void AddController(int gamepad_idx)
    {
         if (SDL_IsGameController(gamepad_idx))
         {
             SDL_GameController* controller = SDL_GameControllerOpen(gamepad_idx);
             if (controller)
             {
                 int joystick_id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
                 gamepads_[joystick_id] = GamepadInfo{controller};
             }
         }
    }

    static void RemoveController(int gamepad_idx)
    {
        auto it = gamepads_.find(gamepad_idx);
        if (it != gamepads_.end())
        {
            SDL_GameControllerClose(it->second.controller);
            gamepads_.erase(it);
        }
    }

    static void ResetState()
    {
        for (auto& [button, state] : keyboard_states_)
        {
            state.pressed = false;
            state.released = false;
        }

        for (auto& [button, state] : mouse_buttons_states_)
        {
            state.pressed = false;
            state.released = false;
        }

        for (auto& [idx, gamepad] : gamepads_)
        {
            gamepad.left_trigger_axis = std::make_tuple(0.0f, 0.0f);
            gamepad.right_trigger_axis = std::make_tuple(0.0f, 0.0f);

            for (auto& [button, state] : gamepad.button_states)
            {
                state.pressed = false;
                state.released = false;
            }
        }

        mouse_delta_ = MouseDelta(0.0f, 0.0f);
        mouse_wheel_ = MouseWheel(0.0f, 0.0f);
    }

    static inline std::unordered_map<KeyCode, ButtonState> keyboard_states_;
    static inline std::unordered_map<MouseButton, ButtonState> mouse_buttons_states_;

    static inline MouseWheel mouse_wheel_;
    static inline MouseDelta mouse_delta_;
    static inline MousePosition mouse_position_;

    static inline float dead_zone_ = 8000.0;

    static inline std::unordered_map<int, GamepadInfo> gamepads_;
};

void InitializeScancodeMap()
{
    SCANCODE_TO_BUTTON_MAP.fill(KeyCode::Unknown);

    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_A] = KeyCode::KeyA;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_B] = KeyCode::KeyB;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_C] = KeyCode::KeyC;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_D] = KeyCode::KeyD;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_E] = KeyCode::KeyE;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F] = KeyCode::KeyF;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_G] = KeyCode::KeyG;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_H] = KeyCode::KeyH;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_I] = KeyCode::KeyI;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_J] = KeyCode::KeyJ;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_K] = KeyCode::KeyK;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_L] = KeyCode::KeyL;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_M] = KeyCode::KeyM;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_N] = KeyCode::KeyN;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_O] = KeyCode::KeyO;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_P] = KeyCode::KeyP;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_Q] = KeyCode::KeyQ;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_R] = KeyCode::KeyR;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_S] = KeyCode::KeyS;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_T] = KeyCode::KeyT;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_U] = KeyCode::KeyU;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_V] = KeyCode::KeyV;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_W] = KeyCode::KeyW;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_X] = KeyCode::KeyX;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_Y] = KeyCode::KeyY;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_Z] = KeyCode::KeyZ;

    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_1] = KeyCode::Key1;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_2] = KeyCode::Key2;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_3] = KeyCode::Key3;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_4] = KeyCode::Key4;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_5] = KeyCode::Key5;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_6] = KeyCode::Key6;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_7] = KeyCode::Key7;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_8] = KeyCode::Key8;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_9] = KeyCode::Key9;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_0] = KeyCode::Key0;

    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F1] = KeyCode::KeyF1;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F2] = KeyCode::KeyF2;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F3] = KeyCode::KeyF3;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F4] = KeyCode::KeyF4;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F5] = KeyCode::KeyF5;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F6] = KeyCode::KeyF6;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F7] = KeyCode::KeyF7;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F8] = KeyCode::KeyF8;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F9] = KeyCode::KeyF9;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F10] = KeyCode::KeyF10;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F11] = KeyCode::KeyF11;
    SCANCODE_TO_BUTTON_MAP[SDL_SCANCODE_F12] = KeyCode::KeyF12;
}

}