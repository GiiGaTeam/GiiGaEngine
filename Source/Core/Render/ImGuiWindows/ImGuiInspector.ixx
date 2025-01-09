module;

#include <pybind11/pybind11.h>;

export module ImGuiInspector;

import <imgui.h>;
import <imgui_internal.h>;
import <memory>;
import <directxtk12/SimpleMath.h>;
import <unordered_map>;

import IImGuiWindow;
import EditorContext;
import World;
import Component;
import CameraComponent;
import StaticMeshComponent;
import PointLightComponent;
import DirectionalLightComponent;
import LightComponent;
import TransformComponent;
import Material;

import PyBehaviourSchemeComponent;

import AssetType;
import AssetHandle;

namespace GiiGa
{
    export class ImGuiInspector : public IImGuiWindow
    {
    public:
        ImGuiInspector(std::shared_ptr<EditorContext> ec):
            editorContext_(ec)
        {
        }

        void RecordImGui() override
        {
            ImGui::Begin("Inspector");

            if (auto gameobject = editorContext_->selectedGameObject.lock())
            {
                {
                    // GO Name
                    char buf[256] = {0};
                    strcpy_s(buf, sizeof(buf), gameobject->name.c_str());

                    if (ImGui::InputText("Tag", buf, sizeof(buf)))
                    {
                        gameobject->name = std::string(buf);
                    }
                }

                auto transform = gameobject->GetTransformComponent();

                if (auto l_transfrom = transform.lock())
                {
                    ImGui::PushID(l_transfrom->GetUuid().ToString().c_str());
                    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        DrawTransformComponent(l_transfrom);
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }

                for (auto&& comp : gameobject->GetComponents())
                {
                    ImGuiComponentWidgetFactory(comp);
                }

                if (ImGui::Button("Add Component"))
                    ImGui::OpenPopup("Add Component");

                if (ImGui::BeginPopup("Add Component"))
                {
                    if (ImGui::MenuItem("Static Mesh Component"))
                    {
                        if (auto l_go = editorContext_->selectedGameObject.lock())
                        {
                            l_go->CreateComponent<StaticMeshComponent>();
                        }
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::MenuItem("Point Light Component"))
                    {
                        if (auto l_go = editorContext_->selectedGameObject.lock())
                        {
                            l_go->CreateComponent<PointLightComponent>();
                        }
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::MenuItem("Direction Light Component"))
                    {
                        if (auto l_go = editorContext_->selectedGameObject.lock())
                        {
                            l_go->CreateComponent<DirectionalLightComponent>();
                        }
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::MenuItem("Behaviour Component"))
                    {
                        if (auto l_go = editorContext_->selectedGameObject.lock())
                        {
                            l_go->CreateComponent<PyBehaviourSchemeComponent>();
                        }
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
            }

            ImGui::End();
        }

    private:
        std::shared_ptr<EditorContext> editorContext_;

        void DrawTransformComponent(std::shared_ptr<TransformComponent> comp)
        {
            auto location = comp->GetLocation();
            auto rot_deg = comp->GetRotation();
            auto scale = comp->GetScale();

            if (DrawVec3Control("Translation", location))
            {
                comp->SetLocation(location);
            }

            if (DrawVec3Control("Rotation", rot_deg))
            {
                comp->SetRotation(rot_deg);
            }

            if (DrawVec3Control("Scale", scale, 1.0f))
            {
                comp->SetScale(scale);
            }
        }

        void DrawCameraComponent(std::shared_ptr<CameraComponent> comp)
        {
            auto camera = comp->GetCamera();

            // Camera type
            const char* camera_types[] = {"Perspective", "Orthographic"};
            int current_type = static_cast<int>(camera.type_);
            if (ImGui::Combo("Camera Type", &current_type, camera_types, IM_ARRAYSIZE(camera_types)))
            {
                comp->SetType(static_cast<CameraType>(current_type));
            }

            // Perspective settings
            if (camera.type_ == Perspective)
            {
                if (ImGui::SliderFloat("FOV", &camera.FOV_, 1.0f, 179.0f))
                {
                    comp->SetFOVinDeg(camera.FOV_);
                }
                if (ImGui::InputFloat("Aspect Ratio", &camera.aspect_, 0.01f, 0.1f))
                {
                    comp->SetAspect(camera.aspect_);
                }
            }
            // Orthographic settings
            else if (camera.type_ == Orthographic)
            {
                if (ImGui::InputFloat("Width", &camera.width_, 1.0f, 10.0f))
                {
                    comp->SetWidth(camera.width_);
                }
                if (ImGui::InputFloat("Height", &camera.height_, 1.0f, 10.0f))
                {
                    comp->SetHeight(camera.height_);
                }
            }

            // Common settings
            if (ImGui::InputFloat("Near Plane", &camera.near_, 0.01f, 0.1f))
            {
                comp->SetNear(camera.near_);
            }
            if (ImGui::InputFloat("Far Plane", &camera.far_, 1.0f, 10.0f))
            {
                comp->SetFar(camera.far_);
            }
        }

        void DrawStaticMeshComponent(std::shared_ptr<StaticMeshComponent> comp)
        {
            auto mesh_handle = comp->GetMeshHandle();
            std::string text_handle = mesh_handle.id.ToString() + " " + std::to_string(mesh_handle.subresource);

            char meshUuidStr[512];
            snprintf(meshUuidStr, sizeof(meshUuidStr), "%s", text_handle.c_str());

            ImGui::InputText("Mesh Handle", meshUuidStr, text_handle.size(), ImGuiInputTextFlags_ReadOnly);

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Mesh)))
                {
                    AssetHandle* handle = (AssetHandle*)payload->Data;
                    comp->SetMeshHandle(*handle);
                }

