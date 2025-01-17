#pragma once
#include <pybind11/conduit/wrap_include_python_h.h>
#include <stdexcept>
#include <json/json.h>
#include <json/value.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <directx/d3dx12_core.h>
#include <directxtk12/SimpleMath.h>

#include <DirectXMath.h>


#include<memory>
#include<array>
#include<cmath>
#include<stdexcept>
#include<exception>
#include<filesystem>

#include<GPULocalResource.h>
#include<RenderDevice.h>
#include<AssetBase.h>
#include<TextureAsset.h>
#include<ObjectMask.h>
#include<Misc.h>
#include<RenderContext.h>
#include<IObjectShaderResource.h>
#include<Engine.h>
#include<DefaultAssetsHandles.h>
#include<DXMathUtils.h>
#include<Logger.h>
#include<IUpdateGPUData.h>

namespace GiiGa
{
    using ComponentMapping = int;

    // order of material textures
    enum class TexturesOrder
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

    //template <typename Enum>
    //constexpr auto toUnderlying(Enum e) noexcept
    //{
    //    return static_cast<std::underlying_type_t<Enum>>(e);
    //}

    constexpr TexturesIndex operator|(TexturesIndex lhs, TexturesIndex rhs)
    {
        return static_cast<TexturesIndex>(toUnderlying(lhs) | toUnderlying(rhs));
    }

    constexpr uint8_t MaxTextureCount = 8;

    using RequiredTexturesMask = std::bitset<MaxTextureCount>;

    class MaterialShaderResource : public IObjectShaderResource
    {
        friend class Material;

    public:
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            // todo: actually we can cache this allocation by adding setters here
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result;
            result.reserve(1 + MaxTextureCount);
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

    class Material : public AssetBase, public IUpdateGPUData, public std::enable_shared_from_this<Material>
    {
        //todo: temp
        friend class ImGuiInspector;

    public:
        struct alignas(256) MaterialData
        {
            alignas(16) DirectX::SimpleMath::Vector3 BaseColorTint_ = DirectX::SimpleMath::Vector3(1);
            alignas(16) DirectX::SimpleMath::Vector3 EmissiveColorTint_ = DirectX::SimpleMath::Vector3(1);
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

        AssetType GetType()
        {
            return AssetType::Material;
        }

        friend class ObjectShaderResource;

        ~Material() override
        {
            Engine::Instance().RenderSystem()->UnregisterInUpdateGPUData(this);
        }

        Material(std::string name) : 
            AssetBase(AssetHandle{ Uuid::New(), 0}),
            shaderResource_(std::make_shared<MaterialShaderResource>()),
            name(name)
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            materialMask_.SetBlendMode(BlendMode::Opaque);
            materialMask_.SetShadingModel(ShadingModel::DefaultLit);

            CheckAndLoadRequirededTextures_();
        }

        Material(AssetHandle handle, const BlendMode& blendMode, const ShadingModel& shadingModel,
                 const MaterialData& data, const std::array<std::shared_ptr<TextureAsset>, MaxTextureCount>& textures,
                 const std::array<ComponentMapping, MaxTextureCount>& component_mappings) :
            AssetBase(handle),
            data_(data),
            shaderResource_(std::make_shared<MaterialShaderResource>()),
            textures_(textures)
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

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

        std::string name;

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

        void UpdateGPUData(RenderContext& Context) override
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

                shaderResource_->MaterialCBV_ = Context.AllocateDynamicConstantView(MaterialSpan, desc);
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

        void SetTexture(TexturesOrder texture_order, std::shared_ptr<TextureAsset> texture_asset, ComponentMapping Channel = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        {
            auto texture_index = static_cast<int>(texture_order) - 1;
            auto req_tex = GetRequiredTextures();
            if (!req_tex[texture_index])
                return;

            textures_[texture_index] = texture_asset;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = Channel;

            shaderResource_->texture_srvs_[texture_index] = textures_[static_cast<int>(texture_order) - 1]->EmplaceShaderResourceBufferView(srvDesc);
        }

        void SetTexture(TexturesOrder texture_order, AssetHandle texture_uuid, ComponentMapping Channel = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        {
            auto texture_index = static_cast<int>(texture_order) - 1;
            auto req_tex = GetRequiredTextures();
            if (!req_tex[texture_index])
                return;

            auto texture_asset = Engine::Instance().ResourceManager()->GetAsset<TextureAsset>(texture_uuid);

            textures_[texture_index] = texture_asset;

            auto resourceDesc = textures_[texture_index]->GetResource()->GetDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = resourceDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            srvDesc.Shader4ComponentMapping = Channel;

            shaderResource_->texture_srvs_[texture_index] = textures_[static_cast<int>(texture_order) - 1]->EmplaceShaderResourceBufferView(srvDesc);
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
        ObjectMask materialMask_ = ObjectMask().SetFillMode(FillMode::Solid);

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
                if (requiredTextures[i - 1] && !textures_[i - 1])
                {
                    TexturesOrder inorder = static_cast<TexturesOrder>(i);
                    switch (inorder)
                    {
                    case TexturesOrder::BaseColor:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::BaseColor));
                        break;
                    case TexturesOrder::Metallic:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Metallic), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::Specular:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Specular), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::Roughness:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Roughness), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::Anisotropy:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Anisotropy), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::EmissiveColor:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::EmissiveColor));
                        break;
                    case TexturesOrder::Opacity:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Opacity), D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
                        break;
                    case TexturesOrder::Normal:
                        SetTexture(inorder,rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Normal));
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
