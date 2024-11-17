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

        static std::shared_ptr<GameObject> CreateGameObject(std::shared_ptr<Level> Level = nullptr)
        {
            auto obj = std::make_shared<GameObject>();

            GetInstance()->game_objects_.push_back(obj);

            if (Level)
            {
                Level->AddGameObject(obj);
            }

            return obj;
        }

        static bool DestroyGameObject(std::shared_ptr<GameObject> GameObject)
        {
            auto Iterator = GetInstance()->game_objects_.erase(
                std::remove(GetInstance()->game_objects_.begin(), GetInstance()->game_objects_.end(), GameObject),
                GetInstance()->game_objects_.end());

            if (Iterator->get())
            {
                return true;
            }
            return false;
        }

        static std::shared_ptr<GameObject> Instantiate(GameObjectAsset Asset,
            std::shared_ptr<GameObject> Parent,
            std::shared_ptr<Level> Level = nullptr)
        {
            auto GO = CreateGameObject(Level);

            GO->SetParent(Parent, false);
            /// Setup GO
            /// ...

            return GO;
        }

    private:
        static inline std::unique_ptr<World> instance_ = nullptr;
        std::vector<Level> levels_;
        std::vector<std::shared_ptr<GameObject>> game_objects_;
    };
}