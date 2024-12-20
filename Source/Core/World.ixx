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

import TransformComponent;

namespace GiiGa
{
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
            auto p_level = std::make_shared<Level>(level_settings);
            GetInstance().levels_.push_back(p_level);
        }
        
        static void DeInitialize()
        {
            instance_.reset();
        }

        static void Tick(float dt)
        {
            auto instance = GetInstance();
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

        static void AddLevelFromAbsolutePath(const std::filesystem::path& absolutePath, bool setIsActive = true)
        {
            el::Loggers::getLogger(LogWorld)->info("Loading level with path: %v", absolutePath);
            AddLevel(Level::FromAbsolutePath(absolutePath), setIsActive);
        }

        static void AddLevel(std::shared_ptr<Level> level, bool setIsActive = true)
        {
            level->SetIsActive(setIsActive);
            GetInstance().levels_.push_back(level);
        }

    private:
        std::vector<std::shared_ptr<Level>> levels_;

        World() = default;

        std::shared_ptr<ILevelRootGameObjects> GetPersistentLevel_Impl() override
        {
            return levels_[0];
        }
    };
}
