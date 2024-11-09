module;
#include <iostream>

export module GameLoop;

import Time;
import World;
import ConsoleComponent;
import Input;
import Window;

namespace GiiGa
{
export class Game
{
public:
    Game(std::shared_ptr<GiiGa::Window> window) : window_(window) { 
        window->OnWindowClose.Register([this](const WindowCloseEvent& arg) { quit_ = true; });
        window->OnQuit.Register([this](const QuitEvent& arg) { quit_ = true; });
    }

    void Run()
    {
        Time::Start();

        while (!quit_)
        {
            window_->ProcessEvents();
            if (false)
            {
                quit_ = true;
            }
            Time::UpdateTime();
            for (auto&& game_object : World::GetGameObjects())
            {
                if (game_object->tick_type == TickType::Default)
                    game_object->Tick(static_cast<float>(Time::GetDeltaTime()));
            }
            if (Input::IsKeyDown(MouseButton::MouseLeft))
            {
                std::cout << "Button Down" << std::endl;
            }
            else if (Input::IsKeyHeld(MouseButton::MouseLeft))
            {
                std::cout << "Button Held" << std::endl;
            }
            else if (Input::IsKeyUp(MouseButton::MouseLeft))
            {
                std::cout << "Button Released" << std::endl;
            }
            else
            {
                std::cout << Input::GetMousePosition().x << " " << Input::GetMousePosition().y << std::endl;
            }
        }
    }

private:
    bool quit_ = false;
    std::shared_ptr<GiiGa::Window> window_;
};
}