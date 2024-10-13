module;

export module GameLoop;

import Time;
import World;
import ConsoleComponent;

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
                Time::UpdateTime();
                for (auto&& game_object : World::GetGameObjects())
                {
                    if (game_object->tick_type == TickType::Default)
                        game_object->Tick(static_cast<float>(Time::GetDeltaTime()));
                }
            }
        }

    private:
        bool quit_ = false;
    };
}