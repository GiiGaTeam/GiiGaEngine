module;

#include <DirectXMath.h>
#include <stdexcept>
#include <json/json.h>
#include <json/value.h>
#include <directx/d3dx12_core.h>
#include <directxtk/SimpleMath.h>

export module Material;

import <memory>;
import <array>;
import <cmath>;
import <stdexcept>;
import <exception>;
import <filesystem>;

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
import Logger;

namespace GiiGa
{
    export using ComponentMapping = int;

    // order of material textures
    export enum class TexturesOrder
    {
        BaseColor = 1,
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
        BaseColor = 1 << 0,
        Metallic = 1 << 1,
        Specular = 1 << 2,
        Roughness = 1 << 3,
        Anisotropy = 1 << 4,
        EmissiveColor = 1 << 5,
        Opacity = 1 << 6,
        Normal = 1 << 7,
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

        struct alignas(256) MaterialData
        {
            DirectX::SimpleMath::Vector3 BaseColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            DirectX::SimpleMath::Vector3 EmissiveColorTint_ = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
            float MetallicScale_ = 1.0f;
            float SpecularScale_ = 1.0f;
            float RoughnessScale_ = 1.0f;
            float AnisotropyScale_ = 1.0f;
            float OpacityScale_ = 1.0f;

            MaterialData() = default;

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
                    auto resourceDesc = textures_[i]->GetResource()->GetDesc();

                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Format = resourceDesc.Format;
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                    srvDesc.Shader4ComponentMapping = component_mappings[i];

                    shaderResource_->texture_srvs_[i] = textures_[i]->EmplaceShaderResourceBufferView(srvDesc);
                }
            }

            SetBlendMode(blendMode);
            SetShadingModel(shadingModel);

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

        //todo: temp solution need change signature in AssetLoader Save
        Json::Value ToJson()
        {
            Json::Value result;

            result["Name"] = name;
            result["BlendMode"] = static_cast<int>(materialMask_.GetBlendMode());
            result["ShadingModel"] = static_cast<int>(materialMask_.GetShadingModel());

            result["Properties"] = data_.ToJson();

            for (int i = 0; i < textures_.size(); ++i)
            {
                Json::Value texture_js;

                if (textures_[i])
                {
                    texture_js["AssetHandle"] = textures_[i]->GetId().ToJson();
                    texture_js["Mapping"] = shaderResource_->texture_srvs_[i]->getCreationDescriptor().Shader4ComponentMapping;
                }
                else
                {
                    texture_js["AssetHandle"] = AssetHandle{}.ToJson();
                    texture_js["Mapping"] = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                }
                result["Textures"].append(texture_js);
            }

            return result;
        }

        void SaveToAbsolutePath(const std::filesystem::path& path)
        {
            el::Loggers::getLogger(LogWorld)->info("Saving Material %v in %v", this->name, path);

            std::ofstream material_file(path, std::ios::out | std::ios::trunc);

            if (!material_file.is_open())
            {
                throw std::runtime_error("Failed to open Material file for writing: " + path.string());
            }

            // Ensure ToJson() works correctly
            Json::Value json = ToJson();
            if (json.isNull())
            {
                throw std::runtime_error("Failed to convert object to JSON");
            }

            Json::StreamWriterBuilder writer_builder;
            std::string jsonString = Json::writeString(writer_builder, json);

            // Writing JSON string to the file
            material_file << jsonString;

            // Check if the file stream has any errors
            if (material_file.fail())
            {
                throw std::runtime_error{"Failed to write to file: " + path.string()};
            }

            material_file.close();
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
            auto texture_index = static_cast<int>(TexturesOrder::BaseColor) - 1;
            textures_[texture_index] = BaseColorTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
        }

