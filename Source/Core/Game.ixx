module;
#include <iostream>

export module GameLoop;

import Time;
import World;
import ConsoleComponent;
import Input;

namespace GiiGa
{
export class Game
{
public:
    void Run()
    {
        Time::Start();

        while (!quit_)
        {
            auto event = Input::ProcessEvents();
            if (event.quit)
            {
                quit_ = true;
            }
            Time::UpdateTime();
            for (auto&& game_object : World::GetGameObjects())
            {
                if (game_object->tick_type == TickType::Default)
                    game_object->Tick(static_cast<float>(Time::GetDeltaTime()));
            }
            if (Input::IsKeyDown(Button::KeyD))
            {
                std::cout << "Button Down" << std::endl;
            }
            else if (Input::IsKeyHeld(Button::KeyD))
            {
                std::cout << "Button Held" << std::endl;
            }
            else if (Input::IsKeyUp(Button::KeyD))
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
};
}