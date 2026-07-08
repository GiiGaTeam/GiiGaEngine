#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<windows.h>
#include<d3dcompiler.h>
#include<nvrhi/nvrhi.h>
#include<map>
#include<memory>
#include<iostream>
#include<stdexcept>

#include<ObjectMask.h>

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

    // Compiles the fixed set of engine HLSL shaders with FXC and wraps the
    // bytecode into nvrhi::ShaderHandles. Must be Initialize()d with the nvrhi
    // device before first use (done by RenderSystem).
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

        static void Initialize(nvrhi::IDevice* device)
        {
            device_ = device;
        }

        static ShaderManager& GetInstance()
        {
            if (!device_)
                throw std::runtime_error("ShaderManager::Initialize was not called");
            if (instance_) return *instance_;
            instance_ = std::shared_ptr<ShaderManager>(new ShaderManager());
            return *instance_;
        }

        static nvrhi::ShaderHandle GetShaderByName(const LPCWSTR& name)
        {
            for (auto const& [key, val] : GetInstance().shaderMap_)
            {
                if (key.first == name)
                    return val->GetShaderHandle();
            }
            throw std::runtime_error("Shader not found: ");
        }

        static nvrhi::ShaderHandle GetShaderByNameAndType(const LPCWSTR& name, ShaderTypes shader_type)
        {
            for (auto const& [key, val] : GetInstance().shaderMap_)
            {
                if (key.first == name && key.second == shader_type)
                    return val->GetShaderHandle();
            }
            throw std::runtime_error("Shader not found: ");
        }

    private:
        static inline std::shared_ptr<ShaderManager> instance_;
        static inline nvrhi::IDevice* device_ = nullptr;

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
                case 'v':
                    shader_type_ = Vertex;
                    return;
                case 'p':
                    shader_type_ = Pixel;
                    return;
                case 'h':
                    shader_type_ = Hull;
                    return;
                case 'd':
                    shader_type_ = Domain;
                    return;
                case 'g':
                    shader_type_ = Geometry;
                    return;
                case 'c':
                    shader_type_ = Compute;
                    return;
                }
            }

            void CompileShader(std::map<std::pair<LPCWSTR, ShaderTypes>, std::shared_ptr<Shader>>* map_to_store)
            {
                ID3DBlob* code_blob_ = nullptr;
                ID3DBlob* error_blob_ = nullptr;

                auto res = D3DCompileFromFile(file_name_, defines_, include_,
                                              entry_point_, target_, flags1_, flags2_, &code_blob_, &error_blob_);

                if (FAILED(res))
                {
                    // If the shader failed to compile it should have written something to the error message.
                    if (error_blob_)
                    {
                        char* compileErrors = (char*)(error_blob_->GetBufferPointer());
                        std::cout << compileErrors << std::endl;
                    }
                    throw std::runtime_error("Shader compilation failed");
                }

                nvrhi::ShaderDesc desc;
                desc.shaderType = ToNvrhiShaderType(shader_type_);
                desc.entryName = entry_point_;

                shader_handle_ = device_->createShader(desc, code_blob_->GetBufferPointer(), code_blob_->GetBufferSize());

                code_blob_->Release();

                map_to_store->emplace(std::make_pair(file_name_, shader_type_), shared_from_this());
            }

            void SetDefines(D3D_SHADER_MACRO* defines)
            {
                defines_ = defines;
            }

            void SetInclude(ID3DInclude* include)
            {
                include_ = include;
            }

            void SetFlags1(UINT flags1)
            {
                flags1_ = flags1;
            }

            void SetFlags2(UINT flags2)
            {
                flags2_ = flags2;
            }

            nvrhi::ShaderHandle GetShaderHandle() { return shader_handle_; }

            LPCWSTR GetFileName() { return file_name_; }

            ShaderTypes GetShaderType() const { return shader_type_; }

            VertexTypes GetVertexType() const { return vertex_type_; }

        private:
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

            LPCWSTR file_name_;
            LPCSTR entry_point_;
            LPCSTR target_;
            ShaderTypes shader_type_;
            VertexTypes vertex_type_;

            D3D_SHADER_MACRO* defines_ = nullptr;
            ID3DInclude* include_ = nullptr;
            UINT flags1_ = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
            UINT flags2_ = 0;

            nvrhi::ShaderHandle shader_handle_;
        };
#pragma endregion
        std::map<std::pair<LPCWSTR, ShaderTypes>, std::shared_ptr<Shader>> shaderMap_;
    };
}
