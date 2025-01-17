#pragma once
#include<vector>
#include<unordered_map>
#include<memory>

#include<ObjectMask.h>
#include<IObjectShaderResource.h>
#include<PerObjectData.h>

namespace GiiGa
{
    struct SortData
    {
        ObjectMask object_mask;
        std::shared_ptr<IObjectShaderResource> shaderResource;
    };

    class IRenderable
    {
    public:
        virtual void Draw(RenderContext& context) =0;
        virtual SortData GetSortData() =0;
        virtual PerObjectData& GetPerObjectData() =0;
    };

    bool operator==(const std::weak_ptr<IRenderable>& lhs, const std::weak_ptr<IRenderable>& rhs)
    {
        return lhs.lock() == rhs.lock();
    }

    struct CommonResourceGroup
    {
        std::shared_ptr<IObjectShaderResource> shaderResource;
        std::vector<std::weak_ptr<IRenderable>> renderables;


        CommonResourceGroup(std::shared_ptr<IObjectShaderResource> in_shaderResource):
            shaderResource(in_shaderResource)
        {
        }
    };

    struct DrawPacket
    {
        ObjectMask objects_mask;
        std::unordered_map<void*, CommonResourceGroup> common_resource_renderables;

        DrawPacket(ObjectMask in_objects_mask):
            objects_mask(in_objects_mask)
        {
        }
    };
}
