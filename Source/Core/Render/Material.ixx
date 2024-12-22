module;
#include <DirectXMath.h>
#include <easylogging++.h>
#include <stdexcept>
#include <json/value.h>

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
import Engine;
import StubTexturesHandles;
import MathUtils;

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

    enum class TexturesIndex
    {
        BaseColor = 1 << 1,
        Metallic = 1 << 2,
        Specular = 1 << 3,
        Roughness = 1 << 4,
        Anisotropy = 1 << 5,
        EmissiveColor = 1 << 6,
        Opacity = 1 << 7,
        Normal = 1 << 8,
    };

    template <typename Enum>
    constexpr auto toUnderlying(Enum e) noexcept
    {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }

    export constexpr TexturesIndex operator|(TexturesIndex lhs, TexturesIndex rhs)
    {
        return static_cast<TexturesIndex>(toUnderlying(lhs) | toUnderlying(rhs));
    }

    export constexpr uint8_t MaxTextureCount = 8;

    using RequiredTexturesMask = std::bitset<MaxTextureCount>;

    const std::unordered_map<ShadingModel, RequiredTexturesMask> ShadingModelRequiredTextures =
    {
        {ShadingModel::DefaultLit, RequiredTexturesMask(toUnderlying(TexturesIndex::Metallic | TexturesIndex::Specular))}
    };

    const std::unordered_map<BlendMode, RequiredTexturesMask> BlendModeRequiredTextures =
    {
        {BlendMode::Masked, RequiredTexturesMask(toUnderlying(TexturesIndex::Metallic | TexturesIndex::Specular))}
    };

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
        AssetType GetType()
        {
            return AssetType::Material;
        }

        friend class ObjectShaderResource;

        std::string name;

        struct MaterialData
        {
            DirectX::SimpleMath::Vector3 BaseColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            DirectX::SimpleMath::Vector3 EmissiveColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            float MetallicScale_ = 1.0f;
            float SpecularScale_ = 1.0f;
            float RoughnessScale_ = 1.0f;
            float AnisotropyScale_ = 1.0f;
            float OpacityScale_ = 1.0f;

            Json::Value ToJson()
            {
                Json::Value result;

                result["BaseColorTint_"] = Vector3ToJson(BaseColorTint_);
                result["EmissiveColorTint_"] = Vector3ToJson(EmissiveColorTint_);

                result["MetallicScale_"] = MetallicScale_;
                result["SpecularScale_"] = MetallicScale_;
                result["RoughnessScale_"] = MetallicScale_;
                result["AnisotropyScale_"] = MetallicScale_;
                result["OpacityScale_"] = MetallicScale_;

                return result;
            }

            MaterialData(const Json::Value& json)
            {
                BaseColorTint_ = Vector3FromJson(json["BaseColorTint_"]);
                EmissiveColorTint_ = Vector3FromJson(json["EmissiveColorTint_"]);
                MetallicScale_ = json["MetallicScale_"].asFloat();
                SpecularScale_ = json["SpecularScale_"].asFloat();
                RoughnessScale_ = json["RoughnessScale_"].asFloat();
                AnisotropyScale_ = json["AnisotropyScale_"].asFloat();
                OpacityScale_ = json["OpacityScale_"].asFloat();
            }
        };

        Material(AssetHandle handle, const BlendMode& blendMode, const ShadingModel& shadingModel,
                 const MaterialData& data, const std::array<std::shared_ptr<TextureAsset>, MaxTextureCount>& textures,
                 const std::array<ComponentMapping, MaxTextureCount>& component_mappings) :
            AssetBase(handle),
            data_(data),
            shaderResource_(std::make_shared<MaterialShaderResource>()),
            textures_(textures)
        {
            for (int i = 0; i < textures_.size(); ++i)
            {
                if (textures_[i])
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                        D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[i]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, component_mappings[i]);

                    shaderResource_->texture_srvs_[i] = textures_[i]->EmplaceShaderResourceBufferView(desc);
                }
            }

            materialMask_.SetBlendMode(blendMode);
            materialMask_.SetShadingModel(shadingModel);

            auto& device = Engine::Instance().RenderSystem()->GetRenderDevice();
            auto& Context = Engine::Instance().RenderSystem()->GetRenderContext();

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

        ObjectMask GetMaterialMask() const
        {
            return materialMask_;
        }

        void SetBlendMode(const BlendMode& new_blend_mode)
        {
            materialMask_.SetBlendMode(new_blend_mode);
            CheckAndLoadRequirededTextures_();
        }

        void SetShadingModel(const ShadingModel& new_shading_model)
        {
            materialMask_.SetShadingModel(new_shading_model);
            CheckAndLoadRequirededTextures_();
        }

        std::shared_ptr<IObjectShaderResource> GetShaderResource()
        {
            return shaderResource_;
        }

        //=============================SETTING MATERIAL TEXTURES & PARAMETERS=============================

        // todo: add shading model / blend mode check
        void SetBaseColorTexture(std::shared_ptr<TextureAsset> BaseColorTexture)
        {
            auto texture_index = static_cast<int>(TexturesOrder::BaseColor);
            textures_[texture_index] = BaseColorTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        // todo: add shading model / blend mode check
        void SetMetallicTexture(std::shared_ptr<TextureAsset> MetallicTexture,
                                ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic);
            textures_[texture_index] = MetallicTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, Channel);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        // todo: add shading model / blend mode check
        void SetSpecularTexture(std::shared_ptr<TextureAsset> SpecularTexture,
                                ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic);
            textures_[texture_index] = SpecularTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, Channel);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        // todo: add shading model / blend mode check
        void SetRoughnessTexture(std::shared_ptr<TextureAsset> RoughnessTexture,
                                 ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic);
            textures_[texture_index] = RoughnessTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, Channel);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        // todo: add shading model / blend mode check
        void SetAnisotropyTexture(std::shared_ptr<TextureAsset> AnisotropyTexture,
                                  ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic);
            textures_[texture_index] = AnisotropyTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, Channel);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        // todo: add shading model / blend mode check
        void SetEmissiveTexture(std::shared_ptr<TextureAsset> EmissiveTexture)
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic);
            textures_[texture_index] = EmissiveTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        // todo: add shading model / blend mode check
        void SetOpacityTexture(std::shared_ptr<TextureAsset> OpacityTexture,
                               ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic);
            textures_[texture_index] = OpacityTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, Channel);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        // todo: add shading model / blend mode check
        void SetNormalTexture(std::shared_ptr<TextureAsset> NormalTexture)
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic);
            textures_[texture_index] = NormalTexture;

            D3D12_SHADER_RESOURCE_VIEW_DESC desc =
                D3D12_SHADER_RESOURCE_VIEW_DESC(textures_[texture_index]->GetResource()->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(desc);
        }

        //=============================SETTING MATERIAL PARAMETERS=============================

        void SetBaseColorTint(DirectX::SimpleMath::Vector3 BaseColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f))
        {
            data_.BaseColorTint_ = BaseColorTint;
            isDirty = true;
        }

        void SetMetallicScale(float MetallicScale = 1.0f)
        {
            data_.MetallicScale_ = MetallicScale;
            isDirty = true;
        }

        void SetSpecularScale(float SpecularScale = 1.0f)
        {
            data_.SpecularScale_ = SpecularScale;
            isDirty = true;
        }

        void SetRoughnessScale(float RoughnessScale = 1.0f)
        {
            data_.RoughnessScale_ = RoughnessScale;
            isDirty = true;
        }

        void SetAnisotropyScale(float AnisotropyScale = 1.0f)
        {
            data_.AnisotropyScale_ = AnisotropyScale;
            isDirty = true;
        }

        void SetEmissiveTint(DirectX::SimpleMath::Vector3 EmissiveColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f))
        {
            data_.EmissiveColorTint_ = EmissiveColorTint;
            isDirty = true;
        }

        void SetOpacityScale(float OpacityScale = 1.0f)
        {
            data_.OpacityScale_ = OpacityScale;
            isDirty = true;
        }

    private:
        ObjectMask materialMask_ = ObjectMask();

        MaterialData data_;
        bool isStatic_ = true;
        bool isDirty = false;

        std::shared_ptr<MaterialShaderResource> shaderResource_;

        std::array<std::shared_ptr<TextureAsset>, MaxTextureCount> textures_;
        std::shared_ptr<GPULocalResource> MaterialCB_;

        void CheckAndLoadRequirededTextures_()
        {
            // Define required texture indices based on the blend mode and shading model
            RequiredTexturesMask requiredTextures = GetRequiredTextures();

            auto rm = Engine::Instance().ResourceManager();

            // Safety check: Ensure required textures are present
            for (size_t i = 0; i < MaxTextureCount; ++i)
            {
                if (requiredTextures[i] && !textures_[i])
                {
                    TexturesOrder inorder = static_cast<TexturesOrder>(i);
                    switch (inorder)
                    {
                    case TexturesOrder::BaseColor:
                        SetBaseColorTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::BaseColor));
                        break;
                    case TexturesOrder::Metallic:
                        SetMetallicTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Metallic), D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
                        break;
                    case TexturesOrder::Specular:
                        SetSpecularTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Specular), D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
                        break;
                    case TexturesOrder::Roughness:
                        SetRoughnessTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Roughness), D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
                        break;
                    case TexturesOrder::Anisotropy:
                        SetAnisotropyTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Anisotropy), D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
                        break;
                    case TexturesOrder::EmissiveColor:
                        SetEmissiveTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::EmissiveColor));
                        break;
                    case TexturesOrder::Opacity:
                        SetOpacityTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Opacity), D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
                        break;
                    case TexturesOrder::Normal:
                        SetNormalTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Normal));
                        break;
                    }
                }
            }

            // unload non req textures
            for (size_t i = 0; i < MaxTextureCount; ++i)
            {
                if (!requiredTextures[i] && textures_[i])
                {
                    textures_[i].reset();
                    shaderResource_->texture_srvs_[i].reset();
                }
            }
        }

        RequiredTexturesMask GetRequiredTextures() const
        {
            return BlendModeRequiredTextures.at(materialMask_.GetBlendMode()) | ShadingModelRequiredTextures.at(materialMask_.GetShadingModel());
        }
    };
}
