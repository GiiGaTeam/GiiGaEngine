#pragma once

// Material.h — renderable material asset.
// NVRHI cutover: the material owns a static constant buffer (nvrhi::BufferHandle) for its
// MaterialData and exposes its textures/CB via IObjectShaderResource::GetMaterialBindings,
// which the render pass uses to build a per-material nvrhi::BindingSet. The old
// MaterialShaderResource (D3D12 descriptor views) is gone.

#include <stdexcept>
#include <json/json.h>
#include <json/value.h>

#include <directx/d3dx12_core.h>
#include <directxtk12/SimpleMath.h>
#include <DirectXMath.h>

#include <memory>
#include <array>
#include <exception>
#include <filesystem>

#include <Core/Render/Nvrhi/NvrhiResources.h>
#include <AssetBase.h>
#include <ConcreteAsset/TextureAsset.h>
#include <ObjectMask.h>
#include <Misc.h>
#include <IObjectShaderResource.h>
#include <Engine.h>
#include <DefaultAssetsHandles.h>
#include <DXMathUtils.h>
#include <Logger.h>
#include <IUpdateGPUData.h>

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

    constexpr TexturesIndex operator|(TexturesIndex lhs, TexturesIndex rhs)
    {
        return static_cast<TexturesIndex>(toUnderlying(lhs) | toUnderlying(rhs));
    }

    constexpr uint8_t MaxTextureCount = 8;
    static_assert(MaxMaterialTextures == MaxTextureCount, "MaxTextureCount must match IObjectShaderResource");

    using RequiredTexturesMask = std::bitset<MaxTextureCount>;

    class Material : public AssetBase,
                     public IUpdateGPUData,
                     public IObjectShaderResource,
                     public std::enable_shared_from_this<Material>
    {
        friend class ImGuiInspector;
        friend class ImGuiAssetEditor;

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
                result["SpecularScale_"] = SpecularScale_;
                result["RoughnessScale_"] = RoughnessScale_;
                result["AnisotropyScale_"] = AnisotropyScale_;
                result["OpacityScale_"] = OpacityScale_;
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

        AssetType GetType() { return AssetType::Material; }

        ~Material() override
        {
            if (auto rs = Engine::Instance().RenderSystem())
            {
                rs->UnregisterInUpdateGPUData(this);
            }
        }

        Material(std::string name)
            : AssetBase(AssetHandle{Uuid::New(), 0})
            , name(std::move(name))
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            materialMask_.SetBlendMode(BlendMode::Opaque);
            materialMask_.SetShadingModel(ShadingModel::DefaultLit);

            CreateConstantBuffer_();
            CheckAndLoadRequirededTextures_();
            RefreshBindings_();
        }

        Material(AssetHandle handle, const BlendMode& blendMode, const ShadingModel& shadingModel,
                 const MaterialData& data, const std::array<std::shared_ptr<TextureAsset>, MaxTextureCount>& textures,
                 const std::array<ComponentMapping, MaxTextureCount>& component_mappings)
            : AssetBase(handle)
            , data_(data)
            , textures_(textures)
            , component_mappings_(component_mappings)
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            SetBlendMode(blendMode);
            SetShadingModel(shadingModel);

            CreateConstantBuffer_();
            RefreshBindings_();
        }

        std::string name;

        Json::Value ToJson()
        {
            Json::Value result;
            result["Name"] = name;
            result["BlendMode"] = static_cast<int>(materialMask_.GetBlendMode());
            result["ShadingModel"] = static_cast<int>(materialMask_.GetShadingModel());
            result["Properties"] = data_.ToJson();

            for (int i = 0; i < MaxTextureCount; ++i)
            {
                Json::Value texture_js;
                if (textures_[i])
                {
                    texture_js["AssetHandle"] = textures_[i]->GetId().ToJson();
                    texture_js["Mapping"] = component_mappings_[i];
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
                throw std::runtime_error("Failed to open Material file for writing: " + path.string());

            Json::Value json = ToJson();
            if (json.isNull())
                throw std::runtime_error("Failed to convert object to JSON");

            Json::StreamWriterBuilder writer_builder;
            material_file << Json::writeString(writer_builder, json);

            if (material_file.fail())
                throw std::runtime_error{"Failed to write to file: " + path.string()};

            material_file.close();
        }

        std::shared_ptr<Material> CreateDynamicMaterailInstance(std::shared_ptr<Material> /*original*/)
        {
            Todo();
            return nullptr;
        }

        // --- IUpdateGPUData: push MaterialData to the constant buffer when dirty ---
        void UpdateGPUData(nvrhi::IDevice* /*device*/, nvrhi::ICommandList* cmd) override
        {
            if (!isDirty_ || !cb_)
                return;
            isDirty_ = false;
            cmd->writeBuffer(cb_, &data_, sizeof(MaterialData));
        }

        // --- IObjectShaderResource: expose CB + textures so the pass can build a binding set ---
        const MaterialBindings& GetMaterialBindings() override { return bindings_; }

        ObjectMask GetMaterialMask() const { return materialMask_; }

        void SetBlendMode(const BlendMode& new_blend_mode)
        {
            materialMask_.SetBlendMode(new_blend_mode);
            CheckAndLoadRequirededTextures_();
            RefreshBindings_();
        }

        void SetShadingModel(const ShadingModel& new_shading_model)
        {
            materialMask_.SetShadingModel(new_shading_model);
            CheckAndLoadRequirededTextures_();
            RefreshBindings_();
        }

        std::shared_ptr<IObjectShaderResource> GetShaderResource()
        {
            return std::static_pointer_cast<IObjectShaderResource>(shared_from_this());
        }

        //=============================SETTING MATERIAL TEXTURES & PARAMETERS=============================

        void SetTexture(TexturesOrder texture_order, std::shared_ptr<TextureAsset> texture_asset,
                        ComponentMapping Channel = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        {
            auto texture_index = static_cast<int>(texture_order) - 1;
            if (!GetRequiredTextures()[texture_index])
                return;

            textures_[texture_index] = texture_asset;
            component_mappings_[texture_index] = Channel;
            RefreshBindings_();
        }

        void SetTexture(TexturesOrder texture_order, AssetHandle texture_uuid,
                        ComponentMapping Channel = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        {
            auto texture_index = static_cast<int>(texture_order) - 1;
            if (!GetRequiredTextures()[texture_index])
                return;

            textures_[texture_index] = Engine::Instance().ResourceManager()->GetAsset<TextureAsset>(texture_uuid);
            component_mappings_[texture_index] = Channel;
            RefreshBindings_();
        }

        //=============================SETTING MATERIAL PARAMETERS=============================

        void SetBaseColorTint(DirectX::SimpleMath::Vector3 BaseColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f)) { data_.BaseColorTint_ = BaseColorTint; isDirty_ = true; }
        void SetMetallicScale(float MetallicScale = 1.0f)    { data_.MetallicScale_ = MetallicScale; isDirty_ = true; }
        void SetSpecularScale(float SpecularScale = 1.0f)    { data_.SpecularScale_ = SpecularScale; isDirty_ = true; }
        void SetRoughnessScale(float RoughnessScale = 1.0f)  { data_.RoughnessScale_ = RoughnessScale; isDirty_ = true; }
        void SetAnisotropyScale(float AnisotropyScale = 1.0f){ data_.AnisotropyScale_ = AnisotropyScale; isDirty_ = true; }
        void SetEmissiveTint(DirectX::SimpleMath::Vector3 EmissiveColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f)) { data_.EmissiveColorTint_ = EmissiveColorTint; isDirty_ = true; }
        void SetOpacityScale(float OpacityScale = 1.0f)      { data_.OpacityScale_ = OpacityScale; isDirty_ = true; }

        // inspector access
        const MaterialData& GetData() const { return data_; }
        const std::array<std::shared_ptr<TextureAsset>, MaxTextureCount>& GetTextures() const { return textures_; }

    private:
        ObjectMask materialMask_ = ObjectMask().SetFillMode(FillMode::Solid);

        MaterialData data_;
        bool isDirty_ = true;

        std::array<std::shared_ptr<TextureAsset>, MaxTextureCount> textures_;
        std::array<ComponentMapping, MaxTextureCount> component_mappings_{};

        nvrhi::BufferHandle cb_;
        MaterialBindings bindings_;

        void CreateConstantBuffer_()
        {
            if (auto rs = Engine::Instance().RenderSystem())
            {
                if (auto dev = rs->GetNvrhiDevice())
                {
                    cb_ = rhi::createConstantBuffer(dev, sizeof(MaterialData), "material cb");
                    bindings_.constantBuffer = cb_.Get();
                }
            }
        }

        // Sync bindings_.textures from textures_ (nullptr where absent).
        void RefreshBindings_()
        {
            for (size_t i = 0; i < MaxTextureCount; ++i)
            {
                bindings_.textures[i] = textures_[i] ? textures_[i]->Get() : nullptr;
            }
            bindings_.opaque = (materialMask_.GetBlendMode() == BlendMode::Opaque);
        }

        void CheckAndLoadRequirededTextures_()
        {
            RequiredTexturesMask requiredTextures = GetRequiredTextures();
            auto rm = Engine::Instance().ResourceManager();

            for (size_t i = 1; i <= MaxTextureCount; ++i)
            {
                if (requiredTextures[i - 1] && !textures_[i - 1])
                {
                    TexturesOrder inorder = static_cast<TexturesOrder>(i);
                    switch (inorder)
                    {
                    case TexturesOrder::BaseColor:     SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::BaseColor)); break;
                    case TexturesOrder::Metallic:      SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Metallic)); break;
                    case TexturesOrder::Specular:      SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Specular)); break;
                    case TexturesOrder::Roughness:     SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Roughness)); break;
                    case TexturesOrder::Anisotropy:    SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Anisotropy)); break;
                    case TexturesOrder::EmissiveColor: SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::EmissiveColor)); break;
                    case TexturesOrder::Opacity:       SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Opacity)); break;
                    case TexturesOrder::Normal:        SetTexture(inorder, rm->GetAsset<TextureAsset>(DefaultAssetsHandles::Normal)); break;
                    }
                }
            }
        }

        RequiredTexturesMask GetRequiredTextures() const
        {
            static const std::unordered_map<ShadingModel, RequiredTexturesMask> ShadingModelRequiredTextures =
            {
                { ShadingModel::None, {} },
                { ShadingModel::DefaultLit, RequiredTexturesMask(toUnderlying(
                      TexturesIndex::BaseColor | TexturesIndex::Metallic | TexturesIndex::Specular |
                      TexturesIndex::Roughness | TexturesIndex::Anisotropy | TexturesIndex::EmissiveColor |
                      TexturesIndex::Normal)) },
                { ShadingModel::Unlit, RequiredTexturesMask(toUnderlying(TexturesIndex::EmissiveColor)) }
            };

            static const std::unordered_map<BlendMode, RequiredTexturesMask> BlendModeRequiredTextures =
            {
                { BlendMode::None, {} },
                { BlendMode::Masked, RequiredTexturesMask(toUnderlying(TexturesIndex::Opacity)) },
                { BlendMode::Opaque, RequiredTexturesMask() },
                { BlendMode::Translucent, RequiredTexturesMask(toUnderlying(TexturesIndex::Opacity)) }
            };

            return BlendModeRequiredTextures.at(materialMask_.GetBlendMode()) |
                   ShadingModelRequiredTextures.at(materialMask_.GetShadingModel());
        }
    };
}