        // todo: add shading model / blend mode check
        void SetMetallicTexture(std::shared_ptr<TextureAsset> MetallicTexture,
                                ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Metallic) - 1;
            textures_[texture_index] = MetallicTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = Channel;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
        }

        // todo: add shading model / blend mode check
        void SetSpecularTexture(std::shared_ptr<TextureAsset> SpecularTexture,
                                ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Specular) - 1;
            textures_[texture_index] = SpecularTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = Channel;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
        }

        // todo: add shading model / blend mode check
        void SetRoughnessTexture(std::shared_ptr<TextureAsset> RoughnessTexture,
                                 ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Roughness) - 1;
            textures_[texture_index] = RoughnessTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = Channel;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
        }

        // todo: add shading model / blend mode check
        void SetAnisotropyTexture(std::shared_ptr<TextureAsset> AnisotropyTexture,
                                  ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Anisotropy) - 1;
            textures_[texture_index] = AnisotropyTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = Channel;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
        }

        // todo: add shading model / blend mode check
        void SetEmissiveTexture(std::shared_ptr<TextureAsset> EmissiveTexture)
        {
            auto texture_index = static_cast<int>(TexturesOrder::EmissiveColor) - 1;
            textures_[texture_index] = EmissiveTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
        }

        // todo: add shading model / blend mode check
        void SetOpacityTexture(std::shared_ptr<TextureAsset> OpacityTexture,
                               ComponentMapping Channel = static_cast<D3D12_SHADER_COMPONENT_MAPPING>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING))
        {
            auto texture_index = static_cast<int>(TexturesOrder::Opacity) - 1;
            textures_[texture_index] = OpacityTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = Channel;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
        }

        // todo: add shading model / blend mode check
        void SetNormalTexture(std::shared_ptr<TextureAsset> NormalTexture)
        {
            auto texture_index = static_cast<int>(TexturesOrder::Normal) - 1;
            textures_[texture_index] = NormalTexture;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            shaderResource_->texture_srvs_[texture_index] = textures_[texture_index]->EmplaceShaderResourceBufferView(srvDesc);
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
            for (size_t i = 1; i <= MaxTextureCount; ++i)
            {
                TexturesOrder t = static_cast<TexturesOrder>(i);
                if (requiredTextures[i - 1] && !textures_[i - 1])
                {
                    TexturesOrder inorder = static_cast<TexturesOrder>(i);
                    switch (inorder)
                    {
                    case TexturesOrder::BaseColor:
                        SetBaseColorTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::BaseColor));
                        break;
                    case TexturesOrder::Metallic:
                        SetMetallicTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Metallic), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::Specular:
                        SetSpecularTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Specular), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::Roughness:
                        SetRoughnessTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Roughness), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::Anisotropy:
                        SetAnisotropyTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Anisotropy), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::EmissiveColor:
                        SetEmissiveTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::EmissiveColor));
                        break;
                    case TexturesOrder::Opacity:
                        SetOpacityTexture(rm->GetAsset<TextureAsset>(StubTexturesHandles::Opacity), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
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
            static const std::unordered_map<ShadingModel, RequiredTexturesMask> ShadingModelRequiredTextures =
            {
                {
                    ShadingModel::None, {}
                },
                {
                    ShadingModel::DefaultLit, RequiredTexturesMask(toUnderlying(
                        TexturesIndex::BaseColor
                        | TexturesIndex::Metallic
                        | TexturesIndex::Specular
                        | TexturesIndex::Roughness
                        | TexturesIndex::Anisotropy
                        | TexturesIndex::EmissiveColor
                        | TexturesIndex::Normal))
                },
                {
                    ShadingModel::Unlit, RequiredTexturesMask(toUnderlying(
                        TexturesIndex::EmissiveColor))
                }
            };

            static const std::unordered_map<BlendMode, RequiredTexturesMask> BlendModeRequiredTextures =
            {
                {BlendMode::None, {}},
                {BlendMode::Masked, RequiredTexturesMask(toUnderlying(TexturesIndex::Opacity))},
                {BlendMode::Opaque, RequiredTexturesMask()},
                {BlendMode::Translucent, RequiredTexturesMask(toUnderlying(TexturesIndex::Opacity))}
            };

            BlendMode bm = materialMask_.GetBlendMode();
            ShadingModel sm = materialMask_.GetShadingModel();
            return BlendModeRequiredTextures.at(bm) | ShadingModelRequiredTextures.at(sm);
        }
    };
}
