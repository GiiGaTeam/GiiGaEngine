#pragma once

// NvrhiShader.h
//
// NVRHI's createShader() takes already-compiled bytecode — HLSL compilation is the
// application's job. This cache compiles the engine's HLSL files with D3DCompileFromFile
// and hands the bytecode to nvrhi::IDevice::createShader, caching the resulting
// nvrhi::ShaderHandle by (file, entry point). It replaces the legacy ShaderManager.h,
// which produced D3D12_SHADER_BYTECODE structs for the hand-rolled PSO builder.

#include "NvrhiCommon.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

namespace GiiGa::rhi
{
    class ShaderCache
    {
    public:
        ShaderCache() = default;

        ShaderCache(const ShaderCache&) = delete;
        ShaderCache& operator=(const ShaderCache&) = delete;

        // Compile (or return cached) `file`/`entry` as an NVRHI shader of `type`.
        nvrhi::ShaderHandle getOrCreate(nvrhi::IDevice* device, const std::wstring& file,
                                        const std::string& entry, nvrhi::ShaderType type)
        {
            const auto key = std::make_pair(file, entry);
            auto it = cache_.find(key);
            if (it != cache_.end()) return it->second;

            Microsoft::WRL::ComPtr<ID3DBlob> code;
            Microsoft::WRL::ComPtr<ID3DBlob> errors;

            UINT flags = 0;
#ifdef _DEBUG
            flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            HRESULT hr = D3DCompileFromFile(file.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                            entry.c_str(), targetFor(type), flags, 0, &code, &errors);

            if (FAILED(hr))
            {
                if (errors)
                {
                    std::cerr << "[shader] failed to compile " << narrow(file)
                              << " / " << entry << ":\n"
                              << static_cast<const char*>(errors->GetBufferPointer()) << "\n";
                }
                ThrowIfFailed(hr, "D3DCompileFromFile");
            }

            nvrhi::ShaderDesc desc(type);
            desc.entryName = entry;
            desc.debugName = narrow(file) + "/" + entry;

            nvrhi::ShaderHandle shader = device->createShader(desc, code->GetBufferPointer(), code->GetBufferSize());
            if (!shader) throw std::runtime_error("nvrhi::IDevice::createShader returned null");

            cache_.emplace(key, shader);
            return shader;
        }

    private:
        std::map<std::pair<std::wstring, std::string>, nvrhi::ShaderHandle> cache_;

        static const char* targetFor(nvrhi::ShaderType type)
        {
            switch (type)
            {
            case nvrhi::ShaderType::Vertex:   return "vs_5_1";
            case nvrhi::ShaderType::Pixel:    return "ps_5_1";
            case nvrhi::ShaderType::Geometry: return "gs_5_1";
            case nvrhi::ShaderType::Hull:     return "hs_5_1";
            case nvrhi::ShaderType::Domain:   return "ds_5_1";
            case nvrhi::ShaderType::Compute:  return "cs_5_1";
            default: throw std::runtime_error("unsupported nvrhi::ShaderType");
            }
        }

        static std::string narrow(const std::wstring& w)
        {
            std::string s(w.size(), '\0');
            std::transform(w.begin(), w.end(), s.begin(), [](wchar_t c) { return static_cast<char>(c); });
            return s;
        }
    };
}
