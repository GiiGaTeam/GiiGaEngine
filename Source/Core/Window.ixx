module;

#include <array>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_events.h>
#include <imgui_impl_sdl2.h>
#include <windows.h>  // Для типа HWND

export module Window;

export import WindowSettings;
import EventSystem;

class WindowManager;

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

export enum class GamepadAxis
{
    X,
    Y
};

export enum class GamepadStick
{
    Left,
    Right
};

export struct BeginProcessEvent
{
};

export struct WindowCloseEvent
{
    uint32_t window_id;
};

export struct QuitEvent
{
};

export struct KeyEvent
{
    KeyCode key;
    bool is_down;
};

export struct MouseMotionEvent
{
    float x;
    float y;
};

export struct MouseWheelEvent
{
    float x;
    float y;
};

export struct MouseButtonEvent
{
    MouseButton button;
    bool is_down;
};

export struct GamepadButtonEvent
{
    int device_id;
    GamepadCode button;
    bool is_down;
};

export struct GamepadAxisMotionEvent
{
    int device_id;
    float value;
    GamepadAxis axis;
    GamepadStick stick;
};

export struct GamepadAddedEvent
{
    int device_id;
};

export struct GamepadRemovedEvent
{
    int device_id;
};

export struct WindowResizeEvent
{
    int width;
    int height;
};

static std::array<KeyCode, SDL_NUM_SCANCODES> SCANCODE_TO_BUTTON_MAP;
void InitializeScancodeMap();

export class Window
{
public:
    Window(SDL_Window* window, WindowSettings&& settings) 
        : window_(window)
        , settings_(settings) 
    { 
        InitializeScancodeMap();

        ImGui_ImplSDL2_InitForD3D(window);
    }

    void ProcessEvents() {
        OnBeginProcess.Invoke({});

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type)
            {
                case SDL_QUIT: 
                    OnQuit.Invoke(QuitEvent{});
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event)
                    {
                        case (SDL_WINDOWEVENT_CLOSE):
                            {
                                WindowCloseEvent t{event.window.windowID};

                                OnWindowClose.Invoke(t);
                            }
                            break;
                        case (SDL_WINDOWEVENT_SIZE_CHANGED):
                            {
                                WindowResizeEvent t{event.window.data1, event.window.data2};
                                OnWindowResize.Invoke(t);
                            }
                            break;
                        default: break;
                    }
                    break;
                case SDL_KEYDOWN: 
                    {
                        KeyEvent t{SDLScancodeToButton(event.key.keysym.scancode), true};
                        OnKey.Invoke(t);
                    }
                    break;
                case SDL_KEYUP: 
                    {
                        KeyEvent t{SDLScancodeToButton(event.key.keysym.scancode), false};
                        OnKey.Invoke(t);
                    }
                    break;
                case SDL_MOUSEMOTION:
                    {
                        MouseMotionEvent t{event.motion.x, event.motion.y};
                        OnMouseMotion.Invoke(t);
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    {
                        MouseWheelEvent t{event.wheel.x, event.wheel.y};
                        OnMouseWheel.Invoke(t);
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN: 
                    {
                        MouseButtonEvent t{SDLButtonToButton(event.button.button), true};
                        OnMouseButton.Invoke(t);
                    }
                    break;
                case SDL_MOUSEBUTTONUP: 
                    {
                        MouseButtonEvent t{SDLButtonToButton(event.button.button), false};
                        OnMouseButton.Invoke(t);
                    }
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    {
                        GamepadButtonEvent t{event.cdevice.which, SDLControllerButtonToButton((SDL_GameControllerButton)event.cbutton.button), true};
                        OnGamepadButton.Invoke(t);
                    }
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    {
                        GamepadButtonEvent t{event.cdevice.which, SDLControllerButtonToButton((SDL_GameControllerButton)event.cbutton.button), false};
                        OnGamepadButton.Invoke(t);
                    }
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    {
                        GamepadAxisMotionEvent t{};
                        t.device_id = event.cdevice.which;
                        t.value = event.caxis.value;
                        if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
                        {
                            t.axis = GamepadAxis::X;
                            t.stick = GamepadStick::Left;
                        }
                        else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
                        {
                            t.axis = GamepadAxis::Y;
                            t.stick = GamepadStick::Left;
                        }
                        else if (event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTX)
                        {
                            t.axis = GamepadAxis::X;
                            t.stick = GamepadStick::Right;
                        }
                        else if (event.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY)
                        {
                            t.axis = GamepadAxis::Y;
                            t.stick = GamepadStick::Right;
                        }
                        OnGamepadAxisMotion.Invoke(t);
                    }
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    {
                        GamepadAddedEvent t{event.cdevice.which};
                        OnGamepadAdded.Invoke(t);
                    }
                    break;
                case SDL_CONTROLLERDEVICEREMOVED: 
                    {
                        GamepadRemovedEvent t{event.cdevice.which};
                        OnGamepadRemoved.Invoke(t);
                    }
                    break;
            }
        }
    }

    SDL_Window* GetSdlWindow() const {
        return window_;
    }

    HWND GetHandle() const { 
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);

        SDL_GetWindowWMInfo(window_, &wmInfo);
        return wmInfo.info.win.window;
    }

    ~Window() {
        ImGui_ImplSDL2_Shutdown();
        SDL_DestroyWindow(window_);
    }

    EventDispatcher<BeginProcessEvent> OnBeginProcess;
    EventDispatcher<WindowCloseEvent> OnWindowClose;
    EventDispatcher<QuitEvent> OnQuit;
    EventDispatcher<KeyEvent> OnKey;
    EventDispatcher<MouseMotionEvent> OnMouseMotion;
    EventDispatcher<MouseWheelEvent> OnMouseWheel;
    EventDispatcher<MouseButtonEvent> OnMouseButton;
    EventDispatcher<GamepadButtonEvent> OnGamepadButton;
    EventDispatcher<GamepadAxisMotionEvent> OnGamepadAxisMotion;
    EventDispatcher<GamepadAddedEvent> OnGamepadAdded;
    EventDispatcher<GamepadRemovedEvent> OnGamepadRemoved;
    EventDispatcher<WindowResizeEvent> OnWindowResize;

private:
    SDL_Window* window_;
    WindowSettings settings_;

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