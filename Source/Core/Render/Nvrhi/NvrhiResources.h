#pragma once

// NvrhiResources.h
//
// Convenience helpers for the NVRHI-backed renderer. NVRHI already provides
// nvrhi::BufferHandle / TextureHandle / InputLayoutHandle and upload via
// ICommandList::writeBuffer, so there are no wrapper classes here — only free functions
// that replace the old GPULocalResource + UploadBuffer + BufferView + Mesh boilerplate
// and the D3D12_INPUT_ELEMENT_DESC arrays in VertexTypes.h.

#include "NvrhiCommon.h"

#include <vector>

#include <VertexTypes.h> // engine vertex types (bridge layer)

namespace GiiGa::rhi
{
    // --- Buffers ---

    // Create a committed vertex buffer and upload `data` (must stay alive until the command
    // list closes). Returns the buffer handle; NVRHI transitions it automatically on use.
    template <typename T>
    nvrhi::BufferHandle createVertexBuffer(nvrhi::IDevice* device, nvrhi::ICommandList* cl,
                                           const T* data, size_t count, const char* debugName = "vertex buffer")
    {
        nvrhi::BufferDesc d;
        d.setByteSize(count * sizeof(T)).setIsVertexBuffer(true)
         .setInitialState(nvrhi::ResourceStates::VertexBuffer).setKeepInitialState(true)
         .setDebugName(debugName);
        nvrhi::BufferHandle buf = device->createBuffer(d);
        if (cl && data && count) cl->writeBuffer(buf, data, count * sizeof(T));
        return buf;
    }

    template <typename T>
    nvrhi::BufferHandle createVertexBuffer(nvrhi::IDevice* device, nvrhi::ICommandList* cl,
                                           const std::vector<T>& vertices, const char* debugName = "vertex buffer")
    {
        return createVertexBuffer<T>(device, cl, vertices.data(), vertices.size(), debugName);
    }

    // Create a committed index buffer and upload `data`. `indexFormat` must match T
    // (R16_UINT for Index16, R32_UINT for uint32_t).
    template <typename T>
    nvrhi::BufferHandle createIndexBuffer(nvrhi::IDevice* device, nvrhi::ICommandList* cl,
                                          const T* data, size_t count, nvrhi::Format indexFormat,
                                          const char* debugName = "index buffer")
    {
        nvrhi::BufferDesc d;
        d.setByteSize(count * sizeof(T)).setIsIndexBuffer(true)
         .setInitialState(nvrhi::ResourceStates::IndexBuffer).setKeepInitialState(true)
         .setDebugName(debugName);
        nvrhi::BufferHandle buf = device->createBuffer(d);
        if (cl && data && count) cl->writeBuffer(buf, data, count * sizeof(T));
        return buf;
    }

    template <typename T>
    nvrhi::BufferHandle createIndexBuffer(nvrhi::IDevice* device, nvrhi::ICommandList* cl,
                                          const std::vector<T>& indices, nvrhi::Format indexFormat,
                                          const char* debugName = "index buffer")
    {
        return createIndexBuffer<T>(device, cl, indices.data(), indices.size(), indexFormat, debugName);
    }

    // Static (committed) constant buffer — update with ICommandList::writeBuffer. D3D12
    // requires constant buffer sizes to be 256-byte aligned, which we enforce here.
    inline nvrhi::BufferHandle createConstantBuffer(nvrhi::IDevice* device, uint64_t byteSize,
                                                    const char* debugName = "constant buffer")
    {
        uint64_t aligned = (byteSize + 255) & ~uint64_t(255);
        nvrhi::BufferDesc d;
        d.setByteSize(aligned).setIsConstantBuffer(true).setDebugName(debugName);
        return device->createBuffer(d);
    }

    // Volatile constant buffer — per-draw data uploaded with writeBuffer; sub-allocated by
    // NVRHI from the command list's upload heap. maxVersions applies on Vulkan only.
    inline nvrhi::BufferHandle createVolatileConstantBuffer(nvrhi::IDevice* device, uint32_t byteSize,
                                                             uint32_t maxVersions = 16,
                                                             const char* debugName = "volatile cb")
    {
        nvrhi::BufferDesc d;
        d.setByteSize(byteSize).setIsVolatile(true).setMaxVersions(maxVersions).setDebugName(debugName);
        return device->createBuffer(d);
    }

    // --- Vertex layouts (NVRHI replacement for the D3D12_INPUT_ELEMENT_DESC arrays) ---

    template <typename VertexType>
    std::vector<nvrhi::VertexAttributeDesc> makeVertexAttributes();

    template <>
    inline std::vector<nvrhi::VertexAttributeDesc> makeVertexAttributes<VertexPosition>()
    {
        static_assert(sizeof(VertexPosition) == 12, "unexpected VertexPosition layout");
        std::vector<nvrhi::VertexAttributeDesc> a;
        a.push_back(nvrhi::VertexAttributeDesc{}
                    .setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT)
                    .setBufferIndex(0).setOffset(0).setElementStride(sizeof(VertexPosition)));
        return a;
    }

    template <>
    inline std::vector<nvrhi::VertexAttributeDesc> makeVertexAttributes<VertexPNTBT>()
    {
        static_assert(sizeof(VertexPNTBT) == 56, "unexpected VertexPNTBT layout");
        std::vector<nvrhi::VertexAttributeDesc> a;
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(0).setElementStride(sizeof(VertexPNTBT)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("NORMAL").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(12).setElementStride(sizeof(VertexPNTBT)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("TANGENT").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(24).setElementStride(sizeof(VertexPNTBT)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("BINORMAL").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(36).setElementStride(sizeof(VertexPNTBT)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setBufferIndex(0).setOffset(48).setElementStride(sizeof(VertexPNTBT)));
        return a;
    }

    template <>
    inline std::vector<nvrhi::VertexAttributeDesc> makeVertexAttributes<VertexBoned>()
    {
        static_assert(sizeof(VertexBoned) == 80, "unexpected VertexBoned layout");
        std::vector<nvrhi::VertexAttributeDesc> a;
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("POSITION").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(0).setElementStride(sizeof(VertexBoned)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("NORMAL").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(12).setElementStride(sizeof(VertexBoned)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("TANGENT").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(24).setElementStride(sizeof(VertexBoned)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("BINORMAL").setFormat(nvrhi::Format::RGB32_FLOAT).setBufferIndex(0).setOffset(36).setElementStride(sizeof(VertexBoned)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("TEXCOORD").setFormat(nvrhi::Format::RG32_FLOAT).setBufferIndex(0).setOffset(48).setElementStride(sizeof(VertexBoned)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("BLENDINDICES").setFormat(nvrhi::Format::RGBA16_UINT).setBufferIndex(0).setOffset(56).setElementStride(sizeof(VertexBoned)));
        a.push_back(nvrhi::VertexAttributeDesc{}.setName("BLENDWEIGHT").setFormat(nvrhi::Format::RGBA32_FLOAT).setBufferIndex(0).setOffset(64).setElementStride(sizeof(VertexBoned)));
        return a;
    }

    // Build an NVRHI input layout from a set of vertex attributes. The vertex shader is
    // required by the NVRHI API (used for D3D11; informational on D3D12).
    inline nvrhi::InputLayoutHandle createInputLayout(nvrhi::IDevice* device,
                                                      const std::vector<nvrhi::VertexAttributeDesc>& attrs,
                                                      nvrhi::IShader* vertexShader)
    {
        return device->createInputLayout(attrs.data(), static_cast<uint32_t>(attrs.size()), vertexShader);
    }
}
