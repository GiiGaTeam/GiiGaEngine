export module ImGuiSceneHierarchy;

import <imgui.h>;
import <memory>;

import IImGuiWindow;
import World;

namespace GiiGa
{
    export class ImGuiSceneHierarchy : public IImGuiWindow
    {
    public:
        ImGuiSceneHierarchy() = default;

        void RecordImGui() override
        {
            ImGui::Begin("Scene Hierarchy");

            auto levels = World::GetInstance().GetLevels();

            for (auto level : levels)
            {
                RecursiveDrawLevel(level);
            }

            //if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            //{
            //    if (selected_)
            //        deselect(selected_);
            //    selected_ = scene_.getNullEntt();
            //}

            //if (ImGui::BeginPopupContextWindow())
            //{
            //    if (ImGui::MenuItem("Create Empty Entity"))
            //        scene_.addNewEntityToRoot("Empty Entity");
            //
            //    ImGui::EndPopup();
            //}
            
            ImGui::End();
        }

    private:
        void RecursiveDrawLevel(std::shared_ptr<Level> level)
        {
            const ImGuiTreeNodeFlags flags =
                (!level->GetNumRootGameObjects() != 0 ? ImGuiTreeNodeFlags_Leaf : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow;

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.467f, 0.686f, 1.0f, 1.0f));

            const bool opened = ImGui::TreeNodeEx(
                level->GetLevelName().c_str(),
                flags);

            ImGui::PopStyleColor();

            if (opened)
            {
                for (auto&& [_,go] : level->GetRootGameObjects())
                {
                    RecursiveDrawGameObject(std::dynamic_pointer_cast<GameObject>(go));
                }
                ImGui::TreePop();
            }
        }

        void RecursiveDrawGameObject(const std::shared_ptr<GameObject> gameObject)
        {
            const ImGuiTreeNodeFlags flags =
                //(selected_ == enttity ? ImGuiTreeNodeFlags_Selected : 0) |
                (!gameObject->GetChildrenCount() != 0 ? ImGuiTreeNodeFlags_Leaf : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow;

            const bool opened = ImGui::TreeNodeEx(
                gameObject->GetUuid().ToString().c_str(),
                flags,
                gameObject->GetName().c_str());

            if (opened)
            {
                for (auto&& [_,kid] : gameObject->GetChildren())
                {
                    RecursiveDrawGameObject(kid);
                }
                ImGui::TreePop();
            }
        }
    };
}
