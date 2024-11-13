module;

#include "directxtk12/SimpleMath.h"
#include <memory>

export module TransformComponent;
import Component;

namespace GiiGa
{
    using namespace DirectX::SimpleMath;

    struct Transform
    {
        Vector3 location;
        Vector3 rotation;
        Vector3 scale;
    };

    export class TransformComponent : public Component
    {
    public:
        TransformComponent(const Vector3 location = Vector3::Zero
            , const Vector3 rotation = Vector3::Zero
            , const Vector3 scale = Vector3::One)
        {
            transform_ = Transform{location, rotation, scale};
        }

        TransformComponent(Transform&& transform)
        {
            transform_ = transform;
        }

        void Tick(float dt) override;

        void Init() override
        {
            //TODO refactor Object-Component system: cyclical dependence 
            /*if (owner_)
            {
                owner
            }*/
        }

        std::weak_ptr<TransformComponent> GetParent() const { return parent_; }

    private:
        Transform transform_;
        Matrix world_matrix_;
        Matrix local_matrix_;

        std::weak_ptr<TransformComponent> parent_;

    };
}
