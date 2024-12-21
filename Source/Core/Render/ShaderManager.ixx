export module ShaderManager;

import <d3d12.h>;
import <windows.h>;
import <d3dcompiler.h>;
import <filesystem>;
import <map>;
import <vector>;

import VertexTypes;
import ObjectMask;

namespace GiiGa
{
    export class ShaderManager
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
        /*
         * Здесь мы будем компилировать и сохранять шейдера для дальнейшего использования
         */
        ShaderManager()
        {
            // Сначала создаём Shader, передавая путь до него, входную функцию в нём, а также тип с версией шейдера. Если это VertexShader, то можно (нужно) ещё указать VertexType для более легкого поиска в дальнейшем
            auto shader = std::make_unique<Shader>(L"Shaders/SimpleVertexShader.hlsl", "VSMain", "vs_5_1", VertexTypes::VertexPosition);
            
            // Можно изменить созданный shader, например задав ему макросы, инклюды или может какие-нибудь другие флаги, но сейчас это не нужно
            //shader->SetDefines(...);

            // Компилируем шейдер, передавая в него ссылку на мапу, которая в качестве ключей имеет пару <путь_до_файла, тип_шейдера>, а в качестве значения - байткод шейдера
            shader->CompileShader(&shaderMap_);

            // И так дальше с остальными шейдерами
        }

        D3D12_SHADER_BYTECODE GetShaderByName(const LPCWSTR& name)
        {
            
            std::map<std::pair<LPCWSTR, ShaderTypes>, std::unique_ptr<Shader>>::iterator it;
            for (auto const& [key, val] : shaderMap_)
            {
                if (key.first == name)
                    return val->GetShaderBC();
            }
        }
        D3D12_SHADER_BYTECODE GetShaderByNameAndType(const LPCWSTR& name, ShaderTypes shader_type)
        {
            for (auto const& [key, val] : shaderMap_)
            {
                if (key.first == name && key.second == shader_type)
                    return val->GetShaderBC();
            }
        }
        D3D12_SHADER_BYTECODE GetVertexShaderByVertexType(VertexTypes vertex_type)
        {
            if (vertex_type == VertexTypes::None) return D3D12_SHADER_BYTECODE{};
            for (auto const& [key, val]  : shaderMap_)
            {
                if (val->GetVertexType() == vertex_type)
                {
                    return val->GetShaderBC();
                }
            }
        }
        
        private:
        #pragma region ShaderClass
        class Shader
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

            void CompileShader(std::map<std::pair<LPCWSTR, ShaderTypes>, std::unique_ptr<Shader>>* map_to_store)
            {
                D3DCompileFromFile(file_name_, defines_, include_,
                    entry_point_, target_, flags1_, flags2_, code_blob_.get(), error_blob_.get());
                
                shader_bc.pShaderBytecode = (*code_blob_)->GetBufferPointer();
                shader_bc.BytecodeLength = (*code_blob_)->GetBufferSize();

                map_to_store->emplace(std::make_pair(file_name_, shader_type_), std::move(this));
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
            
            std::unique_ptr<ID3DBlob*> code_blob_ = nullptr;
            std::unique_ptr<ID3DBlob*> error_blob_ = nullptr;

            D3D12_SHADER_BYTECODE shader_bc = {};          
            
        };
        #pragma endregion
        std::map<std::pair<LPCWSTR, ShaderTypes>, std::unique_ptr<Shader>> shaderMap_;
    };
}