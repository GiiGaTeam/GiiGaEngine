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
