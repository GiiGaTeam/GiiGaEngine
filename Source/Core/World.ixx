module;

#include <pybind11/conduit/wrap_include_python_h.h>

export module World;

import <filesystem>;
import <string>;
import <vector>;
import <memory>;
import <vector>;
import <json/json.h>;

export import Level;
export import GameObject;
export import IWorldQuery;
import Logger;
import Engine;

import TransformComponent;

namespace GiiGa
{
    export enum class WorldState
    {
        Edit,
        Play
    };

    export class World : public WorldQuery
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
            auto p_level = std::make_shared<Level>(AssetHandle{Uuid::New(), 0}, level_settings);
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
                comp_to_init->Init();
            }
            for (auto& level : GetLevels())
            {
                if (!level->GetIsActive())
                {
                    continue;
                }
                for (auto&& [_,game_object] : level->GetRootGameObjects())
                {
                    if (game_object->tick_type == TickType::Default)
                        game_object->Tick(dt);
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
            AddLevel(Engine::Instance().ResourceManager()->GetAsset<Level>({uuid, 0}), setIsActive);
        }

        static void AddLevel(std::shared_ptr<Level> level, bool setIsActive = true)
        {
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
            for (auto&& level : levels_)
            {
                level->BeginPlay();
            }
        }

        void OnPlayToEditStateChange_()
        {
            if (levels_.size() >= 2)
            {
                AssetHandle loaded_levelid = levels_[1]->GetId();

                levels_.erase(levels_.begin() + 1);
                
                AddLevel(Engine::Instance().ResourceManager()->GetAsset<Level>(loaded_levelid));
            }
        }
    };
}
