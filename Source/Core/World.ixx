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

        static std::shared_ptr<GameObject> CreateGameObject(std::shared_ptr<Level> level = nullptr)
        {
            auto obj = std::make_shared<GameObject>();

            if (level)
            {
                level->AddRootGameObject(obj);
            }
            else
            {
                GetInstance()->levels_[0].AddRootGameObject(obj);
            }

            return obj;
        }

        static bool DestroyGameObject(std::shared_ptr<GameObject> gameObject)
        {
            for (auto level : GetInstance()->levels_)
            {
                if (level.DestroyGameObject(gameObject))
                {
                    return true;
                }
            }

            return false;
        }

        static std::shared_ptr<GameObject> Instantiate(GameObject* prefab,
                                                       std::shared_ptr<GameObject> parent,
                                                       Level* level)
        {
            auto newGameObject = prefab->Clone();

            if (level)
            {
                level->AddRootGameObject(newGameObject);
            }
            else
            {
                GetInstance()->levels_[0].AddRootGameObject(newGameObject);
            }

            if (parent)
            {
                newGameObject->SetParent(parent, false);
            }

            return newGameObject;
        }

        static std::shared_ptr<GameObject> Instantiate(GameObject* prefab,
                                                       std::shared_ptr<GameObject> parent,
                                                       std::string levelName)
        {
            if (levelName == "")
            {
                return Instantiate(prefab, nullptr, nullptr);
            }
            for (auto level : GetInstance()->levels_)
            {
                if (level.name == levelName)
                {
                    return Instantiate(prefab, parent, &level);
                }
            }

            return nullptr;
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
