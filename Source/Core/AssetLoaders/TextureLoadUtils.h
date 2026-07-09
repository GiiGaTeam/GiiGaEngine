#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <wrl/client.h>

#include <filesystem>
#include <stdexcept>

#include <directxtk12/WICTextureLoader.h>
#include <directxtk12/DDSTextureLoader.h>
#include <directxtk12/ResourceUploadBatch.h>

#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d12.h>

#include <RenderDevice.h>

namespace GiiGa
{
    inline nvrhi::Format DxgiFormatToNvrhi(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return nvrhi::Format::RGBA8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return nvrhi::Format::SRGBA8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM: return nvrhi::Format::BGRA8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return nvrhi::Format::SBGRA8_UNORM;
        case DXGI_FORMAT_BC1_UNORM: return nvrhi::Format::BC1_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB: return nvrhi::Format::BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM: return nvrhi::Format::BC3_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB: return nvrhi::Format::BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC7_UNORM: return nvrhi::Format::BC7_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB: return nvrhi::Format::BC7_UNORM_SRGB;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return nvrhi::Format::RGBA32_FLOAT;
        default: return nvrhi::Format::RGBA8_UNORM;
        }
    }

    // Loads a texture file into an NVRHI texture. DirectXTK decodes and uploads the image on the
    // native D3D12 device/queue (synchronously, at asset-load time), then the resulting resource
    // is wrapped as an NVRHI texture handle (NVRHI takes a reference and manages its lifetime).
    inline nvrhi::TextureHandle LoadTextureFileToNvrhi(RenderDevice& rd, const std::filesystem::path& path, bool dds)
    {
        DirectX::ResourceUploadBatch upload_batch(rd.GetD3D12Device());
        upload_batch.Begin();

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        HRESULT hr;
        if (dds)
            hr = DirectX::CreateDDSTextureFromFile(rd.GetD3D12Device(), upload_batch, path.c_str(), resource.GetAddressOf());
        else
            hr = DirectX::CreateWICTextureFromFile(rd.GetD3D12Device(), upload_batch, path.c_str(), resource.GetAddressOf());

        if (FAILED(hr))
            throw std::runtime_error("Failed to load texture: " + path.string());

        auto future = upload_batch.End(rd.GetD3D12Queue());
        future.wait();

        const D3D12_RESOURCE_DESC d = resource->GetDesc();

        nvrhi::TextureDesc td;
        td.width = static_cast<uint32_t>(d.Width);
        td.height = d.Height;
        td.format = DxgiFormatToNvrhi(d.Format);
        td.dimension = nvrhi::TextureDimension::Texture2D;
        td.mipLevels = d.MipLevels;
        td.isShaderResource = true;
        td.initialState = nvrhi::ResourceStates::ShaderResource;
        td.keepInitialState = true;
        td.debugName = path.filename().string();

        nvrhi::Object nativeObj(static_cast<void*>(resource.Get()));
        return rd.Device()->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, nativeObj, td);
    }
}
