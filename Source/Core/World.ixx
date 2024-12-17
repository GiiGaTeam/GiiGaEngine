module;

#include <string>
#include <vector>
#include <memory>
#include <vector>
#include <json/json.h>

export module World;

export import Level;
export import GameObject;

namespace GiiGa
{
    export class World
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

        static const std::vector<Level>& GetLevels()
        {
            return GetInstance()->levels_;
        }

        static void AddLevel(Level&& level, bool setIsActive = true)
        {
            level.SetIsActive(setIsActive);
            GetInstance()->levels_.push_back(level);
        }

        Json::Value ToJson()
        {
            Json::Value result;
            Json::Value levelsJson;
            for (auto& level : levels_)
            {
                levelsJson.append(level.ToJson());
            }
            result["Levels"] = levelsJson.toStyledString();
            return result;
        }

    private:
        static inline std::unique_ptr<World> instance_ = nullptr;
        std::vector<Level> levels_;
    };
}
