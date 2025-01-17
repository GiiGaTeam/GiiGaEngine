#pragma once


#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directxtk12/SimpleMath.h>
#include<Octree/octree.h>
#include<DirectXCollision.h>



#include<memory>
#include<vector>
#include<limits>
#include<unordered_map>

#include<IRenderable.h>
#include<ObjectMask.h>
#include<DXMathUtils.h>
#include<Logger.h>

namespace GiiGa
{
    //todo: add sorted Extract
    class SceneVisibility
    {
    public:
        SceneVisibility()
        {
        }

        /*
        static std::vector<OrthoTree::index_t> CheckInsidePlanesOrth(std::vector<OrthoTree::Plane3D> planes)
        {
            auto result = std::vector<OrthoTree::index_t>();
            for (auto& element : GetInstance()->visibility_to_renderable_)
            {
                bool is_inside = true;
                OrthoTree::BoundingBox3D ortho_box = GetInstance()->quadtree.Get(element.first);
                for (const auto& plane : planes)
                {
                    auto rel = OrthoTree::AdaptorGeneralBase<3, OrthoTree::Vector3D, OrthoTree::BoundingBox3D,
                                                             OrthoTree::Ray3D, OrthoTree::Plane3D, OrthoTree::BaseGeometryType,
                                                             OrthoTree::AdaptorGeneralBasics<3, OrthoTree::Vector3D, OrthoTree::BoundingBox3D,
                                                                                             OrthoTree::Ray3D, OrthoTree::Plane3D, OrthoTree::BaseGeometryType>>::
                        GetBoxPlaneRelation(ortho_box, plane.OrigoDistance, plane.Normal, 0.1);
                    if (rel == OrthoTree::PlaneRelation::Negative)
                    {
                        is_inside = false;
                    }
                }
                if (is_inside)
                {
                    result.push_back(element.first);
                }
            }
            return result;
        }

        static std::vector<OrthoTree::index_t> CheckInsidePlanes(std::vector<DirectX::SimpleMath::Plane> planes)
        {
            auto result = std::vector<OrthoTree::index_t>();
            for (auto& element : GetInstance()->visibility_to_renderable_)
            {
                bool is_inside = true;
                OrthoTree::BoundingBox3D ortho_box = GetInstance()->quadtree.Get(element.first);
                auto max = DirectX::SimpleMath::Vector3{static_cast<float>(ortho_box.Max[0]), static_cast<float>(ortho_box.Max[1]), static_cast<float>(ortho_box.Max[2])};
                auto min = DirectX::SimpleMath::Vector3{static_cast<float>(ortho_box.Min[0]), static_cast<float>(ortho_box.Min[1]), static_cast<float>(ortho_box.Min[2])};
                for (const auto& plane : planes)
                {
                    if (plane.DotCoordinate(max) < 0 && plane.DotCoordinate(min) < 0)
                    {
                        is_inside = false;
                    }
                }
                if (is_inside)
                {
                    result.push_back(element.first);
                }
            }
            return result;
        }*/

        //virtual ~SceneVisibility() = default;

        static std::vector<std::weak_ptr<IRenderable>> FrustumCulling(DirectX::SimpleMath::Matrix viewproj)
        {
            // Extract frustum planes from the view-projection matrix.
            auto dxplanes = ExtractFrustumPlanesPointInside(viewproj);
            auto otplanes = std::vector<OrthoTree::Plane3D>(6);

            // Convert DirectX frustum planes to OrthoTree's Plane3D format.
            for (int i = 0; i < 6; i++)
            {
                auto dxnorm = dxplanes[i].Normal();

                if (!(std::abs(dxnorm.LengthSquared() - 1.0) < 0.000001))
                {
                    el::Loggers::getLogger(LogWorld)->error("Length %v", dxnorm.LengthSquared());
                    throw std::runtime_error("Plane Normal Length error");
                }

                OrthoTree::Vector3D normal = {dxnorm.x, dxnorm.y, dxnorm.z};
                otplanes[i] = OrthoTree::Plane3D(dxplanes[i].D(), normal);
            }

            // Perform frustum culling to get IDs of visible entities.
            auto& inst = GetInstance();
            auto ent_ids = inst->quadtree.FrustumCulling(otplanes, 0.1, inst->visibility_to_geometry_);

            std::vector<std::weak_ptr<IRenderable>> result;
            for (auto ent_id : ent_ids)
            {
                // Attempt to lock and retrieve the renderable object.
                if (!inst->visibility_to_renderable_[ent_id].expired())
                    result.push_back(inst->visibility_to_renderable_[ent_id]);
            }
            return result;
        }

