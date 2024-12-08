module;

#include <memory>

export module EditorEngine;

export import Engine;
import Time;
import Misc;
import EditorRenderSystem;
import World;


namespace GiiGa
{
    export class EditorEngine : public Engine
    {
    private:
        virtual void Initialize(std::shared_ptr<Project> proj)
        {
            Engine::Initialize();
            render_system_ = std::make_shared<EditorRenderSystem>(*window_);
            render_system_->Initialize();
            //todo
            //World::LoadLevel(proj->GetDefaultLevelPath());
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
                    if (!level.GetIsActive())
                    {
                        continue;
                    }
                    for (auto&& game_object : level.GetGameObjects())
                    {
                        if (game_object->tick_type == TickType::Default)
                            game_object->Tick(static_cast<float>(Time::GetDeltaTime()));
                    }
                }
                render_system_->Tick();
            }
        }
    };
} // namespace GiiGa
