module;

#include <vector>
#include <memory>
#include <vector>

export module World;

import Level;
export import GameObject;

namespace GiiGa
{
    export class World
    {
    public:
        static std::unique_ptr<World>& GetInstance()
        {
            if (instance_) return instance_;
            else return instance_ = std::make_unique<World>();
        }

        const static std::vector<std::shared_ptr<GameObject>>& GetGameObjects()
        {
            return GetInstance()->game_objects_;
        }

        static void AddLevel(Level&& level)
        {
            GetInstance()->levels_.push_back(level);
        }

        static std::shared_ptr<GameObject> CreateObject(std::shared_ptr<Level> level = nullptr)
        {
            auto obj = std::make_shared<GameObject>();

            GetInstance()->game_objects_.push_back(obj);

            if (level)
            {
                level->AddGameObject(obj);
            }

            return obj;
        }

    private:
        static inline std::unique_ptr<World> instance_ = nullptr;
        std::vector<Level> levels_;
        std::vector<std::shared_ptr<GameObject>> game_objects_;
    };
}