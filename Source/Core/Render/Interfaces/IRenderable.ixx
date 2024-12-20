export module IRenderable;

import <vector>;
import <unordered_map>;
import <memory>;

import ObjectShaderResource;
import RenderSystemSettings;
import RenderContext;

namespace GiiGa
{
    export struct SortData
    {
        ObjectMask object_mask;
        std::shared_ptr<ObjectShaderResource> shaderResource;
    };

    export class IRenderable
    {
    public:
        virtual void Draw(RenderContext& context) =0;
        virtual SortData GetSortData() =0;
    };

    export struct CommonResourceGroup
    {
        std::shared_ptr<ObjectShaderResource> shaderResource;
        std::vector<std::weak_ptr<IRenderable>> renderables;

        CommonResourceGroup(std::shared_ptr<ObjectShaderResource> in_shaderResource):
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
