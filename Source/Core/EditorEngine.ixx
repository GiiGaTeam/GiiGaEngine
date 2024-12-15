module;

#include <memory>

export module EditorEngine;

export import Engine;
import Time;
import Misc;
import EditorRenderSystem;
import EditorAssetDatabase;
import World;

import DDSAssetLoader;
import ImageAssetLoader;

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

            asset_database_->SaveRegistry();
        }

        static EditorEngine& Instance()
        {
            return *dynamic_cast<EditorEngine*>(instance_);
        }

        std::shared_ptr<EditorAssetDatabase> EditorDatabase() {
            return std::dynamic_pointer_cast<EditorAssetDatabase>(asset_database_);
        }
    private:
        void Initialize(std::shared_ptr<Project> proj) override
        {
            Engine::Initialize(proj);
            render_system_ = std::make_shared<EditorRenderSystem>(*window_);
            render_system_->Initialize();

            auto database = std::make_shared<EditorAssetDatabase>(proj);
            asset_database_ = database;

            database->InitializeDatabase();
            DefaultLoaderSetup(database);

            database->StartProjectWatcher();

            //todo
            //World::LoadLevel(proj->GetDefaultLevelPath());
        }

        void DefaultLoaderSetup(std::shared_ptr<EditorAssetDatabase> database) {
            database->RegisterLoader<DDSAssetLoader>();
            database->RegisterLoader<ImageAssetLoader>();
        }
    };
} // namespace GiiGa
