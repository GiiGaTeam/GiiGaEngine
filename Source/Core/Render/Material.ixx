module;
#include <DirectXMath.h>

export module Material;

import <memory>;
import <array>;
import <directx/d3dx12_core.h>;
import <directxtk/SimpleMath.h>;
import <cmath>;

import GPULocalResource;
import RenderDevice;
import AssetBase;
import TextureAsset;
export import ObjectMask;
import Misc;
import RenderContext;
import IObjectShaderResource;

namespace GiiGa
{
    export using ComponentMapping = int;

    // order of material textures
    enum class TexturesOrder
    {
        BaseColor,
        Metallic,
        Specular,
        Roughness,
        Anisotropy,
        EmissiveColor,
        Opacity,
        Normal
    };

    constexpr static uint8_t MaxTextureCount = 8;

    export class MaterialShaderResource : public IObjectShaderResource
    {
        friend class Material;

    public:
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            // todo: actually we can cache this allocation by adding setters here
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result;
            result.reserve(MaxTextureCount);
            result.push_back(MaterialCBV_->getDescriptor().getGPUHandle());
            for (auto texture_srv : texture_srvs_)
            {
                if (texture_srv)
                    result.push_back(texture_srv->getDescriptor().getGPUHandle());
            }
            return result;
        }

