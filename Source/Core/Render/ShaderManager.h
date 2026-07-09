#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<windows.h>
#include<d3dcompiler.h>
#include<wrl/client.h>
#include<map>
#include<iostream>
#include<stdexcept>

#include<nvrhi/nvrhi.h>

#include<ObjectMask.h>
#include<RenderDevice.h>

namespace GiiGa
{
    const LPCWSTR VertexPositionShader = L"Shaders/VertexPositionShader.hlsl";
    const LPCWSTR VertexPNTBTShader = L"Shaders/VertexPNTBTShader.hlsl";
    const LPCWSTR VertexFullQuadShader = L"Shaders/VertexFullQuadShader.hlsl";
    const LPCWSTR OpaqueUnlitShader = L"Shaders/OpaqueUnlitPixelShader.hlsl";
    const LPCWSTR GBufferOpaqueDefaultLitShader = L"Shaders/GBufferOpaqueDefaultLitPixelShader.hlsl";
    const LPCWSTR GBufferWireframeShader = L"Shaders/GBufferWireframeShader.hlsl";
    const LPCWSTR BufferWireframeShader = L"Shaders/BufferWireframeShader.hlsl";
    const LPCWSTR GPointLight = L"Shaders/GPointLight.hlsl";
    const LPCWSTR GDirectionLight = L"Shaders/GDirectionalLight.hlsl";
    const LPCWSTR CascadeShadowShader = L"Shaders/CascadeShadowShader.hlsl";
    const LPCWSTR CascadeShadowGeomShader = L"Shaders/CascadeShadowGeomShader.hlsl";
    const LPCWSTR GammaCorrectionShader = L"Shaders/GammaCorrectionShader.hlsl";

    class ShaderManager
    {
    public:
        enum ShaderTypes
        {
            Vertex,
            Pixel,
            Hull,
            Domain,
            Geometry,
            Compute
        };

        static nvrhi::ShaderType ToNvrhiShaderType(ShaderTypes type)
        {
            switch (type)
            {
            case Vertex: return nvrhi::ShaderType::Vertex;
            case Pixel: return nvrhi::ShaderType::Pixel;
            case Hull: return nvrhi::ShaderType::Hull;
            case Domain: return nvrhi::ShaderType::Domain;
            case Geometry: return nvrhi::ShaderType::Geometry;
            case Compute: return nvrhi::ShaderType::Compute;
            }
            return nvrhi::ShaderType::None;
        }

        static ShaderManager& GetInstance()
        {
            if (instance_) return *std::static_pointer_cast<ShaderManager>(instance_);
            else return *std::static_pointer_cast<ShaderManager>(instance_ = std::shared_ptr<ShaderManager>(new ShaderManager()));
        }

        static nvrhi::ShaderHandle GetShaderByName(RenderDevice& device, const LPCWSTR& name)
        {
            for (auto const& [key, val] : GetInstance().shaderMap_)
            {
                if (key.first == name)
                    return val->GetOrCreateNvrhi(device);
            }
            throw std::runtime_error("Shader not found");
        }

        static nvrhi::ShaderHandle GetShaderByNameAndType(RenderDevice& device, const LPCWSTR& name, ShaderTypes shader_type)
        {
            for (auto const& [key, val] : GetInstance().shaderMap_)
            {
                if (key.first == name && key.second == shader_type)
                    return val->GetOrCreateNvrhi(device);
            }
            throw std::runtime_error("Shader not found");
        }

        static nvrhi::ShaderHandle GetVertexShaderByVertexType(RenderDevice& device, VertexTypes vertex_type)
        {
            if (vertex_type == VertexTypes::None) return nullptr;
            for (auto const& [key, val] : GetInstance().shaderMap_)
            {
                if (val->GetVertexType() == vertex_type)
                    return val->GetOrCreateNvrhi(device);
            }
            return nullptr;
        }

    private:
        static inline std::shared_ptr<ShaderManager> instance_;

