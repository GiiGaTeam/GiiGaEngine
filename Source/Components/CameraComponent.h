#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<json/value.h>
#include<memory>
#include<optional>

#include<Component.h>
#include<TransformComponent.h>
#include<GameObject.h>
#include<Misc.h>
#include<PrefabInstanceModifications.h>
#include<DXMathUtils.h>

using namespace DirectX::SimpleMath;

namespace GiiGa
{
    enum CameraType
    {
        Perspective,
        Orthographic
    };

    /*
    class CameraShaderResource : public IObjectShaderResource
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

    struct Camera
    {
        CameraType type_;

        float FOV_;
        float aspect_;

        float width_;
        float height_;

        float near_;
        float far_;

        Matrix view_;

        Camera() = default;

        Camera(CameraType type, float fov, float aspect, float width, float height, float in_near, float in_far):
            type_(type), FOV_(fov), aspect_(aspect), width_(width), height_(height), near_(in_near), far_(in_far)
        {
        }

        Camera(Json::Value& json)
        {
            if (json["Type"].asString() == "Perspective")
                type_ = Perspective;
            else if (json["Type"].asString() == "Orthographic")
                type_ = Orthographic;

            FOV_ = json["FOV"].asFloat();
            aspect_ = json["aspect"].asFloat();
            width_ = json["width"].asFloat();
            height_ = json["height"].asFloat();
            near_ = json["near"].asFloat();
            far_ = json["far"].asFloat();
        }

        Json::Value ToJson()
        {
            Json::Value json;
            if (type_ == Perspective)
                json["Type"] = "Perspective";
            else if (type_ == Orthographic)
                json["Type"] = "Orthographic";

            json["FOV"] = FOV_;
            json["aspect"] = aspect_;
            json["width"] = width_;
            json["height"] = height_;
            json["near"] = near_;
            json["far"] = far_;

            return json;
        }

        Matrix GetView() const
        {
            return view_;
        };

        Matrix GetProj() const
        {
            switch (type_)
            {
            case Perspective:
                return Matrix::CreatePerspectiveFieldOfView(RadFromDeg(FOV_), aspect_, near_, far_);
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

    class CameraComponent : public Component
    {
    public:
        CameraComponent(CameraType type = Perspective, float FOV = 90, float aspect = 16 / 9, float width = 1280, float height = 720, float Near = 0.01, float Far = 100)
        {
            camera_ = Camera{type, FOV, aspect, width, height, Near, Far};
        };

        CameraComponent(Json::Value json, bool roll_id = false):
            Component(json, roll_id)
        {
            camera_ = Camera(json["Camera"]);
        }

        ::Json::Value DerivedToJson(bool is_prefab_root) override
        {
            Json::Value json;
            json["Type"] = typeid(CameraComponent).name();
            json["Camera"] = camera_.ToJson();
            return json;
        }

        void SetType(CameraType type) { camera_.type_ = type; }

        void SetFOVinDeg(float FOV) { camera_.FOV_ = FOV; }

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

        void RestoreFromLevelJson(const ::Json::Value&) override
        {
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

        std::vector<std::pair<PropertyModificationKey, PrefabPropertyValue>> GetPrefabInstanceModifications(std::shared_ptr<IComponent>) const override
        {
            Todo();
            return {};
        }

        void ApplyModifications(const PrefabPropertyModifications& modifications) override
        {
            Todo();
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
