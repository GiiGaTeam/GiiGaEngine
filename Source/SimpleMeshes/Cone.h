/*
module;

#include <chrono>
#include <d3d12.h>
#include <random>
#include <span>
#include <directx/d3dx12_core.h>
#include <directxtk12/SimpleMath.h>
export module Cone;
import Mesh;
import VertexTypes;
import GPULocalResource;
import RenderDevice;
import IRenderContext;

namespace GiiGa
{
    export class Cone : public Mesh
    {
        //using namespace DirectX;
        
        public:
        Cone(IRenderContext& render_context, RenderDevice& device,
            SimpleMath::Vector3 head, SimpleMath::Vector3 direction, float height, float angle)
        {            
            //MeshGeometry* cone_geom = new MeshGeometry();
            std::vector<SimpleMath::Vector4> vertices;
            std::vector<uint16_t> indices;
            
            vertices.push_back(SimpleMath::Vector4(head.x, head.y, head.z, 1));

            SimpleMath::Vector4 perpendicular4 = GetRandomPerpendicular(SimpleMath::Vector4(direction.x, direction.y, direction.z, 0));
            SimpleMath::Vector4 b;
            {
                SimpleMath::Vector3 perpendicular3 = SimpleMath::Vector3(perpendicular4.x, perpendicular4.y, perpendicular4.z);
                SimpleMath::Vector3 res = direction.Cross(perpendicular3);
                b = SimpleMath::Vector4(res.x, res.y, res.z, 0);
            }

            //root->vertecies[i].position = light_data.PositionWS;
            //DirectX::SimpleMath::Vector4 root_proj = DirectX::SimpleMath::Vector4(1,0,0,1);
            DirectX::SimpleMath::Vector4 root_proj = head + direction * height;
            float Pi = acos(0.0f);
            unsigned int n = 16;
            float radius = height * tan(Pi / 180.0f * angle);
            float phi = 0;
            
            float delta_phi = 2 * Pi / n;
            //Заполняем точки на окружности
            for (int i = 1; i <= n; i++)
            {
                SimpleMath::Vector4 point_on_base = root_proj + radius * cos(phi) * perpendicular4 + radius * sin(phi) * b;
                vertices.push_back(point_on_base);
                phi += delta_phi;
            }
            //Собираем треугольники на боковой поверхности
            float index_on_base = 1;
            for (int i = 0; i < n; i++)
            {
                indices.push_back(0);
                indices.push_back(index_on_base);
                if (i == n - 1) indices.push_back(1);
                else indices.push_back(++index_on_base);
            }
            //Собираем треугольники на основании
            index_on_base = 2;
            for (int i = 0; i < n - 2; i++)
            {
                indices.push_back(1);
                indices.push_back(++index_on_base);
                indices.push_back(index_on_base - 1);
            }

            SimpleMath::Quaternion q;
            {
                DirectX::SimpleMath::Vector3 unit_vector = DirectX::SimpleMath::Vector3(1, 0, 0);
                DirectX::SimpleMath::Vector3 cp = direction.Cross(unit_vector);
                q = DirectX::SimpleMath::Quaternion(cp.x, cp.y, cp.z, 0);
                q.w = sqrtf(powf(direction.Length(), 2) + (powf(unit_vector.Length(), 2))) + direction.Dot(unit_vector);
                q.Normalize();
            }

            /*vertexBuffer_ = GPULocalResource(device, CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(SimpleMath::Vector4), D3D12_RESOURCE_FLAG_NONE));
            indexBuffer_ = GPULocalResource(device, CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(Index16), D3D12_RESOURCE_FLAG_NONE));

            const auto vertices_span = std::span{reinterpret_cast<const uint8_t*>(vertices.data()), vertices.size() * sizeof(VertexType)};
            vertexBuffer_.UpdateContents(render_context, vertices_span, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

            vertexView_ = vertexBuffer_.CreateVetexBufferView(
                D3D12_VERTEX_BUFFER_VIEW{0, static_cast<UINT>(vertices.size() * sizeof(VertexType)), sizeof(VertexType)});

            indexCount = static_cast<UINT>(indices.size());
            auto indices_span = std::span{reinterpret_cast<const uint8_t*>(indices.data()), indices.size() * sizeof(Index16)};
            indexBuffer_.UpdateContents(render_context, indices_span, D3D12_RESOURCE_STATE_INDEX_BUFFER);

            indexView_ = indexBuffer_.CreateIndexBufferView(
                D3D12_INDEX_BUFFER_VIEW{0, static_cast<UINT>(indices.size() * sizeof(Index16)), Index16::Format});#1#
        }
    private:
        DirectX::SimpleMath::Vector4 GetRandomPerpendicular(DirectX::SimpleMath::Vector4 v4)
        {
            bool bx0 = false, by0 = false, bz0 = false;
            float x, y, z = 0;
            if (v4.x == 0) bx0 = true;
            if (v4.y == 0) by0 = true;
            if (v4.z == 0) bz0 = true;
            DirectX::SimpleMath::Vector4 ret = DirectX::SimpleMath::Vector4(0,0,0,0);
            auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::default_random_engine eng(seed);
            std::uniform_real_distribution<> distr(-1, 1);
    
            if (bx0 && by0 && bz0) return ret;
            if (bx0 && by0)
            {
                ret.x = static_cast<float>(distr(eng));
                ret.y = static_cast<float>(distr(eng));
                ret.Normalize();
                return ret;
            }
            if (bx0 && bz0)
            {
                ret.x = static_cast<float>(distr(eng));
                ret.z = static_cast<float>(distr(eng));
                ret.Normalize();
                return ret;
            }
            if (by0 && bz0)
            {
                ret.y = static_cast<float>(distr(eng));
                ret.z = static_cast<float>(distr(eng));
                ret.Normalize();
                return ret;
            }
            if (bx0)
            {
                ret.x = static_cast<float>(distr(eng));
                ret.Normalize();
                return ret;
            }
            if (by0)
            {
                ret.y = static_cast<float>(distr(eng));
                ret.Normalize();
                return ret;
            }
            if (bz0)
            {
                ret.z = static_cast<float>(distr(eng));
                ret.Normalize();
                return ret;
            }
    
            ret.x = static_cast<float>(distr(eng));
            ret.y = static_cast<float>(distr(eng));
            ret.z = -(ret.x*v4.x + ret.y*v4.y)/v4.z;
            return ret;
        }
    };
}
*/
