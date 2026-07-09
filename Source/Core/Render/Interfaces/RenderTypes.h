#pragma once

// Shared NVRHI includes + D3D12<->NVRHI conversion helpers.
// (This file used to be an empty placeholder; it now hosts the small
//  translation helpers used across the NVRHI-based renderer.)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <dxgiformat.h>

#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d12.h>

namespace GiiGa
{
    // Convert a DXGI_FORMAT (still used by asset importers / DirectXTK) to nvrhi::Format.
    // Only the formats actually used by the engine are mapped; extend as needed.
    inline nvrhi::Format ToNvrhiFormat(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM: return nvrhi::Format::RGBA8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return nvrhi::Format::SRGBA8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM: return nvrhi::Format::BGRA8_UNORM;
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return nvrhi::Format::RGBA16_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return nvrhi::Format::RGBA32_FLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT: return nvrhi::Format::RGB32_FLOAT;
        case DXGI_FORMAT_R32G32_FLOAT: return nvrhi::Format::RG32_FLOAT;
        case DXGI_FORMAT_R32_FLOAT: return nvrhi::Format::R32_FLOAT;
        case DXGI_FORMAT_R16_UINT: return nvrhi::Format::R16_UINT;
        case DXGI_FORMAT_R32_UINT: return nvrhi::Format::R32_UINT;
        case DXGI_FORMAT_R16G16B16A16_UINT: return nvrhi::Format::RGBA16_UINT;
        case DXGI_FORMAT_R32G32B32A32_UINT: return nvrhi::Format::RGBA32_UINT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return nvrhi::Format::D24S8;
        case DXGI_FORMAT_D32_FLOAT: return nvrhi::Format::D32;
        case DXGI_FORMAT_BC1_UNORM: return nvrhi::Format::BC1_UNORM;
        case DXGI_FORMAT_BC3_UNORM: return nvrhi::Format::BC3_UNORM;
        case DXGI_FORMAT_BC5_UNORM: return nvrhi::Format::BC5_UNORM;
        case DXGI_FORMAT_BC7_UNORM: return nvrhi::Format::BC7_UNORM;
        default: return nvrhi::Format::UNKNOWN;
        }
    }
}
