#pragma once

#include<memory>
#include<Engine.h>
#include<Timer.h>
#include<EditorRenderSystem.h>
#include<EditorAssetDatabase.h>
#include"World.h"
#include<DDSAssetLoader.h>
#include<ImageAssetLoader.h>
#include<MeshAssetLoader.h>
#include<VertexTypes.h>
#include<MaterialLoader.h>
#include<LevelAssetLoader.h>
#include<PrefabAssetLoader.h>
#include<ScriptAssetLoader.h>

namespace GiiGa
{
    class EditorEngine : public Engine
    {
    public:
        virtual void Run(std::shared_ptr<Project> proj)
        {
            Initialize(proj);

            Timer::Start();

            while (!quit_)
            {
                window_->ProcessEvents();
                CheckAssetUpdateQueue();
                Timer::UpdateTime();
                World::Tick(static_cast<float>(Timer::GetDeltaTime()));
                if (World::GetInstance().GetState() == WorldState::Play) PhysicsSystem::Simulate(static_cast<float>(Timer::GetDeltaTime()));
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
            PhysicsSystem::GetInstance().Initialize();

            editor_asset_database_ = std::make_shared<EditorAssetDatabase>(proj);
            resource_manager_->SetDatabase(editor_asset_database_);
            asset_database_ = editor_asset_database_;

            editor_asset_database_->Initialize();
            SetupDefaultLoader();

            editor_asset_database_->StartProjectWatcher();

            render_system_ = std::make_shared<EditorRenderSystem>(*window_);
            render_system_->Initialize();

            editor_asset_database_->RemoveMissingFilesFromRegistry();
            editor_asset_database_->ScanAssetsFolderForNewFiles();

            World::AddLevelFromUuid(project_->GetDefaultLevelUuid());
        }

        void DeInitialize() override
        {
            editor_asset_database_->Shutdown();
            editor_asset_database_->SaveRegistry();

            editor_asset_database_.reset();
            World::DeInitialize();
            render_system_.reset();
            Engine::DeInitialize();
            PhysicsSystem::GetInstance().Destroy();
        }

        void SetupDefaultLoader()
        {
            editor_asset_database_->RegisterLoader<DDSAssetLoader>();
            editor_asset_database_->RegisterLoader<ImageAssetLoader>();
            editor_asset_database_->RegisterLoader<MeshAssetLoader<VertexPNTBT>>();
            editor_asset_database_->RegisterLoader<MeshAssetLoader<VertexPosition>>();
            editor_asset_database_->RegisterLoader<MaterialLoader>();
            editor_asset_database_->RegisterLoader<LevelAssetLoader>();
            editor_asset_database_->RegisterLoader<PrefabAssetLoader>();
            editor_asset_database_->RegisterLoader<ScriptAssetLoader>();

            editor_asset_database_->RegisterSaver<LevelAssetLoader>();
            editor_asset_database_->RegisterSaver<PrefabAssetLoader>();
            editor_asset_database_->RegisterSaver<MaterialLoader>();
            editor_asset_database_->RegisterSaver<ImageAssetLoader>();
        }

        void CheckAssetUpdateQueue()
        {
            auto update_pair = EditorDatabase()->GetUpdateQueue();
            std::lock_guard lock{*update_pair.first};
            while (!update_pair.second->empty())
            {
                AssetHandle handle = update_pair.second->back();
                update_pair.second->pop_back();
                resource_manager_->UpdateAsset(handle);
            }
        }
    };
} // namespace GiiGa
