module;

#include <DirectXMath.h>
#include <memory>
#include <directx/d3dx12_core.h>
#include <directxtk/SimpleMath.h>
#include <cmath>

export module Material;
import GPULocalResource;
import RenderDevice;

namespace GiiGa
{
    /* Порядок свойтсв материала:
         * 0 - BaseColor;
         * 1 - Metallic;
         * 2 - Specular;
         * 3 - Roughness;
         * 4 - Anisotropy;
         * 5 - Emissive Color;
         * 6 - Opacity;
         * 7 - Normal;
         * 8 - Tangent;
         * 9 - Binormal
         */
    export class Material
    {
        const int BaseColorIndex = 1;
        const int MetallicIndex = 2;
        const int SpecularIndex = 4;
        const int RoughnessIndex = 8;
        const int AnisotropyIndex = 16;
        const int EmissiveColorIndex = 32;
        const int OpacityIndex = 64;
        const int NormalIndex = 128;
        const int TangentIndex = 256;
        const int BinormalIndex = 512;
        
        public:
        friend class ObjectShaderResource;

        enum class BlendMode
        {
            Opaque = 0b1110111111,
            Masked = 0b1111111111,
            Translucent = 0b0001100001
        };
        
        enum class ShadingModel
        {
            DefaultLit = 0b1111111111,
            Unlit = 0b0000100000
        };
        struct ColorTexture;
        struct ChannelTexture;
        /*
         * SimpleTexture - Просто BufferView текстуры.
         * Родитель всех остальных текстур.
         * Предназначен для Normal, Tangent и Binormal текстур
         */
        struct SimpleTexture
        {
            std::shared_ptr<BufferView<ShaderResource>> TextureResource;
            //bool IsDirty = false;
            virtual ~SimpleTexture() = default;
        };
        /*
         * ChannelTexture - для текстур, из которых нужен лишь один или несколько каналов.
         * Предназначен для всех остальных текстур, которые не указаны в SimpleTexture и ColorTexture
         */
        struct ChannelTexture : SimpleTexture
        {
            float Scale = 1.0f;
            void GenerateChannelDesc(std::shared_ptr<GPULocalResource> GPUResource, D3D12_SHADER_COMPONENT_MAPPING NewChannel)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                    D3D12_SHADER_RESOURCE_VIEW_DESC(GPUResource->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, NewChannel);
                TextureResource = GPUResource->CreateShaderResourceBufferView(desc);
            }
        };

        /*
         * ChannelTexture - для текстур, которые определяют цвет.
         * Предназначен для BaseColor и EmissiveColor текстур
         */
        struct ColorTexture : SimpleTexture
        {
            DirectX::SimpleMath::Vector3 Tint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
        };

        struct MaterialData
        {
            DirectX::SimpleMath::Vector3 BaseColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            DirectX::SimpleMath::Vector3 EmissiveColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            float MetallicScale_ = 1.0f;
            float SpecularScale_ = 1.0f;
            float RoughnessScale_ = 1.0f;
            float AnisotropyScale_ = 1.0f;
            float OpacityScale_ = 1.0f;

            bool HasBaseColorTexture_ = false;
            bool HasMetallicTexture_ = false;
            bool HasSpecularTexture_ = false;
            bool HasRoughnessTexture_ = false;
            bool HasAnisotropyTexture_ = false;
            bool HasEmissiveColorTexture_ = false;
            bool HasOpacityTexture_ = false;
            bool HasNormalTexture_ = false;
            bool HasTangentTexture_ = false;
            bool HasBinormalTexture_ = false;
            //54 bytes

            
        } data;
        
        Material( BlendMode blendMode, ShadingModel shadingModel) : blendMode_(blendMode), shadingModel_(shadingModel)
        {
            Textures_ = std::vector<std::shared_ptr<SimpleTexture>>( 10 );
            MaterialMask = static_cast<int>(blendMode_) & static_cast<int>(shadingModel_);

            for (int i = 0; i < sizeof(Textures_) / sizeof(ChannelTexture); i++)
            {
                if (IfColorTexture(i))
                    Textures_[i] = std::static_pointer_cast<SimpleTexture>(std::make_shared<ColorTexture>());
                else if (IfChannelTexture(i))
                    Textures_[i] = std::static_pointer_cast<SimpleTexture>(std::make_shared<ChannelTexture>());
                else if (IfSimpleTexture(i))
                    Textures_[i] = std::make_shared<SimpleTexture>();
            }
        }
        
        MaterialData GetMaterialData()
        {
            MaterialData DataToExport;

            if (MaterialMask & BaseColorIndex && !Textures_[0])
            {
                DataToExport.HasBaseColorTexture_ = true;
                DataToExport.BaseColorTint_ = AsColorTexture(Textures_[0])->Tint;
            }
            if (MaterialMask & MetallicIndex && !Textures_[1])
            {
                DataToExport.HasMetallicTexture_ = true;
                DataToExport.MetallicScale_ = AsChannelTexture(Textures_[1])->Scale;
            }
            if (MaterialMask & SpecularIndex && !Textures_[2])
            {
                DataToExport.HasSpecularTexture_ = true;
                DataToExport.SpecularScale_ = AsChannelTexture(Textures_[2])->Scale;
            }
            if (MaterialMask & RoughnessIndex && !Textures_[3])
            {
                DataToExport.HasRoughnessTexture_ = true;
                DataToExport.RoughnessScale_ = AsChannelTexture(Textures_[3])->Scale;
            }
            if (MaterialMask & AnisotropyIndex && !Textures_[4])
            {
                DataToExport.HasAnisotropyTexture_ = true;
                DataToExport.AnisotropyScale_ = AsChannelTexture(Textures_[4])->Scale;
            }
            if (MaterialMask & EmissiveColorIndex && !Textures_[5])
            {
                DataToExport.HasEmissiveColorTexture_ = true;
                DataToExport.EmissiveColorTint_ = AsColorTexture(Textures_[5])->Tint;
            }
            if (MaterialMask & OpacityIndex && !Textures_[6])
            {
                DataToExport.HasOpacityTexture_ = true;
                DataToExport.OpacityScale_ = AsChannelTexture(Textures_[6])->Scale;
            }
            if (MaterialMask & NormalIndex && !Textures_[7])
            {
                DataToExport.HasNormalTexture_ = true;
            }
            if (MaterialMask & TangentIndex && !Textures_[8])
            {
                DataToExport.HasTangentTexture_ = true;
            }
            if (MaterialMask & BinormalIndex && !Textures_[9])
            {
                DataToExport.HasBinormalTexture_ = true;
            }
            
            return DataToExport;
        }

