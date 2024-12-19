module;

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <vector>
#include <json/json.h>

export module World;

export import Level;
export import GameObject;
export import IWorldQuery;
import Logger;

import TransformComponent;

namespace GiiGa
{
    export class World final
    {
    public:
        static std::unique_ptr<World>& GetInstance()
        {
            if (instance_) return instance_;
            else return instance_ = std::unique_ptr<World>(new World());
        }

        static void Initialize()
        {
            Json::Value level_settings;
            level_settings["Name"] = "PersistentLevel";
            auto p_level = std::make_shared<Level>(level_settings);
            GetInstance()->levels_.push_back(p_level);
        }

        static void BeginPlay()
        {
        }

        static void Tick(float dt)
        {
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
            return GetInstance()->levels_;
        }

        static void AddLevelFromAbsolutePath(const std::filesystem::path& absolutePath)
        {
            el::Loggers::getLogger(LogWorld)->info("Loading level with path: %v", absolutePath);
            AddLevel(Level::FromAbsolutePath(absolutePath), true);
        }

        static void AddLevel(std::shared_ptr<Level> level, bool setIsActive = true)
        {
            level->SetIsActive(setIsActive);
            GetInstance()->levels_.push_back(level);
        }

        static std::shared_ptr<GameObject> CreateGameObject()
        {
            if (GetInstance()->levels_.empty()) return nullptr;
            return GameObject::CreateEmptyGameObjectInLevelRoot(GetInstance()->levels_[0]);
        }

    private:
        static inline std::unique_ptr<World> instance_ = nullptr;
        std::vector<std::shared_ptr<Level>> levels_;

        World() = default;
    };
}
