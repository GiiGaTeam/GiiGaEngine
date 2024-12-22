export module CameraComponent;

import <algorithm>;
import <directxtk12/SimpleMath.h>;
import <json/value.h>;
import <memory>;
import <directx/d3dx12_core.h>;

import Component;
import TransformComponent;
import GameObject;
import Misc;
import IObjectShaderResource;

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
    };

    export class CameraComponent : public Component
    {
    public:
        CameraComponent(CameraType type = Perspective, float FOV = 90, float aspect = 16 / 9, float width = 1280, float height = 720, float Near = 0.01, float Far = 100)
        {
            camera_ = Camera{type, FOV, aspect, width, height, Near, Far};
        };

        void SetType(CameraType type) { camera_.type_ = type; }

        void SetFOV(float FOV) { camera_.FOV_ = FOV; }

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

        void Restore(const ::Json::Value&) override
        {
            Todo();
        }

        std::shared_ptr<IComponent> Clone() override
        {
            Todo();
            return nullptr;
        }

        ::Json::Value DerivedToJson() override
        {
            Todo();
            return Json::Value();
        }

        void Tick(float dt) override
        {
            if (ownerGO_.expired() || ownerGO_.lock()->GetTransformComponent().expired()) return;

            const auto transform = ownerGO_.lock()->GetTransformComponent().lock().get();
            if (!transform) return;

            const Vector3 position = transform->GetLocation();
            const Vector3 target = position + transform->GetTransform().GetForward();
            const Vector3 up = transform->GetTransform().GetUp();

            camera_.view_ = Matrix::CreateLookAt(position, target, up);
            //camera_.view_ = transform->GetWorldMatrix().Invert();
        }

    protected:
        Camera camera_;
        std::weak_ptr<GameObject> ownerGO_;
    };
}
