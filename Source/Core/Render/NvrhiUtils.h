#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>
#include<nvrhi/d3d12.h>
#include<nvrhi/utils.h>
#ifndef NDEBUG
#include<nvrhi/validation.h>
#endif

#include<iostream>
#include<span>
#include<stdexcept>

namespace GiiGa
{
    // Routes NVRHI diagnostics to the engine log/console.
    class NvrhiMessageCallback : public nvrhi::IMessageCallback
    {
    public:
        static NvrhiMessageCallback& GetInstance()
        {
            static NvrhiMessageCallback instance;
            return instance;
        }

        void message(nvrhi::MessageSeverity severity, const char* messageText) override
        {
            const char* severityName = "INFO";
            switch (severity)
            {
            case nvrhi::MessageSeverity::Info: severityName = "INFO";
                break;
            case nvrhi::MessageSeverity::Warning: severityName = "WARNING";
                break;
            case nvrhi::MessageSeverity::Error: severityName = "ERROR";
                break;
            case nvrhi::MessageSeverity::Fatal: severityName = "FATAL";
                break;
            }

            std::cout << "[NVRHI][" << severityName << "] " << messageText << std::endl;

            if (severity == nvrhi::MessageSeverity::Fatal)
                throw std::runtime_error(std::string("NVRHI fatal error: ") + messageText);
        }
    };

    // DXGI -> nvrhi::Format, built as the inverse of nvrhi::d3d12::convertFormat.
    inline nvrhi::Format DxgiToNvrhiFormat(DXGI_FORMAT dxgi_format)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(nvrhi::Format::COUNT); ++i)
        {
            const auto format = static_cast<nvrhi::Format>(i);
            if (nvrhi::d3d12::convertFormat(format) == dxgi_format)
                return format;
        }
        return nvrhi::Format::UNKNOWN;
    }

    // Wraps an existing native texture (e.g. loaded by DirectXTK12) into an nvrhi handle.
    // The texture is expected to already contain its data and be in a shader-readable state.
    inline nvrhi::TextureHandle WrapNativeTexture(nvrhi::IDevice* device, ID3D12Resource* resource, const char* debug_name = "WrappedTexture")
    {
        const D3D12_RESOURCE_DESC native_desc = resource->GetDesc();

        nvrhi::TextureDesc desc;
        desc.width = static_cast<uint32_t>(native_desc.Width);
        desc.height = native_desc.Height;
        desc.mipLevels = native_desc.MipLevels;
        desc.format = DxgiToNvrhiFormat(native_desc.Format);
        desc.debugName = debug_name;
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;

        if (native_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
        {
            desc.dimension = nvrhi::TextureDimension::Texture3D;
            desc.depth = native_desc.DepthOrArraySize;
        }
        else if (native_desc.DepthOrArraySize > 1)
        {
            desc.dimension = nvrhi::TextureDimension::Texture2DArray;
            desc.arraySize = native_desc.DepthOrArraySize;
        }
        else
        {
            desc.dimension = nvrhi::TextureDimension::Texture2D;
        }

        return device->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, nvrhi::Object(resource), desc);
    }

    // Common helper: a GPU-local constant buffer usable in binding sets, updated via writeBuffer.
    inline nvrhi::BufferHandle CreateConstantBuffer(nvrhi::IDevice* device, size_t size, const char* debug_name)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = size;
        desc.isConstantBuffer = true;
        desc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        desc.keepInitialState = true;
        desc.debugName = debug_name;
        return device->createBuffer(desc);
    }

    inline nvrhi::BufferHandle CreateStructuredBuffer(nvrhi::IDevice* device, size_t size, size_t stride, const char* debug_name)
    {
        nvrhi::BufferDesc desc;
        desc.byteSize = size;
        desc.structStride = static_cast<uint32_t>(stride);
        desc.initialState = nvrhi::ResourceStates::ShaderResource;
        desc.keepInitialState = true;
        desc.debugName = debug_name;
        return device->createBuffer(desc);
    }
}