        void SetBlendMode( BlendMode NewBlendMode )
        {
            blendMode_ = NewBlendMode;
            MaterialMask = static_cast<int>(blendMode_) & static_cast<int>(shadingModel_);
        }

        void SetShadingModel( ShadingModel NewShadingModel )
        {
            shadingModel_ = NewShadingModel;
            MaterialMask = static_cast<int>(blendMode_) & static_cast<int>(shadingModel_);
        }

        std::vector<std::shared_ptr<SimpleTexture>> GetTextures()
        {
            return Textures_;
        }
        //=============================SETTING MATERIAL TEXTURES & PARAMETERS=============================

        void SetBaseColor(std::shared_ptr<GPULocalResource> BaseColorTexture,
            DirectX::SimpleMath::Vector3 BaseColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f))
        {
            Textures_[0]->TextureResource = BaseColorTexture->GetViewFirstShaderResource();
            AsColorTexture(Textures_[0])->Tint = BaseColorTint;
        }

        void SetMetallic(std::shared_ptr<GPULocalResource> MetallicTexture, float MetallicScale = 1.0f,
            D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            AsChannelTexture(Textures_[1])->GenerateChannelDesc(MetallicTexture, Channel);
            AsChannelTexture(Textures_[1])->Scale = MetallicScale;
        }
        
        void SetSpecular(std::shared_ptr<GPULocalResource> SpecularTexture, float SpecularScale = 1.0f,
            D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            AsChannelTexture(Textures_[2])->GenerateChannelDesc(SpecularTexture, Channel);
            AsChannelTexture(Textures_[2])->Scale = SpecularScale;
        }
        
        void SetRoughness(std::shared_ptr<GPULocalResource> RoughnessTexture, float RoughnessScale = 1.0f,
            D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            AsChannelTexture(Textures_[3])->GenerateChannelDesc(RoughnessTexture, Channel);
            AsChannelTexture(Textures_[3])->Scale = RoughnessScale;
        }

        void SetAnisotropy(std::shared_ptr<GPULocalResource> AnisotropyTexture, float AnisotropyScale = 1.0f,
            D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            AsChannelTexture(Textures_[4])->GenerateChannelDesc(AnisotropyTexture, Channel);
            AsChannelTexture(Textures_[4])->Scale = AnisotropyScale;
        }
        
        void SetEmissive(std::shared_ptr<GPULocalResource> EmissiveTexture,
            DirectX::SimpleMath::Vector3 EmissiveColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f))
        {
            Textures_[5]->TextureResource = EmissiveTexture->GetViewFirstShaderResource();
            AsColorTexture(Textures_[5])->Tint = EmissiveColorTint;
        }
        
        void SetOpacity(std::shared_ptr<GPULocalResource> OpacityTexture, float OpacityScale= 1.0f,
        D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            AsChannelTexture(Textures_[6])->GenerateChannelDesc(OpacityTexture, Channel);
            AsChannelTexture(Textures_[6])->Scale = OpacityScale;
        }
        
        void SetNormal(std::shared_ptr<GPULocalResource> NormalTexture)
        {
            Textures_[7]->TextureResource = NormalTexture->GetViewFirstShaderResource();
        }
        
        void SetTangent(std::shared_ptr<GPULocalResource> TangentTexture)
        {
            Textures_[8]->TextureResource = TangentTexture->GetViewFirstShaderResource();
        }
        
        void SetBinormal(std::shared_ptr<GPULocalResource> BinormalTexture)
        {
            Textures_[9]->TextureResource = BinormalTexture->GetViewFirstShaderResource();
        }
        
        private:
        //=============================TEXTURES'S TYPE CHECK=============================
        bool IfSimpleTexture(int i)
        {
            if (i <= 9 && i >= 7) return true;
            return false;
        }
        bool IfColorTexture(int i)
        {
            if (i == 0 || i == 5) return true;
            return false;
        }
        bool IfChannelTexture(int i)
        {
            if (i == 1 || i == 2 || i == 3 || i == 4 || i == 6) return true;
            return false;
        }
        //=============================TEXTURE DOWNCASTING=============================
        std::shared_ptr<ColorTexture> AsColorTexture(std::shared_ptr<SimpleTexture> texture)
        {
            return std::dynamic_pointer_cast<ColorTexture>(texture);
        }
        std::shared_ptr<ChannelTexture> AsChannelTexture(std::shared_ptr<SimpleTexture> texture)
        {
            return std::dynamic_pointer_cast<ChannelTexture>(texture);
        }
        int MaterialMask;
        
        std::vector<std::shared_ptr<SimpleTexture>> Textures_;
        
        BlendMode blendMode_;
        ShadingModel shadingModel_;

        
    };
}