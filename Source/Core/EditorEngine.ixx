module;

#include <filesystem>
#include <memory>
#include <json/reader.h>
#include <fstream>

export module EditorEngine;

export import Engine;
import Time;
import Misc;
import EditorRenderSystem;
import World;

import TransformComponent;

namespace GiiGa
{
    export class EditorEngine : public Engine
    {
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

    private:
        void Initialize(std::shared_ptr<Project> proj) override
        {
            Engine::Initialize(proj);
            render_system_ = std::make_shared<EditorRenderSystem>(*window_);
            render_system_->Initialize();
            //todo
            auto&& level_path = project_->GetProjectPath() / project_->GetDefaultLevelPath();
            World::AddLevelFromAbsolutePath(level_path);
        }
    };
} // namespace GiiGa
