#pragma once


#include <pybind11/conduit/wrap_include_python_h.h>



#include<memory>

#include<Engine.h>
#include<Misc.h>
#include<Timer.h>
#include<RenderSystem.h>
#include<World.h

namespace GiiGa
{
    class GameEngine : public Engine
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

            Timer::Start();

            while (!quit_)
            {
                window_->ProcessEvents();
                if (false)
                {
                    quit_ = true;
                }
                Timer::UpdateTime();
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
                            level_root_gos[i]->Tick(Timer::GetDeltaTime());
                    }
                }
                render_system_->Tick();
            }
        }
    };
}
