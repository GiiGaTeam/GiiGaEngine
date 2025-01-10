module;

#include <pybind11/conduit/wrap_include_python_h.h>

export module GameEngine;

import <memory>;

import Engine;
import Misc;
import Time;
import RenderSystem;
import  World;

namespace GiiGa
{
    export class GameEngine : public Engine
    {
    private:
        virtual void Initialize(std::shared_ptr<Project> proj)
        {
            Engine::Initialize(proj);
            //render_system_ = std::make_shared<RenderSystem>(*window_);
        }

    public:
        virtual void Run(std::shared_ptr<Project> proj)
        {
            Initialize(proj);

            Time::Start();

            while (!quit_)
            {
                window_->ProcessEvents();
                if (false)
                {
                    quit_ = true;
                }
                Time::UpdateTime();
                for (auto& level : World::GetLevels())
                {
                    if (!level->GetIsActive())
                    {
                        continue;
                    }
                    for (auto&& [_,game_object] : level->GetRootGameObjects())
                    {
                        if (game_object->tick_type == TickType::Default)
                            game_object->Tick(static_cast<float>(Time::GetDeltaTime()));
                    }
                }
                render_system_->Tick();
            }
        }
    };
}
