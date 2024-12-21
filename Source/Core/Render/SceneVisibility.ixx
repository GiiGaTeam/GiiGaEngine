module;

#define NOMINMAX
#include <directxtk12/SimpleMath.h>
#include<directxtk12/SimpleMath.inl>
#include<Octree/octree.h>

export module SceneVisibility;

import <DirectXCollision.h>;
import <memory>;
import <vector>;
import <limits>;
import <unordered_map>;

import IRenderable;
import ITickable;
export import ObjectMask;
import MathUtils;

namespace GiiGa
{
    //todo: add sorted Extract
    export class SceneVisibility
    {
    public:
        SceneVisibility()
        {
        }

        //virtual ~SceneVisibility() = default;

        // Extract visible objects matching a filter and organize them into draw packets.
        static std::unordered_map<ObjectMask, DrawPacket> Extract(ObjectMask render_filter_type, DirectX::SimpleMath::Matrix viewproj)
        {
            // Extract frustum planes from the view-projection matrix.
            auto dxplanes = ExtractFrustumPlanesPointInside(viewproj);
            auto otplanes = std::vector<OrthoTree::Plane3D>(6);

            // Convert DirectX frustum planes to OrthoTree's Plane3D format.
            for (int i = 0; i < 6; i++)
            {
                OrthoTree::Vector3D normal = {dxplanes[i].Normal().x, dxplanes[i].Normal().y, dxplanes[i].Normal().z};
                otplanes[i] = OrthoTree::Plane3D(dxplanes[i].D(), normal);
            }

            // Perform frustum culling to get IDs of visible entities.
            auto ent_ids = GetInstance()->quadtree.FrustumCulling(otplanes, 0.1);

            // Map to store draw packets grouped by object mask.
            std::unordered_map<ObjectMask, DrawPacket> mask_to_draw_packets;

            // Iterate through each entity ID from frustum culling.
            for (auto ent_id : ent_ids)
            {
                // Attempt to lock and retrieve the renderable object.
                if (std::shared_ptr<IRenderable> renderable = GetInstance()->visibility_to_renderable_[ent_id].lock())
                {
                    // Get sorting data (e.g., mask and shader resource).
                    auto sort_data = renderable->GetSortData();

                    // Apply render filter to the object mask.
                    ObjectMask mask_with_filter = sort_data.object_mask & render_filter_type;

                    // If the object matches the filter, process it.
                    if (mask_with_filter.any())
                    {
                        // Check if a draw packet already exists for this mask.
                        auto drawPacket = mask_to_draw_packets.find(mask_with_filter);

                        // Create a new draw packet if one doesn't exist.
                        if (drawPacket == mask_to_draw_packets.end())
                            mask_to_draw_packets.emplace(mask_with_filter, DrawPacket{mask_with_filter});

                        drawPacket = mask_to_draw_packets.find(mask_with_filter);

                        // Find or create a resource group for the object's shader resource.
                        auto t = sort_data.shaderResource.get();
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

        static std::unique_ptr<SceneVisibility>& GetInstance()
        {
            if (instance_) return instance_;
            else return instance_ = std::make_unique<SceneVisibility>();
        }

        static OrthoTree::index_t Register(std::shared_ptr<IRenderable> renderable, OrthoTree::BoundingBox3D box)
        {
            auto ent_id = current_free_id_++;
            if (!GetInstance()->quadtree.Add(ent_id, box, true))
                throw std::runtime_error("SceneVisibility::Register(): failed to add quadtree");
            GetInstance()->visibility_to_renderable_[ent_id] = renderable;
            return ent_id;
        }

        static void Unregister(OrthoTree::index_t ent_id)
        {
            if (!GetInstance()->quadtree.Erase(ent_id))
                throw std::runtime_error("SceneVisibility::Unregister(): failed to remove quadtree");
            GetInstance()->visibility_to_renderable_.erase(ent_id);
        }

        static void Update(OrthoTree::index_t ent_id, OrthoTree::BoundingBox3D newBox)
        {
            GetInstance()->instance_->quadtree.Update(ent_id, newBox);
        }

    protected:
        static inline std::unique_ptr<SceneVisibility> instance_ = nullptr;
        static inline OrthoTree::index_t current_free_id_ = 0;

        std::unordered_map<OrthoTree::index_t, std::weak_ptr<IRenderable>> visibility_to_renderable_;

        OrthoTree::TreeBoxContainerND<3, 2, OrthoTree::BaseGeometryType, std::unordered_map<OrthoTree::index_t, OrthoTree::BoundingBox3D>> quadtree
        {
            {},
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
    };

    // todo: create DirectX::Math to OrthoTree conversions or adapter
    export class VisibilityEntry
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
