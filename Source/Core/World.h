#pragma once


#include <pybind11/conduit/wrap_include_python_h.h>


#include<vector>
#include<memory>
#include<json/json.h>

#include<Level.h>
#include<GameObject.h>
#include<IWorldQuery.h>
#include<Logger.h>
#include<Engine.h>
#include<EditorAssetDatabase.h>

namespace GiiGa
{
    enum class WorldState
    {
        Edit,
        Play
    };

    class World : public WorldQuery
    {
    public:
        static World& GetInstance()
        {
            if (instance_) return *std::static_pointer_cast<World>(instance_);
            else return *std::static_pointer_cast<World>(instance_ = std::shared_ptr<World>(new World()));
        }

        static void Initialize()
        {
            Json::Value level_settings;
            level_settings["Name"] = "PersistentLevel";
            auto p_level = std::make_shared<Level>(level_settings);
            GetInstance().AddLevel(p_level);
        }

        static void DeInitialize()
        {
            GetInstance().levels_.clear();
            instance_.reset();
        }

        static void Tick(float dt)
        {
            auto& instance = GetInstance();
            while (!instance.comp_init_queue_.empty())
            {
                auto comp_to_init = instance.comp_init_queue_.front();
                instance.comp_init_queue_.pop();
                if (!comp_to_init.expired())
                {
                    comp_to_init.lock()->Init();
                    if (instance.state_ == WorldState::Play)
                        AddComponentToBeginPlayQueue(comp_to_init.lock());
                }
            }

            while (!instance.comp_begin_play_queue_.empty())
            {
                auto comp_to_bp = instance.comp_begin_play_queue_.front();
                instance.comp_begin_play_queue_.pop();
                if (!comp_to_bp.expired())
                {
                    comp_to_bp.lock()->BeginPlay();
                }
            }

            for (auto& level : GetLevels())
            {
                if (!level->GetIsActive())
                {
                    continue;
                }
                const auto& level_root_gos = level->GetRootGameObjects();
                for (int i = 0; i < level_root_gos.size(); ++i)
                {
                    if (level_root_gos[i]->tick_type == TickType::Default)
                        level_root_gos[i]->Tick(dt);
                }
            }
        }

        static const std::vector<std::shared_ptr<Level>>& GetLevels()
        {
            return GetInstance().levels_;
        }

        static void AddLevelFromUuid(const Uuid& uuid, bool setIsActive = true)
        {
            el::Loggers::getLogger(LogWorld)->info("Loading level with uuid: %v", uuid.ToString());
            AddLevel(Engine::Instance().ResourceManager()->GetAsset<LevelAsset>({uuid, 0}), setIsActive);
        }

        static void AddLevel(std::shared_ptr<Level> level, bool setIsActive = true)
        {
            level->SetIsActive(setIsActive);
            GetInstance().levels_.push_back(level);
        }

        static void AddLevel(std::shared_ptr<LevelAsset> asset, bool setIsActive = true)
        {
            auto level = Level::LevelFromLevelAsset(asset);
            level->SetIsActive(setIsActive);
            GetInstance().levels_.push_back(level);
        }

        WorldState GetState()
        {
            return state_;
        }

        void SetState(WorldState state)
        {
            if (state_ == state)
                return;

            if (state_ == WorldState::Edit && state == WorldState::Play)
            {
                OnEditToPlayStateChange_();
            }
            else if (state_ == WorldState::Play && state == WorldState::Edit)
            {
                OnPlayToEditStateChange_();
            }

            state_ = state;
        }

    private:
        std::vector<std::shared_ptr<Level>> levels_;

        WorldState state_;

        World() = default;

        std::shared_ptr<ILevelRootGameObjects> GetPersistentLevel_Impl() override
        {
            return levels_[0];
        }

        void OnEditToPlayStateChange_()
        {
            if (levels_.size() >= 2)
            {
                auto database = Engine::Instance().ResourceManager()->Database();
                auto level_asset = levels_[1]->CreateAndReplaceLevelAsset();
                //std::dynamic_pointer_cast<EditorAssetDatabase>(database)->CreateAsset(level_asset, "~" + levels_[1]->GetLevelName() + ".level");
                for (auto&& level : levels_)
                {
                    level->BeginPlay();
                }
            }
        }

        void OnPlayToEditStateChange_()
        {
            if (levels_.size() >= 2)
            {
                std::shared_ptr<LevelAsset> asset = levels_[1]->GetLevelAsset();

                //auto database = Engine::Instance().ResourceManager()->Database();
                //std::dynamic_pointer_cast<EditorAssetDatabase>(database)->RemoveAssetFile(asset);
                
                levels_.erase(levels_.begin() + 1);

                AddLevel(asset);
            }
        }
    };
}
