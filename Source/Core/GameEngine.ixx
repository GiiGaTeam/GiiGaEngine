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
                    const auto& level_root_gos = level->GetRootGameObjects();
                    for (int i = 0; i < level_root_gos.size(); ++i)
                    {
                        if (level_root_gos[i]->tick_type == TickType::Default)
                            level_root_gos[i]->Tick(Time::GetDeltaTime());
                    }
                }
                render_system_->Tick();
            }
        }
    };
}
