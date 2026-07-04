#pragma once

// NvrhiCommon.h
// Shared includes and helpers for the NVRHI-backed renderer (GiiGa::rhi).
// Phase 1 foundation: introduces the NVRHI device layer that will replace the
// hand-rolled D3D12 backend in the parent Render/ directory.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d12.h>
#include <nvrhi/utils.h>

namespace GiiGa::rhi
{
    // Throws on a failing HRESULT. Self-contained so the NVRHI foundation does not
    // depend on the legacy hand-rolled DirectXUtils.h.
    inline void ThrowIfFailed(HRESULT hr, const char* what = "rhi operation")
    {
        if (FAILED(hr))
        {
            std::ostringstream oss;
            oss << what << " failed: HRESULT 0x" << std::hex << static_cast<unsigned long>(hr);
            throw std::runtime_error(oss.str());
        }
    }

    // Maps the subset of DXGI formats used by the engine's textures to NVRHI formats.
    // Unknown formats fall back to RGBA8_UNORM.
    inline nvrhi::Format dxgiToNvrhi(DXGI_FORMAT f)
    {
        switch (f)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:      return nvrhi::Format::RGBA8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return nvrhi::Format::SRGBA8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM:      return nvrhi::Format::BGRA8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return nvrhi::Format::SBGRA8_UNORM;
        case DXGI_FORMAT_R8_UNORM:            return nvrhi::Format::R8_UNORM;
        case DXGI_FORMAT_BC1_UNORM:           return nvrhi::Format::BC1_UNORM;
        case DXGI_FORMAT_BC1_UNORM_SRGB:      return nvrhi::Format::BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:           return nvrhi::Format::BC3_UNORM;
        case DXGI_FORMAT_BC3_UNORM_SRGB:      return nvrhi::Format::BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC5_UNORM:           return nvrhi::Format::BC5_UNORM;
        case DXGI_FORMAT_BC7_UNORM:           return nvrhi::Format::BC7_UNORM;
        case DXGI_FORMAT_BC7_UNORM_SRGB:      return nvrhi::Format::BC7_UNORM_SRGB;
        default:                              return nvrhi::Format::RGBA8_UNORM;
        }
    }

    // Routes NVRHI diagnostic messages to stderr (and the VS debug output window).
    // Implements nvrhi::IMessageCallback; an instance is owned by rhi::Device and
    // handed to nvrhi::d3d12::createDevice via DeviceDesc::errorCB.
    class MessageCallback : public nvrhi::IMessageCallback
    {
    public:
        void message(nvrhi::MessageSeverity severity, const char* messageText) override
        {
            const char* tag = "nvrhi";
            switch (severity)
            {
            case nvrhi::MessageSeverity::Info: tag = "nvrhi/info"; break;
            case nvrhi::MessageSeverity::Warning: tag = "nvrhi/warn"; break;
            case nvrhi::MessageSeverity::Error: tag = "nvrhi/err"; break;
            case nvrhi::MessageSeverity::Fatal: tag = "nvrhi/fatal"; break;
            }

            std::cerr << "[" << tag << "] " << (messageText ? messageText : "<null>") << "\n";

#ifdef _DEBUG
            if (IsDebuggerPresent())
            {
                std::string dbg = std::string("[") + tag + "] " + (messageText ? messageText : "<null>") + "\n";
                OutputDebugStringA(dbg.c_str());
            }
#endif
        }
    };
}
