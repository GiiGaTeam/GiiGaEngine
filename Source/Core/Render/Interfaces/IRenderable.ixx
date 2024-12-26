export module IRenderable;

import <vector>;
import <unordered_map>;
import <memory>;
import <directx/d3dx12.h>;

import ObjectMask;
import RenderSystemSettings;
import RenderContext;
import IObjectShaderResource;
import PerObjectData;

namespace GiiGa
{
    export struct SortData
    {
        ObjectMask object_mask;
        std::shared_ptr<IObjectShaderResource> shaderResource;
    };

    export class IRenderable
    {
    public:
        virtual void Draw(RenderContext& context) =0;
        virtual SortData GetSortData() =0;
        virtual PerObjectData& GetPerObjectData() =0;
    };

    export struct CommonResourceGroup
    {
        std::shared_ptr<IObjectShaderResource> shaderResource;
        std::vector<std::weak_ptr<IRenderable>> renderables;

        CommonResourceGroup(std::shared_ptr<IObjectShaderResource> in_shaderResource):
            shaderResource(in_shaderResource)
        {
        }
    };

    export struct DrawPacket
    {
        ObjectMask objects_mask;
        std::unordered_map<void*, CommonResourceGroup> common_resource_renderables;

        DrawPacket(ObjectMask in_objects_mask):
            objects_mask(in_objects_mask)
        {
        }
    };
}
