#pragma once


#include<memory>
#include<tuple>
#include<unordered_map>
#include<SDL2/SDL_events.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<Window.h>

namespace GiiGa
{
    struct MousePosition
    {
        int x;
        int y;
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

    class Input
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

        static inline std::unordered_map<KeyCode, ButtonState> keyboard_states_;
        static inline std::unordered_map<MouseButton, ButtonState> mouse_buttons_states_;

        static inline MouseWheel mouse_wheel_;
        static inline MouseDelta mouse_delta_;
        static inline MousePosition mouse_position_;

        static inline float dead_zone_ = 8000.0;

        static inline std::unordered_map<int, GamepadInfo> gamepads_;

    public:
        void Init(std::shared_ptr<Window> window)
        {
            for (int i = 0; i < SDL_NumJoysticks(); ++i)
            {
                if (SDL_IsGameController(i))
                {
                    AddController(i);
                }
            }

            window->OnBeginProcess.Register([this](const BeginProcessEvent& e)
            {
                ResetState();
            });

            window->OnKey.Register([this](const KeyEvent& e)
            {
                ChangeButtonState(e.key, e.is_down);
            });

            window->OnMouseMotion.Register([this](const MouseMotionEvent& e)
            {
                float dx = e.x - mouse_position_.x;
                float dy = e.y - mouse_position_.y;

                float len = sqrtf(dx * dx + dy * dy);

                if (len) dx /= len;
                if (len) dy /= len;

                mouse_position_ = MousePosition(e.x, e.y);
                mouse_delta_ = MouseDelta(dx, dy);
            });

            window->OnMouseWheel.Register([this](const MouseWheelEvent& e)
            {
                mouse_wheel_ = MouseWheel(e.x, e.y);
            });

            window->OnMouseButton.Register([this](const MouseButtonEvent& e)
            {
                ChangeButtonState(e.button, e.is_down);
            });

            window->OnGamepadButton.Register([this](const GamepadButtonEvent& e)
            {
                ChangeButtonState(e.device_id, e.button, e.is_down);
            });

            window->OnGamepadAxisMotion.Register([this](const GamepadAxisMotionEvent& e)
            {
                auto& gamepad = gamepads_[e.device_id];
                auto stick = &gamepad.left_trigger_axis;

                switch (e.stick)
                {
                case (GamepadStick::Left):
                    stick = &gamepad.left_trigger_axis;
                    break;
                case (GamepadStick::Right):
                    stick = &gamepad.right_trigger_axis;
                    break;
                default:
                    break;
                }

                if (abs(e.value) < dead_zone_)
                {
                    *stick = std::make_tuple(0.0f, 0.0f);
                    return;
                }

                auto normal_value = e.value / 32767.0f;

                if (e.axis == GamepadAxis::X)
                {
                    std::get<0>(*stick) = normal_value;
                }
                else
                {
                    std::get<1>(*stick) = normal_value;
                }
            });

            window->OnGamepadAdded.Register([this](const GamepadAddedEvent& e)
            {
                AddController(e.device_id);
            });

            window->OnGamepadRemoved.Register([this](const GamepadRemovedEvent& e)
            {
                RemoveController(e.device_id);
            });
        }

        static bool IsKeyHeld(KeyCode button)
        {
            auto it = keyboard_states_.find(button);
            return it != keyboard_states_.end() && it->second.is_held;
        }

        static bool IsKeyDown(KeyCode button)
        {
            auto it = keyboard_states_.find(button);
            return it != keyboard_states_.end() && it->second.pressed;
        }

        static bool IsKeyUp(KeyCode button)
        {
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

        static bool IsKeyPressed(MouseButton button)
        {
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

        static bool IsKeyPressed(int32_t index, GamepadCode button)
        {
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
                for (auto& [button, state] : gamepad.button_states)
                {
                    state.pressed = false;
                    state.released = false;
                }
            }

            mouse_delta_ = MouseDelta(0.0f, 0.0f);
            mouse_wheel_ = MouseWheel(0.0f, 0.0f);
        }
    };
}
