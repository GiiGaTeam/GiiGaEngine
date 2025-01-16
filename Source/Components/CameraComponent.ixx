export module CameraComponent;

import <algorithm>;
import <directxtk12/SimpleMath.h>;
import <json/value.h>;
import <memory>;
import <directx/d3dx12_core.h>;
import <optional>;

import Component;
import TransformComponent;
import GameObject;
import Misc;
import IObjectShaderResource;
import PrefabInstance;
import MathUtils;

using namespace DirectX::SimpleMath;

namespace GiiGa
{
    export enum CameraType
    {
        Perspective,
        Orthographic
    };

    /*
    export class CameraShaderResource : public IObjectShaderResource
    {
        friend struct Camera;

    public:
        
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            // todo: actually we can cache this allocation by adding setters here
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result; 
            result.reserve(1);
            result.push_back(CameraCBV_->getDescriptor().getGPUHandle());
            return result;
        }

    private:
        std::shared_ptr<BufferView<Constant>> CameraCBV_;
    };*/

    export struct Camera
    {
        CameraType type_;

        float FOV_;
        float aspect_;

        float width_;
        float height_;

        float near_;
        float far_;

        Matrix view_;

        Matrix GetView() const
        {
            return view_;
        };

        Matrix GetProj() const
        {
            switch (type_)
            {
            case Perspective:
                return Matrix::CreatePerspectiveFieldOfView(FOV_, aspect_, near_, far_);
            case Orthographic:
                return Matrix::CreateOrthographic(width_, height_, near_, far_);
            }
            return Matrix::Identity;
        };

        Matrix GetViewProj() const
        {
            return view_ * GetProj();
        };

        Matrix GetProjView() const
        {
            return GetProj() * view_;
        }

        /*
         * nearLayer and farLayer in the range from 0 to 1
         */
        void GetSubProjAndDistanceToFar(float nearLayer, float farLayer, Matrix& retProj, float& retDistanceToFar) const
        {
            nearLayer = nearLayer >= 0.0f && nearLayer <= 1.0f ? near_ + (far_ - near_) * nearLayer : near_;
            farLayer = farLayer >= 0.0f && farLayer <= 1.0f ? near_ + (far_ - near_) * farLayer : far_;
            retDistanceToFar = farLayer;

            if (type_ == Perspective)
                retProj = Matrix::CreatePerspectiveFieldOfView(RadFromDeg(FOV_), aspect_, nearLayer, farLayer);
            else
                retProj = Matrix::CreateOrthographic(width_, height_, nearLayer, farLayer);
        }
    };

    export class CameraComponent : public Component
    {
    public:
        CameraComponent(CameraType type = Perspective, float FOV = DirectX::XMConvertToRadians(90), float aspect = 16 / 9, float width = 1280, float height = 720, float Near = 0.01, float Far = 100)
        {
            camera_ = Camera{type, FOV, aspect, width, height, Near, Far};
        };

        void SetType(CameraType type) { camera_.type_ = type; }

        void SetFOVinDeg(float FOV) { camera_.FOV_ = DirectX::XMConvertToRadians(FOV); }

        void SetAspect(float aspect) { camera_.aspect_ = aspect; }

        void SetWidth(float width) { camera_.width_ = width; }

        void SetHeight(float height) { camera_.height_ = height; }

        void SetNear(float Near) { camera_.near_ = Near; }

        void SetFar(float Far) { camera_.far_ = Far; }

        Camera GetCamera() const { return camera_; }

        void Init() override
        {
            ownerGO_ = std::dynamic_pointer_cast<GameObject>(owner_.lock());
        }

        void BeginPlay() override
        {
            
        }

        void Restore(const ::Json::Value&) override
        {
            Todo();
        }

        std::shared_ptr<IComponent> Clone(std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid, const std::optional<std::unordered_map<Uuid, Uuid>>
                                          & instance_uuid) override
        {
            Todo();
            return {};
        }

        void RestoreFromOriginal(std::shared_ptr<IComponent> original, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
        }

        void RestoreAsPrefab(const Json::Value&, const std::unordered_map<Uuid, Uuid>& prefab_uuid_to_world_uuid) override
        {
        }

        std::vector<std::pair<PropertyModificationKey,PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
            Todo();
        }

        ::Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Todo();
            return Json::Value();
        }

        void Tick(float dt) override
        {
            if (ownerGO_.expired() || ownerGO_.lock()->GetTransformComponent().expired()) return;

            const auto transform = ownerGO_.lock()->GetTransformComponent().lock().get();
            if (!transform) return;

            const Vector3 position = transform->GetWorldLocation();
            const Vector3 target = position + transform->GetWorldTransform().GetForward();
            const Vector3 up = transform->GetWorldTransform().GetUp();

            camera_.view_ = Matrix::CreateLookAt(position, target, up);
            //camera_.view_ = transform->GetInverseWorldMatrix();
        }

    protected:
        Camera camera_;
        std::weak_ptr<GameObject> ownerGO_;
    };
}