        // Extract visible objects matching a filter and organize them into draw packets.
        static std::unordered_map<ObjectMask, DrawPacket> Extract(ObjectMask render_filter_type, const std::vector<std::weak_ptr<IRenderable>>& renderables)
        {
            // Map to store draw packets grouped by object mask.
            std::unordered_map<ObjectMask, DrawPacket> mask_to_draw_packets;

            // Iterate through each entity ID from frustum culling.
            for (auto weak_renderable : renderables)
            {
                // Attempt to lock and retrieve the renderable object.
                if (std::shared_ptr<IRenderable> renderable = weak_renderable.lock())
                {
                    // Get sorting data (e.g., mask and shader resource).
                    auto sort_data = renderable->GetSortData();

                    // Apply render filter to the object mask.
                    // If the object matches the filter, process it.
                    ObjectMask object_mask = sort_data.object_mask;
                    if ((object_mask & render_filter_type) == object_mask)
                    {
                        // Check if a draw packet already exists for this mask.
                        auto drawPacket = mask_to_draw_packets.find(object_mask);

                        // Create a new draw packet if one doesn't exist.
                        if (drawPacket == mask_to_draw_packets.end())
                            mask_to_draw_packets.emplace(object_mask, DrawPacket{object_mask});

                        drawPacket = mask_to_draw_packets.find(object_mask);

                        // Find or create a resource group for the object's shader resource.
                        auto CMR = drawPacket->second.common_resource_renderables.find(sort_data.shaderResource.get());
                        if (CMR == drawPacket->second.common_resource_renderables.end())
                            drawPacket->second.common_resource_renderables.emplace(sort_data.shaderResource.get(), CommonResourceGroup(sort_data.shaderResource));

                        CMR = drawPacket->second.common_resource_renderables.find(sort_data.shaderResource.get());

                        // Add the renderable to the resource group's renderables list.
                        CMR->second.renderables.push_back(renderable);
                    }
                }
                else
                // Throw an error if the renderable is null.
                    throw std::runtime_error("SceneVisibility::SceneVisibility(): renderable is null");
            }

            return mask_to_draw_packets;
        }

        static std::unordered_map<ObjectMask, DrawPacket> ExtractFromFrustum(ObjectMask render_filter_type, DirectX::SimpleMath::Matrix viewproj)
        {
            const auto& culling = FrustumCulling(viewproj);
            return Extract(render_filter_type, culling);
        }

        static void Expand(const std::unordered_map<ObjectMask, DrawPacket>& from_packets, std::unordered_map<ObjectMask, DrawPacket>& result_packets)
        {
            for (auto& [filter, packet] : from_packets)
            {
                if (!result_packets.contains(filter))
                    result_packets.emplace(filter, packet);
                else
                {
                    auto common_packet = result_packets.find(filter);
                    for (auto& [shaderResource, resourceGroup] : packet.common_resource_renderables)
                    {
                        if (!common_packet->second.common_resource_renderables.contains(resourceGroup.shaderResource.get()))
                            common_packet->second.common_resource_renderables.emplace(shaderResource, resourceGroup);
                        else
                        {
                            auto common_resourceGroup = common_packet->second.common_resource_renderables.find(shaderResource);
                            for (const auto& renderable : resourceGroup.renderables)
                            {
                                //auto& com_rend = common_resourceGroup.renderables;
                                bool contains = false;
                                for (auto& com_rend : common_resourceGroup->second.renderables)
                                {
                                    if (com_rend == renderable)
                                    {
                                        contains = true;
                                        break;
                                    }
                                }
                                if (!contains)
                                {
                                    common_resourceGroup->second.renderables.push_back(renderable);
                                }
                            }
                        }
                    }
                }
            }
        }

        static void ExpandByFilterFromAll(ObjectMask render_filter_type, std::unordered_map<ObjectMask, DrawPacket>& result_packets)
        {
            // Iterate through each entity ID from frustum culling.
            for (const auto& [ent_id, renderable] : GetInstance()->visibility_to_renderable_)
            {
                // Attempt to lock and retrieve the renderable object.

                // Get sorting data (e.g., mask and shader resource).
                auto sort_data = renderable.lock()->GetSortData();

                // Apply render filter to the object mask.
                // If the object matches the filter, process it.
                ObjectMask object_mask = sort_data.object_mask;
                if ((object_mask & render_filter_type) == object_mask)
                {
                    // Check if a draw packet already exists for this mask.
                    auto drawPacket = result_packets.find(object_mask);

                    // Create a new draw packet if one doesn't exist.
                    if (drawPacket == result_packets.end())
                        result_packets.emplace(object_mask, DrawPacket{object_mask});

                    drawPacket = result_packets.find(object_mask);

                    // Find or create a resource group for the object's shader resource.
                    auto CMR = drawPacket->second.common_resource_renderables.find(sort_data.shaderResource.get());
                    if (CMR == drawPacket->second.common_resource_renderables.end())
                        drawPacket->second.common_resource_renderables.emplace(sort_data.shaderResource.get(), CommonResourceGroup(sort_data.shaderResource));

                    CMR = drawPacket->second.common_resource_renderables.find(sort_data.shaderResource.get());

                    // Add the renderable to the resource group's renderables list.
                    CMR->second.renderables.push_back(renderable);
                }
            }
        }

