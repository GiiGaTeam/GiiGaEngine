#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>
#include<windows.h>
#include<d3dcompiler.h>
#include<map>
#include<iostream>

#include<ObjectMask.h>

namespace GiiGa
{
    const LPCWSTR VertexPositionShader = L"Shaders/VertexPositionShader.hlsl";
    const LPCWSTR VertexPNTBTShader = L"Shaders/VertexPNTBTShader.hlsl";
    const LPCWSTR VertexFullQuadShader = L"Shaders/VertexFullQuadShader.hlsl";
    const LPCWSTR GBufferOpaqueUnlitShader = L"Shaders/GBufferOpaqueUnlitPixelShader.hlsl";
    const LPCWSTR GBufferOpaqueDefaultLitShader = L"Shaders/GBufferOpaqueDefaultLitPixelShader.hlsl";
    const LPCWSTR GBufferWireframeShader = L"Shaders/GBufferWireframeShader.hlsl";
    const LPCWSTR BufferWireframeShader = L"Shaders/BufferWireframeShader.hlsl";
    const LPCWSTR GPointLight = L"Shaders/GPointLight.hlsl";
    const LPCWSTR GDirectionLight = L"Shaders/GDirectionalLight.hlsl";
    const LPCWSTR CascadeShadowShader = L"Shaders/CascadeShadowShader.hlsl";
    const LPCWSTR CascadeShadowGeomShader = L"Shaders/CascadeShadowGeomShader.hlsl";
    
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

        static ShaderManager& GetInstance()
        {
            if (instance_) return *std::static_pointer_cast<ShaderManager>(instance_);
            else return *std::static_pointer_cast<ShaderManager>(instance_ = std::shared_ptr<ShaderManager>(new ShaderManager()));
        }
        
        
        static D3D12_SHADER_BYTECODE GetShaderByName(const LPCWSTR& name)
        {
            std::map<std::pair<LPCWSTR, ShaderTypes>, std::shared_ptr<Shader>>::iterator it;
            for (auto const& [key, val] : GetInstance().shaderMap_)
            {
                if (key.first == name)
                    return val->GetShaderBC();
            }
            throw std::runtime_error("Shader not found: ");
        }
        static D3D12_SHADER_BYTECODE GetShaderByNameAndType(const LPCWSTR& name, ShaderTypes shader_type)
        {
            for (auto const& [key, val] : GetInstance().shaderMap_)
            {
                if (key.first == name && key.second == shader_type)
                    return val->GetShaderBC();
            }
        }
        static D3D12_SHADER_BYTECODE GetVertexShaderByVertexType(VertexTypes vertex_type)
        {
            if (vertex_type == VertexTypes::None) return D3D12_SHADER_BYTECODE{};
            for (auto const& [key, val]  : GetInstance().shaderMap_)
            {
                if (val->GetVertexType() == vertex_type)
                {
                    return val->GetShaderBC();
                }
            }
        }
        
        private:
        static inline std::shared_ptr<ShaderManager> instance_;
        /*
         * Здесь мы будем компилировать и сохранять шейдера для дальнейшего использования
         */
        ShaderManager()
        {
            // Сначала создаём Shader, передавая путь до него, входную функцию в нём, а также тип с версией шейдера. Если это VertexShader, то можно (нужно) ещё указать VertexType для более легкого поиска в дальнейшем
            auto shader = std::make_shared<Shader>(VertexPositionShader, "VSMain", "vs_5_1", VertexTypes::VertexPosition);
            
            // Можно изменить созданный shader, например задав ему макросы, инклюды или может какие-нибудь другие флаги, но сейчас это не нужно
            //shader->SetDefines(...);
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);

            // Компилируем шейдер, передавая в него ссылку на мапу, которая в качестве ключей имеет пару <путь_до_файла, тип_шейдера>, а в качестве значения - байткод шейдера
            shader->CompileShader(&shaderMap_);

            // И так дальше с остальными шейдерами

            shader = std::make_shared<Shader>(VertexPNTBTShader, "VSMain", "vs_5_1", VertexTypes::VertexPNTBT);
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);
            
            shader = std::make_shared<Shader>(VertexFullQuadShader, "VSMain", "vs_5_1", VertexTypes::VertexPNTBT);
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GBufferOpaqueUnlitShader, "PSMain", "ps_5_1" );
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GBufferOpaqueDefaultLitShader, "PSMain", "ps_5_1" );
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GBufferWireframeShader, "PSMain", "ps_5_1" );
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(BufferWireframeShader, "PSMain", "ps_5_1" );
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GPointLight, "PSMain", "ps_5_1" );
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(GDirectionLight, "PSMain", "ps_5_1" );
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(CascadeShadowShader, "VSMain", "vs_5_1" );
            shader->SetInclude(D3D_COMPILE_STANDARD_FILE_INCLUDE);
            shader->CompileShader(&shaderMap_);

            shader = std::make_shared<Shader>(CascadeShadowGeomShader, "GSMain", "gs_5_1" );
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
                        char* compileErrors =  (char*)(error_blob_->GetBufferPointer());
                        std::cout << compileErrors << std::endl;
                    }
                }
                
                shader_bc.pShaderBytecode = (code_blob_)->GetBufferPointer();
                shader_bc.BytecodeLength = (code_blob_)->GetBufferSize();

                map_to_store->emplace(std::make_pair(file_name_, shader_type_), shared_from_this());
            }
            
            void SetDefines(D3D_SHADER_MACRO* defines)
            {
                defines_ = defines ;
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

            D3D12_SHADER_BYTECODE GetShaderBC() {return shader_bc;}
            
            LPCWSTR GetFileName() {return file_name_;}
            
            ShaderTypes GetShaderType() const {return shader_type_;}
            
            VertexTypes GetVertexType() const {return vertex_type_;}

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

            D3D12_SHADER_BYTECODE shader_bc = {};

            
        };
        #pragma endregion
        std::map<std::pair<LPCWSTR, ShaderTypes>, std::shared_ptr<Shader>> shaderMap_;
    };
}
