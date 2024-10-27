module;

#include <memory>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <tuple>
#include <SDL2/SDL_events.h>

export module RawInputSystem;
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

export class RawInputSystem
{
public:
    RawInputSystem(std::shared_ptr<Window> window) { 
        ImGui_ImplSDL2_InitForD3D(window->GetSdlWindow());
    }

    void ProcessEvents() {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    }

    bool IsKeyHeld(Button button) const { return GetButtonState(button) == ButtonState::Held; }
    bool IsKeyDown(Button button) const { return GetButtonState(button) == ButtonState::Pressed; }
    bool IsKeyUp(Button button) const { return GetButtonState(button) == ButtonState::Released; }

    float Get1DAxis(Button positive, Button negative) const { 
        return (IsKeyHeld(positive) ? 1.0f : 0.0f) - (IsKeyHeld(negative) ? 1.0f : 0.0f);
    }

    std::tuple<float, float> Get2DAxis(Button positiveX, Button negativeX, Button positiveY, Button negativeY) const { 
        float x = Get1DAxis(positiveX, negativeX);
        float y = Get1DAxis(positiveY, negativeY);

        return std::make_tuple(x, y);
    }

private:
    enum class ButtonState
    {
        None,
        Pressed,
        Released,
        Held
    };

    ButtonState GetButtonState(Button button) const
    {
        ImGuiIO& io = ImGui::GetIO();

        if (button == Button::KeyA) return io.KeysDown[ImGuiKey_A] ? ButtonState::Held : ButtonState::Released;
        if (button == Button::KeyW) return io.KeysDown[ImGuiKey_W] ? ButtonState::Held : ButtonState::Released;
        if (button == Button::KeyS) return io.KeysDown[ImGuiKey_S] ? ButtonState::Held : ButtonState::Released;
        if (button == Button::KeyD) return io.KeysDown[ImGuiKey_D] ? ButtonState::Held : ButtonState::Released;
        if (button == Button::KeySpace) return io.KeysDown[ImGuiKey_Space] ? ButtonState::Held : ButtonState::Released;

        if (button == Button::MouseLeft) return io.MouseDown[0] ? ButtonState::Held : ButtonState::Released;
        if (button == Button::MouseRight) return io.MouseDown[1] ? ButtonState::Held : ButtonState::Released;

        if (button == Button::GamepadButtonA)
            return io.NavInputs[ImGuiNavInput_Activate] > 0.0f ? ButtonState::Held : ButtonState::Released;
        if (button == Button::GamepadButtonB) return io.NavInputs[ImGuiNavInput_Cancel] > 0.0f ? ButtonState::Held : ButtonState::Released;
        if (button == Button::GamepadButtonX) return io.NavInputs[ImGuiNavInput_Menu] > 0.0f ? ButtonState::Held : ButtonState::Released;
        if (button == Button::GamepadButtonY) return io.NavInputs[ImGuiNavInput_Input] > 0.0f ? ButtonState::Held : ButtonState::Released;

        return ButtonState::None;
    }
};
}