        static void ExpandByFilterFromFrustum(ObjectMask render_filter_type, DirectX::SimpleMath::Matrix viewproj, std::unordered_map<ObjectMask, DrawPacket>& result_packets)
        {
            // Extract frustum planes from the view-projection matrix.
            const auto culling = FrustumCulling(viewproj);
            const auto res = Extract(render_filter_type, culling);
            Expand(res, result_packets);
        }

        static void Tick()
        {
            GetInstance()->quadtree = OrthoTree::OctreeBoxMap
            {
                GetInstance()->visibility_to_geometry_,
                10,
                OrthoTree::BoundingBox3D{
                    {
                        -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max()
                    },
                    {
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max()
                    }
                },
                20
            };
        }

        static std::unique_ptr<SceneVisibility>& GetInstance()
        {
            if (instance_) return instance_;
            else
            {
                instance_ = std::make_unique<SceneVisibility>();
                return instance_;
            }
        }

        static OrthoTree::index_t Register(std::shared_ptr<IRenderable> renderable, OrthoTree::BoundingBox3D box)
        {
            auto ent_id = current_free_id_++;
            GetInstance()->visibility_to_renderable_[ent_id] = renderable;
            GetInstance()->visibility_to_geometry_[ent_id] = box;
            return ent_id;
        }

        static void Unregister(OrthoTree::index_t ent_id)
        {
            GetInstance()->visibility_to_renderable_.erase(ent_id);
            GetInstance()->visibility_to_geometry_.erase(ent_id);
        }

        static void Update(OrthoTree::index_t ent_id, OrthoTree::BoundingBox3D newBox)
        {
            GetInstance()->visibility_to_geometry_[ent_id] = newBox;
        }

    protected:
        static inline std::unique_ptr<SceneVisibility> instance_ = nullptr;
        static inline OrthoTree::index_t current_free_id_ = 0;

        std::unordered_map<OrthoTree::index_t, std::weak_ptr<IRenderable>> visibility_to_renderable_;
        std::unordered_map<OrthoTree::index_t, OrthoTree::BoundingBox3D> visibility_to_geometry_;

        OrthoTree::OctreeBoxMap quadtree;
    };

    // todo: create DirectX::Math to OrthoTree conversions or adapter
    class VisibilityEntry
    {
    public:
        static std::unique_ptr<VisibilityEntry> Register(std::shared_ptr<IRenderable> renderable, DirectX::BoundingBox aabb = DirectX::BoundingBox())
        {
            OrthoTree::Vector3D min{
                aabb.Center.x - aabb.Extents.x,
                aabb.Center.y - aabb.Extents.y,
                aabb.Center.z - aabb.Extents.z
            };

            OrthoTree::Vector3D max{
                aabb.Center.x + aabb.Extents.x,
                aabb.Center.y + aabb.Extents.y,
                aabb.Center.z + aabb.Extents.z
            };

            return std::unique_ptr<VisibilityEntry>(new VisibilityEntry(SceneVisibility::Register(renderable, OrthoTree::BoundingBox3D{min, max})));
        }

        ~VisibilityEntry()
        {
            SceneVisibility::Unregister(entity_index_);
        }

        void Update(DirectX::BoundingBox aabb)
        {
            OrthoTree::Vector3D min{
                aabb.Center.x - aabb.Extents.x,
                aabb.Center.y - aabb.Extents.y,
                aabb.Center.z - aabb.Extents.z
            };

            OrthoTree::Vector3D max{
                aabb.Center.x + aabb.Extents.x,
                aabb.Center.y + aabb.Extents.y,
                aabb.Center.z + aabb.Extents.z
            };

            SceneVisibility::Update(entity_index_, OrthoTree::BoundingBox3D{min, max});
        }

    private:
        VisibilityEntry(OrthoTree::index_t id):
            entity_index_(id)
        {
        }

        OrthoTree::index_t entity_index_;
    };
}
