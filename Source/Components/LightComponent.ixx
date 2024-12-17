module;

#include <memory>
#include <directxtk/SimpleMath.h>

export module LightComponent;

import Component;
import IComponentsInLevel;

namespace GiiGa
{
    class LightComponent : public Component
    {
        enum class LightType
        {
            Directional,
            Spot,
            Point
        };

    public:
        LightComponent(std::shared_ptr<IComponentsInLevel> inLevel):
            Component(inLevel)
        {
            data_.type = LightType::Directional;
            data_.range = 10.0f;
            data_.angle = 45.0f;
            data_.color = DirectX::SimpleMath::Vector3::One;
            data_.intensity = 1.0f;
        }

        void SetType(LightType type)
        {
            data_.type = type;
        }

        void SetRange(float range)
        {
            data_.range = range;
        }

        void SetAngle(float angle)
        {
            data_.angle = angle;
        }

        void SetColor(const DirectX::SimpleMath::Vector3& color)
        {
            data_.color = color;
        }

        void SetIntensity(float intensity)
        {
            data_.intensity = intensity;
        }

    private:
        struct LightData
        {
            LightType type;
            float range;
            float angle;
            DirectX::SimpleMath::Vector3 color;
            float intensity;
            //TODO:
            //LightMode mode; // Baked, Realtime
        } data_;
    };
}
