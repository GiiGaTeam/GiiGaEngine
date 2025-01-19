#pragma once
#include <pybind11/embed.h>


#include<imgui.h>
#include<imgui_internal.h>
#include<memory>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<directxtk12/SimpleMath.h>
#include<unordered_map>
#include<json/json.h>

#include<IImGuiWindow.h>
#include<EditorContext.h>

#include"World.h"
#include<Material.h>

#include<Engine.h>
#include<BaseAssetDatabase.h>
#include<EditorAssetDatabase.h>

#include<PyBehaviourSchemeComponent.h>
#include<AssetType.h>
#include<AssetBase.h>
#include<AssetHandle.h>
#include<Logger.h>

namespace GiiGa
{
    class ImGuiAssetEditor : public IImGuiWindow
    {
    public:
        ImGuiAssetEditor(std::shared_ptr<EditorContext> ec) :
            editorContext_(ec)
        {
            database_ = std::dynamic_pointer_cast<EditorAssetDatabase>(Engine::Instance().ResourceManager()->Database());
        }

        void RecordImGui() override
        {
            ImGui::Begin("Asset Editor");

            if (auto asset = editorContext_->selectedAsset)
            {
                ImGuiComponentWidgetFactory(asset);
            }

            ImGui::End();
        }

    private:
        void DrawMaterialProperties(std::shared_ptr<Material> material)
        {
            if (!material) {
                return;
            }

            ImGui::Text("Material: %s", material->name.c_str());
            
            auto material_handle = material->GetId();
            std::string text_handle = material_handle.id.ToString() + " " + std::to_string(material_handle.subresource);

            // Shading Model
            static const char* shadingModelNames[] = {
                "None", "DefaultLit", "Unlit"
            };
            int currentShadingModel = static_cast<int>(material->GetMaterialMask().GetShadingModel());
            if (ImGui::Combo("Shading Model", &currentShadingModel, shadingModelNames, IM_ARRAYSIZE(shadingModelNames)))
            {
                material->SetShadingModel(static_cast<ShadingModel>(currentShadingModel));
            }

            // Blend Mode
            static const char* blendModeNames[] = {
                "None", "Opaque", "Masked", "Translucent"
            };
            int currentBlendMode = static_cast<int>(material->GetMaterialMask().GetBlendMode());
            if (ImGui::Combo("Blend Mode", &currentBlendMode, blendModeNames, IM_ARRAYSIZE(blendModeNames)))
            {
                material->SetBlendMode(static_cast<BlendMode>(currentBlendMode));
            }

            // Define required texture indices based on the blend mode and shading model
            RequiredTexturesMask requiredTextures = material->GetRequiredTextures();
            {
                TexturesOrder texture_order = TexturesOrder::BaseColor;
                int texture_index = static_cast<int>(texture_order) - 1;
                if (requiredTextures[texture_index] && material->textures_[texture_index])
                {
                    AssetHandle texture_handle = material->textures_[texture_index]->GetId();
                    std::string text_handle = texture_handle.id.ToString() + " " + std::to_string(texture_handle.subresource);

                    char textureUuidStr[512];
                    snprintf(textureUuidStr, sizeof(textureUuidStr), "%s", text_handle.c_str());
                    ImGui::InputText("BaseColor Texture Handle", textureUuidStr, text_handle.size(), ImGuiInputTextFlags_ReadOnly);

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Texture2D)))
                        {
                            AssetHandle* handle = (AssetHandle*)payload->Data;
                            material->SetTexture(texture_order, *handle);
                        }

                        ImGui::EndDragDropTarget();
                    }

                    DirectX::SimpleMath::Vector3 val = material->data_.BaseColorTint_;
                    if (ImGui::ColorEdit3("BaseColor Tint", &val.x))
                    {
                        material->SetBaseColorTint(val);
                    }
                }
                else
                {
                    ImGui::Text("You Can't Set BaseColor");
                }
            }

            {
                TexturesOrder texture_order = TexturesOrder::EmissiveColor;
                int texture_index = static_cast<int>(texture_order) - 1;
                if (requiredTextures[texture_index] && material->textures_[texture_index])
                {
                    AssetHandle texture_handle = material->textures_[texture_index]->GetId();
                    std::string text_handle = texture_handle.id.ToString() + " " + std::to_string(texture_handle.subresource);

                    char textureUuidStr[512];
                    snprintf(textureUuidStr, sizeof(textureUuidStr), "%s", text_handle.c_str());

                    ImGui::InputText("EmissiveColor Texture Handle", textureUuidStr, text_handle.size(), ImGuiInputTextFlags_ReadOnly);

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Texture2D)))
                        {
                            AssetHandle* handle = (AssetHandle*)payload->Data;
                            material->SetTexture(texture_order, *handle);
                        }

                        ImGui::EndDragDropTarget();
                    }

                    DirectX::SimpleMath::Vector3 val = material->data_.EmissiveColorTint_;
                    if (ImGui::ColorEdit3("EmissiveColor Tint", &val.x))
                    {
                        material->SetEmissiveTint(val);
                    }
                }
                else
                {
                    ImGui::Text("You Can't Set EmissiveColor");
                }
            }

            {
                TexturesOrder texture_order = TexturesOrder::Metallic;
                int texture_index = static_cast<int>(texture_order) - 1;
                if (requiredTextures[texture_index] && material->textures_[texture_index])
                {
                    AssetHandle texture_handle = material->textures_[texture_index]->GetId();
                    std::string text_handle = texture_handle.id.ToString() + " " + std::to_string(texture_handle.subresource);

                    char textureUuidStr[512];
                    snprintf(textureUuidStr, sizeof(textureUuidStr), "%s", text_handle.c_str());

                    ImGui::InputText("Metallic Texture Handle", textureUuidStr, text_handle.size(), ImGuiInputTextFlags_ReadOnly);

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Texture2D)))
                        {
                            AssetHandle* handle = (AssetHandle*)payload->Data;
                            material->SetTexture(texture_order, *handle);
                        }

                        ImGui::EndDragDropTarget();
                    }

                    float val = material->data_.MetallicScale_;
                    if (ImGui::SliderFloat("Metallic Scale", &val, 0.0, 1.0))
                    {
                        material->SetMetallicScale(val);
                    }
                }
                else
                {
                    ImGui::Text("You Can't Set Metallic");
                }
            }

            if (ImGui::Button("Save")) {
                database_->SaveAsset(material);
            }
        }

        void ImGuiComponentWidgetFactory(std::shared_ptr<AssetBase> asset)
        {
            ImGui::PushID(asset->GetId().id.ToString().c_str());
            if (auto material = std::dynamic_pointer_cast<Material>(asset))
            {
                DrawMaterialProperties(material);
            }
    
            ImGui::PopID();
        }

        std::shared_ptr<EditorContext> editorContext_;
        std::shared_ptr<EditorAssetDatabase> database_;
    };
}