        ShaderManager()
        {
            auto shader = std::make_shared<Shader>(VertexPositionShader, "VSMain", "vs_5_1", VertexTypes::VertexPosition);
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(VertexPNTBTShader, "VSMain", "vs_5_1", VertexTypes::VertexPNTBT);
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(VertexFullQuadShader, "VSMain", "vs_5_1", VertexTypes::VertexPNTBT);
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(OpaqueUnlitShader, "PSMain", "ps_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GBufferOpaqueDefaultLitShader, "PSMain", "ps_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GBufferWireframeShader, "PSMain", "ps_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(BufferWireframeShader, "PSMain", "ps_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GPointLight, "PSMain", "ps_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GDirectionLight, "PSMain", "ps_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GammaCorrectionShader, "PSMain", "ps_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(CascadeShadowShader, "VSMain", "vs_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(CascadeShadowGeomShader, "GSMain", "gs_5_1");
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);
        }

#pragma region ShaderClass
        class Shader : public std::enable_shared_from_this<Shader>
        {
        public:
            Shader(LPCWSTR file_name, LPCSTR entry_point, LPCSTR target, VertexTypes vertex_type = VertexTypes::None) :
                file_name_(file_name),
                entry_point_(entry_point),
                target_(target),
                vertex_type_(vertex_type)
            {
                switch (target_[0])
                {
                case 'v': shader_type_ = Vertex; return;
                case 'p': shader_type_ = Pixel; return;
                case 'h': shader_type_ = Hull; return;
                case 'd': shader_type_ = Domain; return;
                case 'g': shader_type_ = Geometry; return;
                case 'c': shader_type_ = Compute; return;
                }
            }

            void CompileShader(std::map<std::pair<LPCWSTR, ShaderTypes>, std::shared_ptr<Shader>>* map_to_store)
            {
                Microsoft::WRL::ComPtr<ID3DBlob> error_blob;

                auto res = D3DCompileFromFile(file_name_, defines_, include_,
                                              entry_point_, target_, flags1_, flags2_, &code_blob_, &error_blob);

                if (FAILED(res))
                {
                    if (error_blob)
                        std::cout << static_cast<char*>(error_blob->GetBufferPointer()) << std::endl;
                    throw std::runtime_error("Failed to compile shader");
                }

                map_to_store->emplace(std::make_pair(file_name_, shader_type_), shared_from_this());
            }

            // Lazily create (and cache) the NVRHI shader wrapping the compiled DXBC bytecode.
            nvrhi::ShaderHandle GetOrCreateNvrhi(RenderDevice& device)
            {
                if (!nvrhi_shader_)
                {
                    nvrhi::ShaderDesc desc(ToNvrhiShaderType(shader_type_));
                    desc.entryName = entry_point_;
                    nvrhi_shader_ = device.Device()->createShader(
                        desc, code_blob_->GetBufferPointer(), code_blob_->GetBufferSize());
                }
                return nvrhi_shader_;
            }

            void SetDefines(D3D_SHADER_MACRO* defines) { defines_ = defines; }
            void SetInclude(ID3DInclude* include) { include_ = include; }
            void SetFlags1(UINT flags1) { flags1_ = flags1; }
            void SetFlags2(UINT flags2) { flags2_ = flags2; }

            LPCWSTR GetFileName() { return file_name_; }
            ShaderTypes GetShaderType() const { return shader_type_; }
            VertexTypes GetVertexType() const { return vertex_type_; }

        private:
            LPCWSTR file_name_;
            LPCSTR entry_point_;
            LPCSTR target_;
            ShaderTypes shader_type_;
            VertexTypes vertex_type_;

            D3D_SHADER_MACRO* defines_ = nullptr;
            ID3DInclude* include_ = nullptr;
            UINT flags1_ = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
            UINT flags2_ = 0;

            Microsoft::WRL::ComPtr<ID3DBlob> code_blob_;
            nvrhi::ShaderHandle nvrhi_shader_;
        };
#pragma endregion

        std::map<std::pair<LPCWSTR, ShaderTypes>, std::shared_ptr<Shader>> shaderMap_;
    };
}
