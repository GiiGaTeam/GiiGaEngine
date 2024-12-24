export module EditorEngine;

import <filesystem>;
import <memory>;
import <json/reader.h>;
import <fstream>;

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
                Time::UpdateTime();
                World::Tick(static_cast<float>(Time::GetDeltaTime()));
                render_system_->Tick();
            }

            DeInitialize();
        }

        static EditorEngine& Instance()
        {
            return *dynamic_cast<EditorEngine*>(instance_);
        }

        std::shared_ptr<EditorAssetDatabase> EditorDatabase()
        {
            return std::dynamic_pointer_cast<EditorAssetDatabase>(asset_database_);
        }

    private:
        std::shared_ptr<EditorAssetDatabase> editor_asset_database_;

        void Initialize(std::shared_ptr<Project> proj) override
        {
            Engine::Initialize(proj);
            World::Initialize();
           
            editor_asset_database_ = std::make_shared<EditorAssetDatabase>(proj);
            asset_database_ = editor_asset_database_;

            editor_asset_database_->Initialize();
            SetupDefaultLoader();
            editor_asset_database_->RemoveMissingFilesFromRegistry();
            editor_asset_database_->ScanAssetsFolderForNewFiles();

            editor_asset_database_->StartProjectWatcher();
            resource_manager_->SetDatabase(editor_asset_database_);

            render_system_ = std::make_shared<EditorRenderSystem>(*window_);
            render_system_->Initialize();

            //todo
            auto&& level_path = project_->GetProjectPath() / project_->GetDefaultLevelPath();
            World::AddLevelFromAbsolutePath(level_path);
        }

        void DeInitialize() override
        {

            editor_asset_database_->Shutdown();
            editor_asset_database_->SaveRegistry();
            
            editor_asset_database_.reset();
            World::DeInitialize();
            render_system_.reset();
            Engine::DeInitialize();
        }

        void SetupDefaultLoader()
        {
            editor_asset_database_->RegisterLoader<DDSAssetLoader>();
            editor_asset_database_->RegisterLoader<ImageAssetLoader>();
            editor_asset_database_->RegisterLoader<MeshAssetLoader>();
        }
    };
} // namespace GiiGa
