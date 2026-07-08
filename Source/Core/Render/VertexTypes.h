#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<nvrhi/nvrhi.h>
#include<array>

using namespace DirectX;

namespace GiiGa
{
    struct Index16
    {
        uint16_t index;
        static inline const nvrhi::Format Format = nvrhi::Format::R16_UINT;
    };

    struct VertexPosition
    {
        friend struct VertexPNTBT;

        VertexPosition() = default;

        explicit VertexPosition(const SimpleMath::Vector3& position) : Position(position)
        {
        }

        SimpleMath::Vector3 Position;

        static inline const std::array<nvrhi::VertexAttributeDesc, 1> InputLayout =
        {
            nvrhi::VertexAttributeDesc()
            .setName("POSITION")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(0)
            .setElementStride(sizeof(SimpleMath::Vector3)),
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
                             const SimpleMath::Vector2& texCoord = {0, 0})
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

        static const std::array<nvrhi::VertexAttributeDesc, 5> InputLayout;
    };

    inline const std::array<nvrhi::VertexAttributeDesc, 5> VertexPNTBT::InputLayout =
    {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexPNTBT, Position)).setElementStride(sizeof(VertexPNTBT)),
        nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexPNTBT, Normal)).setElementStride(sizeof(VertexPNTBT)),
        nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexPNTBT, Tangent)).setElementStride(sizeof(VertexPNTBT)),
        nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexPNTBT, Bitangent)).setElementStride(sizeof(VertexPNTBT)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)
                                    .setOffset(offsetof(VertexPNTBT, TexCoord)).setElementStride(sizeof(VertexPNTBT)),
    };

    struct VertexBoned
    {
        static inline constexpr uint8_t MaxCountOfBonesPerVertex = 4;

        explicit VertexBoned(const SimpleMath::Vector3& position,
                             const SimpleMath::Vector3& normal,
                             const SimpleMath::Vector3& tangent = {0, 0, 0},
                             const SimpleMath::Vector3& bitangent = {0, 0, 0},
                             const SimpleMath::Vector2& texCoord = {0, 0},
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

        static const std::array<nvrhi::VertexAttributeDesc, 7> InputLayout;
    };

    inline const std::array<nvrhi::VertexAttributeDesc, 7> VertexBoned::InputLayout =
    {
        nvrhi::VertexAttributeDesc().setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexBoned, Position)).setElementStride(sizeof(VertexBoned)),
        nvrhi::VertexAttributeDesc().setName("NORMAL").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexBoned, Normal)).setElementStride(sizeof(VertexBoned)),
        nvrhi::VertexAttributeDesc().setName("TANGENT").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexBoned, Tangent)).setElementStride(sizeof(VertexBoned)),
        nvrhi::VertexAttributeDesc().setName("BINORMAL").setFormat(nvrhi::Format::RGB32_FLOAT)
                                    .setOffset(offsetof(VertexBoned, Bitangent)).setElementStride(sizeof(VertexBoned)),
        nvrhi::VertexAttributeDesc().setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT)
                                    .setOffset(offsetof(VertexBoned, TexCoord)).setElementStride(sizeof(VertexBoned)),
        nvrhi::VertexAttributeDesc().setName("BLENDINDICES").setFormat(nvrhi::Format::RGBA16_UINT)
                                    .setOffset(offsetof(VertexBoned, BoneIDs)).setElementStride(sizeof(VertexBoned)),
        nvrhi::VertexAttributeDesc().setName("BLENDWEIGHT").setFormat(nvrhi::Format::RGBA32_FLOAT)
                                    .setOffset(offsetof(VertexBoned, BoneWeights)).setElementStride(sizeof(VertexBoned)),
    };
}
