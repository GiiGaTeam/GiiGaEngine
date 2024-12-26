module;

#define NOMINMAX
#include <directx/d3dx12.h>
#include <DirectXCollision.h>
#include <directxtk12/SimpleMath.h>

export module LightComponent;

import <memory>;
import <json/value.h>;
import <bitset>;
import <filesystem>;
import <algorithm>;

import Engine;
import Component;
import MeshAsset;
import Material;
import IRenderable;
import SceneVisibility;
import TransformComponent;
import IUpdateGPUData;

namespace GiiGa
{
    export class LightComponent : public Component, public IRenderable, public IUpdateGPUData
    {
    public:
        virtual void SetIntensity(float intensity) { maxIntensity_ = intensity; }
        virtual void SetColor(const DirectX::SimpleMath::Vector3& color) { color_ = color; }

        float GetIntensity() const { return maxIntensity_; }
        DirectX::SimpleMath::Vector3 GetColor() const { return color_; }

    protected:
        float maxIntensity_ = 1;
        DirectX::SimpleMath::Vector3 color_ = {1, 0, 0};
        std::weak_ptr<TransformComponent> transform_;
    };
}
