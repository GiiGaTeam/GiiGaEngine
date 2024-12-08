module;

export module StaticMeshComponent;

import Engine;
import Component;
import Mesh;
import Material;
#include <memory>
#include <json/value.h>

namespace GiiGa
{
    export class StaticMeshComponent : public Component
    {
    public:
        void Tick(float dt) override;
        void Init() override;
        ::std::shared_ptr<Component> Clone() override;

        // todo
        Json::Value ToJSon() const override
        {
            //mesh_->GetId();
            //material_->GetId();
        }

        // todo
        Json::Value FromJson() const
        {
            //mesh_ = Engine::Instance().ResourceManager().GetAsset<Mesh>(AssetHandle{jsonid, AssetType::Mesh});
            //material_ = Engine::Instance().ResourceManager().GetAsset<Material>(AssetHandle{jsonid, AssetType::Material});
        }

    private:
        std::shared_ptr<Mesh> mesh_;
        std::shared_ptr<Material> material_;
    };
}
