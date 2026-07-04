#pragma once

// IRenderable.h — abstraction for anything the renderer draws.
// NVRHI cutover: a renderable no longer draws itself (it used to call IASetVertexBuffers /
// DrawIndexedInstanced on a D3D12 command list). With NVRHI the *pass* assembles the full
// GraphicsState, so the renderable is a data provider: its vertex/index buffers, index
// count/format, per-object data source, material, object mask, and AABB.
//
// SortData / CommonResourceGroup / DrawPacket are retained (lightly adapted) so the
// SceneVisibility bucketing still groups renderables by material for binding-set reuse.

#include <vector>
#include <unordered_map>
#include <memory>

#include <nvrhi/nvrhi.h>
#include <DirectXCollision.h>

#include <ObjectMask.h>
#include <IObjectShaderResource.h>
#include <PerObjectData.h>

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
        virtual ~IRenderable() = default;

        // --- geometry (NVRHI buffers) ---
        virtual nvrhi::IBuffer* GetVertexBuffer() = 0;
        virtual nvrhi::IBuffer* GetIndexBuffer() = 0;
        virtual uint32_t GetIndexCount() = 0;
        virtual nvrhi::Format GetIndexFormat() = 0;

        // --- per-object / material data ---
        virtual PerObjectData& GetPerObjectData() = 0;
        virtual std::shared_ptr<IObjectShaderResource> GetShaderResource() = 0;

        // --- bucketing / culling ---
        virtual const ObjectMask& GetObjectMask() = 0;
        virtual const DirectX::BoundingBox& GetAABB() = 0;

        SortData GetSortData()
        {
            return SortData{GetObjectMask(), GetShaderResource()};
        }
    };

    inline bool operator==(const std::weak_ptr<IRenderable>& lhs, const std::weak_ptr<IRenderable>& rhs)
    {
        return lhs.lock() == rhs.lock();
    }

    struct CommonResourceGroup
    {
        std::shared_ptr<IObjectShaderResource> shaderResource;
        std::vector<std::weak_ptr<IRenderable>> renderables;

        explicit CommonResourceGroup(std::shared_ptr<IObjectShaderResource> in_shaderResource)
            : shaderResource(std::move(in_shaderResource))
        {
        }
    };

    struct DrawPacket
    {
        ObjectMask objects_mask;
        std::unordered_map<void*, CommonResourceGroup> common_resource_renderables;

        explicit DrawPacket(ObjectMask in_objects_mask)
            : objects_mask(in_objects_mask)
        {
        }
    };
}