    private:
        std::array<std::shared_ptr<BufferView<ShaderResource>>, MaxTextureCount> texture_srvs_;
        std::shared_ptr<BufferView<Constant>> MaterialCBV_;
    };

    export class Material : public AssetBase
    {
    public:
        friend class ObjectShaderResource;

        struct MaterialData
        {
            DirectX::SimpleMath::Vector3 BaseColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            DirectX::SimpleMath::Vector3 EmissiveColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            float MetallicScale_ = 1.0f;
            float SpecularScale_ = 1.0f;
            float RoughnessScale_ = 1.0f;
            float AnisotropyScale_ = 1.0f;
            float OpacityScale_ = 1.0f;
        } data;

        Material(RenderDevice& device, RenderContext& Context, AssetHandle handle, const BlendMode& blendMode, const ShadingModel& shadingModel,
                 const MaterialData& data, const std::array<std::shared_ptr<TextureAsset>, MaxTextureCount>& textures,
                 const std::array<ComponentMapping, MaxTextureCount>& component_mappings) :
            AssetBase(handle),
            data_(data),
            shaderResource_(std::make_shared<MaterialShaderResource>()),
            textures_(textures)
        {
            materialMask_.SetBlendMode(blendMode);
            materialMask_.SetShadingModel(shadingModel);

            // Define required texture indices based on the blend mode and shading model
            /*
            std::bitset<MaxTextureCount> requiredTextures;
            switch (shadingModel) {
            case ShadingModel::DefaultLit:
                requiredTextures.set(0); // BaseColor
                requiredTextures.set(1); // Metallic
                requiredTextures.set(3); // Roughness
                requiredTextures.set(7); // Normal
                break;
            case ShadingModel::Unlit:
                requiredTextures.set(0); // BaseColor
                break;
            }

            if (blendMode == BlendMode::Masked || blendMode == BlendMode::Translucent) {
                requiredTextures.set(6); // Opacity
            }

            // Safety check: Ensure required textures are present
            for (size_t i = 0; i < MaxTextureCount; ++i) {
                if (requiredTextures[i] && !textures_[i]) {
                    throw std::exception("Required texture is missing for the current blend mode and shading model.");
                }
            }

            // Safety check: Warn if unnecessary textures are provided
            for (size_t i = 0; i < MaxTextureCount; ++i) {
                if (!requiredTextures[i] && textures_[i]) {
                    // Optional: Log a warning or take corrective action
                    throw std::exception("Warning: Excess texture provided at index i for the current blend mode and shading model.");
                }
            }*/

            for (int i = 0; i < textures_.size(); ++i)
            {
                if (textures_[i])
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                        D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[i]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, component_mappings[i]);

                    shaderResource_->texture_srvs_[i] = textures_[i]->EmplaceShaderResourceBufferView(desc);
                }
            }

            if (isStatic_)
            {
                UINT SizeInBytes = sizeof(MaterialData);

                const auto MaterialSpan = std::span{reinterpret_cast<uint8_t*>(&data_), SizeInBytes};
                MaterialCB_ = std::make_unique<GPULocalResource>(device, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_NONE));
                MaterialCB_->UpdateContentsDeffered(Context, MaterialSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(MaterialCB_->GetResource()->GetGPUVirtualAddress(), SizeInBytes);
                shaderResource_->MaterialCBV_ = MaterialCB_->CreateConstantBufferView(desc);
            }
        }

        std::shared_ptr<Material> CreateDynamicMaterailInstance(std::shared_ptr<Material> original)
        {
            Todo();
            // some how get device here
            //auto result = std::make_shared<Material>();
            //result->isStatic_ = false;
            return nullptr;
        }

        void UpdateGPUData(RenderContext& Context)
        {
            if (!isDirty)
                return;

            isDirty = false;

            UINT SizeInBytes = sizeof(Material::MaterialData);

            const auto MaterialSpan = std::span{reinterpret_cast<uint8_t*>(&data_), SizeInBytes};

            if (isStatic_)
            {
                MaterialCB_->UpdateContentsImmediate(Context, MaterialSpan, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            }
            else
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, SizeInBytes);

                shaderResource_->MaterialCBV_ = Context.AllocateDynamicConstantView(MaterialSpan, 0, desc);
            }
        }

        MaterialData GetMaterialData()
        {
            MaterialData DataToExport;


            return DataToExport;
        }

        ObjectMask GetMaterialMask() const
        {
            return materialMask_;
        }

        void SetBlendMode(const BlendMode& new_blend_mode)
        {
            materialMask_.SetBlendMode(new_blend_mode);
        }

        void SetShadingModel(const ShadingModel& new_shading_model)
        {
            materialMask_.SetShadingModel(new_shading_model);
        }

        std::shared_ptr<IObjectShaderResource> GetShaderResource()
        {
            return shaderResource_;
        }

        //=============================SETTING MATERIAL TEXTURES & PARAMETERS=============================

        void SetBaseColorTexture(std::shared_ptr<TextureAsset> BaseColorTexture)
        {
            auto texture_index = static_cast<int>(TexturesOrder::BaseColor);
            textures_[texture_index] = BaseColorTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        void SetMetallicTexture(std::shared_ptr<TextureAsset> MetallicTexture,
                                D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            Todo();
        }

        void SetSpecularTexture(std::shared_ptr<TextureAsset> SpecularTexture,
                                D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            Todo();
        }

        void SetRoughnessTexture(std::shared_ptr<TextureAsset> RoughnessTexture,
                                 D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            Todo();
        }

        void SetAnisotropyTexture(std::shared_ptr<TextureAsset> AnisotropyTexture,
                                  D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            Todo();
        }

        void SetEmissiveTexture(std::shared_ptr<TextureAsset> EmissiveTexture)
        {
            Todo();
        }

        void SetOpacityTexture(std::shared_ptr<TextureAsset> OpacityTexture,
                               D3D12_SHADER_COMPONENT_MAPPING Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            Todo();
        }

        void SetNormalTexture(std::shared_ptr<TextureAsset> NormalTexture)
        {
            Todo();
        }

        void SetTangentTexture(std::shared_ptr<TextureAsset> TangentTexture)
        {
            Todo();
        }

        void SetBinormalTexture(std::shared_ptr<TextureAsset> BinormalTexture)
        {
            Todo();
        }

        //=============================SETTING MATERIAL PARAMETERS=============================

        void SetBaseColorTint(DirectX::SimpleMath::Vector3 BaseColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f))
        {
            data_.BaseColorTint_ = BaseColorTint;
        }

        void SetMetallicScale(float MetallicScale = 1.0f)
        {
            Todo();
        }

        void SetSpecularScale(float SpecularScale = 1.0f)
        {
            Todo();
        }

        void SetRoughnessScale(float RoughnessScale = 1.0f)
        {
            Todo();
        }

        void SetAnisotropyScale(float AnisotropyScale = 1.0f)
        {
            Todo();
        }

        void SetEmissiveTint(DirectX::SimpleMath::Vector3 EmissiveColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f))
        {
            Todo();
        }

        void SetOpacityScale(float OpacityScale = 1.0f)
        {
            Todo();
        }

    private:
        ObjectMask materialMask_ = ObjectMask();

        MaterialData data_;
        bool isStatic_ = true;
        bool isDirty = false;

        std::shared_ptr<MaterialShaderResource> shaderResource_;

        std::array<std::shared_ptr<TextureAsset>, MaxTextureCount> textures_;
        std::shared_ptr<GPULocalResource> MaterialCB_;
    };
}
