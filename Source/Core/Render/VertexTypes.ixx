module;

#include <d3d12.h>
#include <directxtk/SimpleMath.h>
#include <array>

export module VertexTypes;

using namespace DirectX;

namespace GiiGa
{
    export struct Index16
    {
        uint16_t index;
        static inline const DXGI_FORMAT Format = DXGI_FORMAT_R16_UINT;
    };

    export enum class VertexTypes
    {
        None = 0,
        VertexPosition = 1,
        VertexPNTBT = 2,
        VertexBoned = 4
    };

    export struct VertexPNTBT
    {
        friend struct VertexBoned;

        VertexPNTBT() = default;

        explicit VertexPNTBT(const SimpleMath::Vector3& position,
            const SimpleMath::Vector3& normal,
            const SimpleMath::Vector3& texCoord,
            const SimpleMath::Vector3& tangent = {0, 0, 0},
            const SimpleMath::Vector3& bitangent = {0, 0, 0})
            : Position(position)
              , Normal(normal)
              , Tangent(tangent)
              , Bitangent(bitangent)
              , TexCoord(texCoord)
        {
        }

        SimpleMath::Vector3 Position;
        SimpleMath::Vector3 Normal;
        SimpleMath::Vector3 Tangent;
        SimpleMath::Vector3 Bitangent;
        SimpleMath::Vector3 TexCoord;

    private:

        static inline const int InputElementCount = 5;
        static inline const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

    public:
        static inline const D3D12_INPUT_LAYOUT_DESC InputLayout = {
            InputElements,
            InputElementCount
        };
    };

    export struct VertexBoned
    {
        static inline constexpr uint8_t MaxCountOfBonesPerVertex = 4;

        explicit VertexBoned(const SimpleMath::Vector3& position,
            const SimpleMath::Vector3& normal,
            const SimpleMath::Vector3& texCoord,
            const SimpleMath::Vector3& tangent = {0, 0, 0},
            const SimpleMath::Vector3& bitangent = {0, 0, 0},
            const std::array<uint16_t, MaxCountOfBonesPerVertex>& boneIDs = {0, 0, 0, 0},
            const SimpleMath::Vector4& boneWeights = {0, 0, 0, 0})
            : Position(position)
              , Normal(normal)
              , Tangent(tangent)
              , Bitangent(bitangent)
              , TexCoord(texCoord)
              , BoneIDs(boneIDs)
              , BoneWeights(boneWeights)
        {
        }

        SimpleMath::Vector3 Position;
        SimpleMath::Vector3 Normal;
        SimpleMath::Vector3 Tangent;
        SimpleMath::Vector3 Bitangent;
        SimpleMath::Vector3 TexCoord;
        std::array<uint16_t, MaxCountOfBonesPerVertex> BoneIDs;
        SimpleMath::Vector4 BoneWeights;

    private:
        static inline const int InputElementCount = 7;
        static inline const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount] =
        {
            VertexPNTBT::InputElements[0],
            VertexPNTBT::InputElements[1],
            VertexPNTBT::InputElements[2],
            VertexPNTBT::InputElements[3],
            VertexPNTBT::InputElements[4],
            {"BLENDINDICES", 0, DXGI_FORMAT_R16G16B16A16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0},
            {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0},
        };

    public:
        static inline const D3D12_INPUT_LAYOUT_DESC InputLayout = {
            InputElements,
            InputElementCount
        };
    };
}