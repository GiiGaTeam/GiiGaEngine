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

namespace GiiGa
{
    export class World final
    {
    public:
        World()
        {
        }

    public:
        static std::unique_ptr<World>& GetInstance()
        {
            if (instance_) return instance_;
            else return instance_ = std::make_unique<World>();
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
            const auto gameObject = std::make_shared<GameObject>();
            gameObject->AttachToLevel(GetInstance()->levels_[0]);
            gameObject->RegisterInWorld();
            return gameObject;
        }

        static void CreateLevelHAHAHA()
        {
            GetInstance()->levels_.push_back(std::make_shared<Level>(Json::Value()));
        }

    private:
        static inline std::unique_ptr<World> instance_ = nullptr;
        std::vector<std::shared_ptr<Level>> levels_;
    };
}
