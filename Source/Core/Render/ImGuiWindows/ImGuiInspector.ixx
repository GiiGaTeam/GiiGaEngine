export module ImGuiInspector;

import <imgui.h>;
import <imgui_internal.h>;
import <memory>;
import <directxtk12/SimpleMath.h>;

import IImGuiWindow;
import EditorContext;
import World;
import CameraComponent;
import StaticMeshComponent;
import PointLightComponent;
import DirectionLightComponent;
import LightComponent;

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
            ImGui::Begin("ImGuiInspector");

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
                    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        auto location = l_transfrom->GetLocation();
                        auto rot_deg = l_transfrom->GetRotation();
                        auto scale = l_transfrom->GetScale();
                        if (drawVec3Control("Translation", location))
                        {
                            l_transfrom->SetLocation(location);
                        }
                        if (drawVec3Control("Rotation", rot_deg))
                        {
                            l_transfrom->SetRotation(rot_deg);
                        }
                        if (drawVec3Control("Scale", scale, 1.0f))
                        {
                            l_transfrom->SetScale(scale);
                        }

                        ImGui::TreePop();
                    }
                }

                for (auto&& [_,comp] : gameobject->GetComponents())
                {
                    if (auto camera_comp = std::dynamic_pointer_cast<CameraComponent>(comp))
                    {
                        if (ImGui::TreeNodeEx("Camera Component", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            auto camera = camera_comp->GetCamera();

                            // Camera type
                            const char* camera_types[] = {"Perspective", "Orthographic"};
                            int current_type = static_cast<int>(camera.type_);
                            if (ImGui::Combo("Camera Type", &current_type, camera_types, IM_ARRAYSIZE(camera_types)))
                            {
                                camera_comp->SetType(static_cast<CameraType>(current_type));
                            }

                            // Perspective settings
                            if (camera.type_ == Perspective)
                            {
                                auto fov = camera.FOV_;
                                if (ImGui::SliderFloat("FOV", &fov, 1.0f, 179.0f))
                                {
                                    camera_comp->SetFOVinDeg(camera.FOV_);
                                }
                                if (ImGui::InputFloat("Aspect Ratio", &camera.aspect_, 0.01f, 0.1f))
                                {
                                    camera_comp->SetAspect(camera.aspect_);
                                }
                            }
                            // Orthographic settings
                            else if (camera.type_ == Orthographic)
                            {
                                if (ImGui::InputFloat("Width", &camera.width_, 1.0f, 10.0f))
                                {
                                    camera_comp->SetWidth(camera.width_);
                                }
                                if (ImGui::InputFloat("Height", &camera.height_, 1.0f, 10.0f))
                                {
                                    camera_comp->SetHeight(camera.height_);
                                }
                            }

                            // Common settings
                            if (ImGui::InputFloat("Near Plane", &camera.near_, 0.01f, 0.1f))
                            {
                                camera_comp->SetNear(camera.near_);
                            }
                            if (ImGui::InputFloat("Far Plane", &camera.far_, 1.0f, 10.0f))
                            {
                                camera_comp->SetFar(camera.far_);
                            }

                            ImGui::TreePop();
                        }
                    }
                    else if (auto static_mesh_comp = std::dynamic_pointer_cast<StaticMeshComponent>(comp))
                    {
                        if (ImGui::TreeNodeEx("StaticMeshComponent", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            // Get and display the Mesh UUID
                            Uuid meshUuid = static_mesh_comp->GetMeshUuid();
                            char meshUuidStr[37];
                            snprintf(meshUuidStr, sizeof(meshUuidStr), "%s", meshUuid.ToString().c_str());
                            if (ImGui::InputText("Mesh UUID", meshUuidStr, sizeof(meshUuidStr)))
                            {
                                auto newUuid = Uuid::FromString(meshUuidStr);
                                if (newUuid.has_value())
                                    static_mesh_comp->SetMeshUuid(newUuid.value());
                            }

                            // Get and display the Material UUID
                            Uuid materialUuid = static_mesh_comp->GetMaterialUuid();
                            char materialUuidStr[37];
                            snprintf(materialUuidStr, sizeof(materialUuidStr), "%s", materialUuid.ToString().c_str());
                            if (ImGui::InputText("Material UUID", materialUuidStr, sizeof(materialUuidStr)))
                            {
                                auto newUuid = Uuid::FromString(materialUuidStr);
                                if (newUuid.has_value())
                                    static_mesh_comp->SetMaterialUuid(newUuid.value());
                            }

                            ImGui::TreePop();
                        }
                    }                    
                    else if (auto light_comp = std::dynamic_pointer_cast<LightComponent>(comp))
                    {
                        if (ImGui::TreeNodeEx("LightComponent", ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            auto rot_deg = light_comp->GetColor();
                            auto intensity = light_comp->GetIntensity();
                            if (drawColorControl("Color", rot_deg))
                            {
                                light_comp->SetColor(rot_deg);
                            }
                            if (ImGui::InputFloat("Far Plane", &intensity, 1.0f, 10.0f))
                            {
                                light_comp->SetIntensity(intensity);
                            }

                            ImGui::TreePop();
                        }
                    }
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
                            l_go->CreateComponent<DirectionLightComponent>();
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

        static bool drawVec3Control(const std::string& label, DirectX::SimpleMath::Vector3& values, float resetValue = 0.0f,
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

            const float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
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

        static bool drawColorControl(const std::string& label, DirectX::SimpleMath::Vector3& values, float resetValue = 0.0f,
                                    float columnWidth = 100.0f)
        {
            ImGuiIO& io = ImGui::GetIO();
            auto boldFont = io.Fonts->Fonts[0];
            bool edited = false;

            ImGui::PushID(label.c_str());

            ImGui::Columns(3);
            ImGui::SetColumnWidth(0, columnWidth);
            ImGui::Text(label.c_str());
            ImGui::NextColumn();

            ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

            const float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            const ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("R", buttonSize))
            {
                values.x = resetValue;
                edited = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##R", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
                edited = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("G", buttonSize))
            {
                values.y = resetValue;
                edited = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##G", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
                edited = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushFont(boldFont);
            if (ImGui::Button("B", buttonSize))
            {
                values.z = resetValue;
                edited = true;
            }
            ImGui::PopFont();
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##B", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
                edited = true;
            ImGui::PopItemWidth();

            ImGui::PopStyleVar();

            ImGui::NextColumn();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{values.x, values.y, values.z, 1.0f});
            ImGui::({0, 0}, {1, 1}, 45);
            ImGui::PopStyleColor();
            
            ImGui::Columns(2);
            ImGui::PopID();
            return edited;
        }
    };
}
