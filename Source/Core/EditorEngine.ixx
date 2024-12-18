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
import EditorAssetDatabase;
import World;
import TransformComponent;
import DDSAssetLoader;
import ImageAssetLoader;
import MeshAssetLoader;

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

            editor_asset_database_->Shutdown();
            editor_asset_database_->SaveRegistry();
        }

        static EditorEngine& Instance()
        {
            return *dynamic_cast<EditorEngine*>(instance_);
        }

        std::shared_ptr<EditorAssetDatabase> EditorDatabase() {
            return std::dynamic_pointer_cast<EditorAssetDatabase>(asset_database_);
        }

    private:
        std::shared_ptr<EditorAssetDatabase> editor_asset_database_;

        void Initialize(std::shared_ptr<Project> proj) override
        {
            Engine::Initialize(proj);
            World::CreateLevelHAHAHA();
            render_system_ = std::make_shared<EditorRenderSystem>(*window_);
            render_system_->Initialize();

            editor_asset_database_ = std::make_shared<EditorAssetDatabase>(proj);
            asset_database_ = editor_asset_database_;

            editor_asset_database_->InitializeDatabase();
            DefaultLoaderSetup();

            editor_asset_database_->StartProjectWatcher();
            resource_manager_->SetDatabase(editor_asset_database_);

            //todo
            auto&& level_path = project_->GetProjectPath() / project_->GetDefaultLevelPath();
            World::AddLevelFromAbsolutePath(level_path);
        }

        void DefaultLoaderSetup() {
            editor_asset_database_->RegisterLoader<DDSAssetLoader>();
            editor_asset_database_->RegisterLoader<ImageAssetLoader>();
            editor_asset_database_->RegisterLoader<MeshAssetLoader>();
        }
    };
} // namespace GiiGa