                ImGui::EndDragDropTarget();
            }

            auto material = comp->material_;

            if (material && ImGui::CollapsingHeader("Material Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto material_handle = comp->GetMaterialHandle();
                std::string text_handle = material_handle.id.ToString() + " " + std::to_string(material_handle.subresource);

                char materialUuidStr[512];
                snprintf(materialUuidStr, sizeof(materialUuidStr), "%s", text_handle.c_str());
                ImGui::InputText("Material Handle", materialUuidStr, text_handle.size(), ImGuiInputTextFlags_ReadOnly);

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Material)))
                    {
                        AssetHandle* handle = (AssetHandle*)payload->Data;
                        comp->SetMaterialHandle(*handle);
                    }

                    ImGui::EndDragDropTarget();
                }

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
            }
        }

        void DrawPointLightComponent(std::shared_ptr<PointLightComponent> comp)
        {
            // Edit Color
            DirectX::SimpleMath::Vector3 color = comp->GetData().color;
            float colorArray[3] = {color.x, color.y, color.z};
            if (ImGui::ColorEdit3("Color", colorArray))
            {
                comp->SetColor({colorArray[0], colorArray[1], colorArray[2]});
            }

            // Edit Radius
            float radius = comp->GetData().radius;
            if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 1000.0f))
            {
                comp->SetRadius(radius);
            }

            // Edit Intensity
            float intensity = comp->GetData().max_intensity;
            if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 10.0f))
            {
                comp->SetIntensity(intensity);
            }

            // Edit Falloff
            float falloff = comp->GetData().falloff;
            if (ImGui::DragFloat("Falloff", &falloff, 0.01f, 0.0f, 10.0f))
            {
                comp->SetFallOff(falloff);
            }
        }

        void DrawDirectionLightComponent(std::shared_ptr<DirectionalLightComponent> comp)
        {
            // Edit Color
            DirectX::SimpleMath::Vector3 color = comp->GetData().color;
            float colorArray[3] = {color.x, color.y, color.z};
            if (ImGui::ColorEdit3("Color", colorArray))
            {
                comp->SetColor({colorArray[0], colorArray[1], colorArray[2]});
            }

            // Edit Intensity
            float intensity = comp->GetData().max_intensity;
            if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 10.0f))
            {
                comp->SetIntensity(intensity);
            }
        }

        void DrawBehaviourComponent(std::shared_ptr<PyBehaviourSchemeComponent> comp)
        {
            auto script_handle = comp->GetScriptHandle();
            std::string text_handle = script_handle.id.ToString() + " " + std::to_string(script_handle.subresource);

            char scriptUuidStr[512];
            snprintf(scriptUuidStr, sizeof(scriptUuidStr), "%s", text_handle.c_str());

            ImGui::InputText("Script Handle", scriptUuidStr, text_handle.size(), ImGuiInputTextFlags_ReadOnly);

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Behaviour)))
                {
                    AssetHandle* handle = (AssetHandle*)payload->Data;
                    comp->SetScriptHandle(*handle);
                }

                ImGui::EndDragDropTarget();
            }
        }

        void ImGuiComponentWidgetFactory(std::shared_ptr<IComponent> comp)
        {
            ImGui::PushID(comp->GetUuid().ToString().c_str());
            if (auto camera_comp = std::dynamic_pointer_cast<CameraComponent>(comp))
            {
                if (ImGui::TreeNodeEx("Camera Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    DrawCameraComponent(camera_comp);
                    ImGui::TreePop();
                }
            }
            else if (auto static_mesh_comp = std::dynamic_pointer_cast<StaticMeshComponent>(comp))
            {
                if (ImGui::TreeNodeEx("StaticMeshComponent", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    DrawStaticMeshComponent(static_mesh_comp);
                    ImGui::TreePop();
                }
            }
            else if (auto point_light = std::dynamic_pointer_cast<PointLightComponent>(comp))
            {
                if (ImGui::TreeNodeEx("PointLightComponent", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    DrawPointLightComponent(point_light);
                    ImGui::TreePop();
                }
            }
            else if (auto direction_light = std::dynamic_pointer_cast<DirectionalLightComponent>(comp))
            {
                if (ImGui::TreeNodeEx("DirectionalLightComponent", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    DrawDirectionLightComponent(direction_light);
                    ImGui::TreePop();
                }
            }
            else if (auto py_beh = std::dynamic_pointer_cast<PyBehaviourSchemeComponent>(comp))
            {
                //todo: add script Class name to TreeNodeEx
                if (ImGui::TreeNodeEx("PyBehaviourSchemeComponent", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    DrawBehaviourComponent(py_beh);
                    ImGui::TreePop();
                }
            }
            ImGui::PopID();
        }

        static bool DrawVec3Control(const std::string& label, DirectX::SimpleMath::Vector3& values, float resetValue = 0.0f,
                                    float columnWidth = 100.0f)
        {
            ImGuiIO& io = ImGui::GetIO();
            auto boldFont = io.Fonts->Fonts[0];
            bool edited = false;

            ImGui::PushID(label.c_str());

            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, columnWidth);
            ImGui::Text(label.c_str());
            ImGui::NextColumn();

            ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

            const float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            const ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("X", buttonSize))
            {
                values.x = resetValue;
                edited = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
                edited = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("Y", buttonSize))
            {
                values.y = resetValue;
                edited = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
                edited = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("Z", buttonSize))
            {
                values.z = resetValue;
                edited = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
                edited = true;
            ImGui::PopItemWidth();

            ImGui::PopStyleVar();

            ImGui::Columns(1);

            ImGui::PopID();

            return edited;
        }
    };
}
