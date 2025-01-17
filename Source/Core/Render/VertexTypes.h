#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<array>

using namespace DirectX;

namespace GiiGa
{
    struct Index16
    {
        uint16_t index;
        static inline const DXGI_FORMAT Format = DXGI_FORMAT_R16_UINT;
    };

    struct VertexPosition
    {
        friend struct VertexPNTBT;
        
        VertexPosition() = default;

        explicit VertexPosition(const SimpleMath::Vector3& position) : Position(position) {}

        SimpleMath::Vector3 Position;

        //virtual ~VertexPosition() = default;

    private:
        static inline const int InputElementCount = 1;

        static inline const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
    public:
        static inline const D3D12_INPUT_LAYOUT_DESC InputLayout = {
            InputElements,
            InputElementCount
        };
    };

    struct VertexPNTBT
    {
        friend struct VertexBoned;

        VertexPNTBT() = default;

        explicit VertexPNTBT(const SimpleMath::Vector3& position,
            const SimpleMath::Vector3& normal,
            const SimpleMath::Vector3& tangent = {0, 0, 0},
            const SimpleMath::Vector3& bitangent = {0, 0, 0},
            const SimpleMath::Vector2& texCoord = {0,0})
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
        SimpleMath::Vector2 TexCoord;

        //virtual ~VertexPNTBT() = default;

    private:

        static inline const int InputElementCount = 5;
        static inline const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount] =
        {
            VertexPosition::InputElements[0],
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };

    public:
        static inline const D3D12_INPUT_LAYOUT_DESC InputLayout = {
            InputElements,
            InputElementCount
        };
    };

    struct VertexBoned
    {
        static inline constexpr uint8_t MaxCountOfBonesPerVertex = 4;

        explicit VertexBoned(const SimpleMath::Vector3& position,
            const SimpleMath::Vector3& normal,
            const SimpleMath::Vector3& tangent = {0, 0, 0},
            const SimpleMath::Vector3& bitangent = {0, 0, 0},
            const SimpleMath::Vector2& texCoord = {0,0},
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
        SimpleMath::Vector2 TexCoord;
        std::array<uint16_t, MaxCountOfBonesPerVertex> BoneIDs;
        SimpleMath::Vector4 BoneWeights;
        
        //virtual ~VertexBoned() = default;

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