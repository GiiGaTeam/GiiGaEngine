#pragma once
#include<vector>
#include<unordered_map>
#include<memory>

#include<nvrhi/nvrhi.h>

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
        // The pass assembles `state` (pipeline, framebuffer, viewport, binding set with view/
        // object/material bindings); the renderable contributes its mesh vertex/index buffers
        // and issues the draw.
        virtual void Draw(RenderContext& context, nvrhi::GraphicsState& state) = 0;
        virtual SortData GetSortData() = 0;
        virtual PerObjectData& GetPerObjectData() = 0;
    };

    inline bool operator==(const std::weak_ptr<IRenderable>& lhs, const std::weak_ptr<IRenderable>& rhs)
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
