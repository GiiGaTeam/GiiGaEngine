#pragma once
#include <stdexcept>
#include <json/json.h>
#include <json/value.h>

#include <directxtk12/SimpleMath.h>
#include <DirectXMath.h>

#include<memory>
#include<array>
#include<exception>
#include<filesystem>

#include<nvrhi/nvrhi.h>

#include<RenderDevice.h>
#include<RenderContext.h>
#include<AssetBase.h>
#include<ConcreteAsset/TextureAsset.h>
#include<ObjectMask.h>
#include<Misc.h>
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

    constexpr TexturesIndex operator|(TexturesIndex lhs, TexturesIndex rhs)
    {
        return static_cast<TexturesIndex>(toUnderlying(lhs) | toUnderlying(rhs));
    }

    constexpr uint8_t MaxTextureCount = 8;

    using RequiredTexturesMask = std::bitset<MaxTextureCount>;

    // Per-material bindings: constant buffer at b2 + 8 textures at t0..t7.
    // All 8 texture slots are always populated (with engine defaults) so the binding set
    // always satisfies the pipeline's binding layout.
    class MaterialShaderResource : public IObjectShaderResource
    {
        friend class Material;

    public:
        void AddBindings(nvrhi::BindingSetDesc& set) override
        {
            set.addItem(nvrhi::BindingSetItem::ConstantBuffer(2, MaterialCB_));
            for (uint32_t i = 0; i < MaxTextureCount; ++i)
                set.addItem(nvrhi::BindingSetItem::Texture_SRV(i, textures_[i]));
        }

    private:
        std::array<nvrhi::ITexture*, MaxTextureCount> textures_{};
        nvrhi::IBuffer* MaterialCB_ = nullptr;
    };

    class Material : public AssetBase, public IUpdateGPUData, public std::enable_shared_from_this<Material>
    {
        //todo: temp
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
            if (auto rs = Engine::Instance().RenderSystem())
            {
                rs->UnregisterInUpdateGPUData(this);
            }
        }

        Material(std::string name) :
            AssetBase(AssetHandle{Uuid::New(), 0}),
            shaderResource_(std::make_shared<MaterialShaderResource>()),
            name(name)
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            materialMask_.SetBlendMode(BlendMode::Opaque);
            materialMask_.SetShadingModel(ShadingModel::DefaultLit);

            CreateAndUploadCB();
            CheckAndLoadRequirededTextures_();
        }

        Material(AssetHandle handle, const BlendMode& blendMode, const ShadingModel& shadingModel,
                 const MaterialData& data, const std::array<std::shared_ptr<TextureAsset>, MaxTextureCount>& textures,
                 const std::array<ComponentMapping, MaxTextureCount>& component_mappings) :
            AssetBase(handle),
            data_(data),
            shaderResource_(std::make_shared<MaterialShaderResource>()),
            textures_(textures),
            component_mappings_(component_mappings)
        {
            Engine::Instance().RenderSystem()->RegisterInUpdateGPUData(this);

            for (int i = 0; i < textures_.size(); ++i)
                if (textures_[i])
                    shaderResource_->textures_[i] = textures_[i]->GetTexture();

            SetBlendMode(blendMode);
            SetShadingModel(shadingModel);

            CreateAndUploadCB();
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
                    texture_js["Mapping"] = component_mappings_[i];
                }
                else
                {
                    texture_js["AssetHandle"] = AssetHandle{}.ToJson();
                    texture_js["Mapping"] = static_cast<int>(D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING);
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
            std::string jsonString = Json::writeString(writer_builder, json);

            material_file << jsonString;

            if (material_file.fail())
                throw std::runtime_error{"Failed to write to file: " + path.string()};

            material_file.close();
        }

        std::shared_ptr<Material> CreateDynamicMaterailInstance(std::shared_ptr<Material> original)
        {
            Todo();
            return nullptr;
        }

        void UpdateGPUData(RenderContext& Context) override
        {
            if (!isDirty)
                return;

            isDirty = false;

            if (MaterialCB_)
                Context.GetCommandList()->writeBuffer(MaterialCB_, &data_, sizeof(MaterialData));
        }

        ObjectMask GetMaterialMask() const { return materialMask_; }

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

        std::shared_ptr<IObjectShaderResource> GetShaderResource() { return shaderResource_; }

        //=============================SETTING MATERIAL TEXTURES & PARAMETERS=============================

        void SetTexture(TexturesOrder texture_order, std::shared_ptr<TextureAsset> texture_asset, ComponentMapping Channel = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        {
            auto texture_index = static_cast<int>(texture_order) - 1;
            auto req_tex = GetRequiredTextures();
            if (!req_tex[texture_index])
                return;

            textures_[texture_index] = texture_asset;
            component_mappings_[texture_index] = Channel;
            shaderResource_->textures_[texture_index] = texture_asset->GetTexture();
        }

        void SetTexture(TexturesOrder texture_order, AssetHandle texture_uuid, ComponentMapping Channel = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING)
        {
            auto texture_asset = Engine::Instance().ResourceManager()->GetAsset<TextureAsset>(texture_uuid);
            SetTexture(texture_order, texture_asset, Channel);
        }

        //=============================SETTING MATERIAL PARAMETERS=============================

        void SetBaseColorTint(DirectX::SimpleMath::Vector3 BaseColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f)) { data_.BaseColorTint_ = BaseColorTint; isDirty = true; }
        void SetMetallicScale(float MetallicScale = 1.0f) { data_.MetallicScale_ = MetallicScale; isDirty = true; }
        void SetSpecularScale(float SpecularScale = 1.0f) { data_.SpecularScale_ = SpecularScale; isDirty = true; }
        void SetRoughnessScale(float RoughnessScale = 1.0f) { data_.RoughnessScale_ = RoughnessScale; isDirty = true; }
        void SetAnisotropyScale(float AnisotropyScale = 1.0f) { data_.AnisotropyScale_ = AnisotropyScale; isDirty = true; }
        void SetEmissiveTint(DirectX::SimpleMath::Vector3 EmissiveColorTint = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f)) { data_.EmissiveColorTint_ = EmissiveColorTint; isDirty = true; }
        void SetOpacityScale(float OpacityScale = 1.0f) { data_.OpacityScale_ = OpacityScale; isDirty = true; }

    private:
        ObjectMask materialMask_ = ObjectMask().SetFillMode(FillMode::Solid);

        MaterialData data_;
        bool isStatic_ = true;
        bool isDirty = false;

        std::shared_ptr<MaterialShaderResource> shaderResource_;

        std::array<std::shared_ptr<TextureAsset>, MaxTextureCount> textures_;
        std::array<ComponentMapping, MaxTextureCount> component_mappings_{};
        nvrhi::BufferHandle MaterialCB_;

        void CreateAndUploadCB()
        {
            auto& device = Engine::Instance().RenderSystem()->GetRenderDevice();

            MaterialCB_ = device.Device()->createBuffer(
                nvrhi::BufferDesc().setByteSize(sizeof(MaterialData)).setIsConstantBuffer(true)
                                   .setInitialState(nvrhi::ResourceStates::ConstantBuffer).setKeepInitialState(true)
                                   .setDebugName("MaterialCB"));

            shaderResource_->MaterialCB_ = MaterialCB_;

            nvrhi::CommandListHandle upload = device.Device()->createCommandList();
            upload->open();
            upload->writeBuffer(MaterialCB_, &data_, sizeof(MaterialData));
            upload->close();
            nvrhi::ICommandList* list = upload;
            device.Device()->executeCommandLists(&list, 1);
        }

        // Loads a default texture into a slot bypassing the required-texture gate, so every
        // t0..t7 slot is always populated (needed for a valid NVRHI binding set).
        void LoadDefaultInto_(TexturesOrder order, const AssetHandle& handle)
        {
            auto texture_index = static_cast<int>(order) - 1;
            if (shaderResource_->textures_[texture_index])
                return;
            auto texture_asset = Engine::Instance().ResourceManager()->GetAsset<TextureAsset>(handle);
            textures_[texture_index] = texture_asset;
            component_mappings_[texture_index] = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            shaderResource_->textures_[texture_index] = texture_asset->GetTexture();
        }

        void CheckAndLoadRequirededTextures_()
        {
            // Always ensure all 8 slots have a texture bound (engine defaults for slots the
            // active shading/blend model does not strictly require).
            LoadDefaultInto_(TexturesOrder::BaseColor, DefaultAssetsHandles::BaseColor);
            LoadDefaultInto_(TexturesOrder::Metallic, DefaultAssetsHandles::Metallic);
            LoadDefaultInto_(TexturesOrder::Specular, DefaultAssetsHandles::Specular);
            LoadDefaultInto_(TexturesOrder::Roughness, DefaultAssetsHandles::Roughness);
            LoadDefaultInto_(TexturesOrder::Anisotropy, DefaultAssetsHandles::Anisotropy);
            LoadDefaultInto_(TexturesOrder::EmissiveColor, DefaultAssetsHandles::EmissiveColor);
            LoadDefaultInto_(TexturesOrder::Opacity, DefaultAssetsHandles::Opacity);
            LoadDefaultInto_(TexturesOrder::Normal, DefaultAssetsHandles::Normal);
        }

        RequiredTexturesMask GetRequiredTextures() const
        {
            static const std::unordered_map<ShadingModel, RequiredTexturesMask> ShadingModelRequiredTextures =
            {
                {ShadingModel::None, {}},
                {
                    ShadingModel::DefaultLit, RequiredTexturesMask(toUnderlying(
                        TexturesIndex::BaseColor | TexturesIndex::Metallic | TexturesIndex::Specular
                        | TexturesIndex::Roughness | TexturesIndex::Anisotropy | TexturesIndex::EmissiveColor
                        | TexturesIndex::Normal))
                },
                {ShadingModel::Unlit, RequiredTexturesMask(toUnderlying(TexturesIndex::EmissiveColor))}
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
