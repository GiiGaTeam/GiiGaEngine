export module ImGuiSceneHierarchy;

import <imgui.h>;
import <memory>;
import <ranges>;
import <optional>;

import IImGuiWindow;
import World;
import EditorContext;
import Logger;
import Engine;
import EditorAssetDatabase;
import AssetBase;
import PrefabAsset;

namespace GiiGa
{
    export class ImGuiSceneHierarchy : public IImGuiWindow
    {
    public:
        ImGuiSceneHierarchy(std::shared_ptr<EditorContext> ec):
            editorContext_(ec)
        {
        }

        void RecordImGui() override
        {
            ImGui::Begin("Scene Hierarchy");

            auto levels = World::GetInstance().GetLevels();

            for (auto level : levels)
            {
                RecursiveDrawLevel(level);
            }

            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
            {
                //if (selected_)
                //    deselect(selected_);
                editorContext_->selectedGameObject.reset();
            }

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
        std::shared_ptr<EditorContext> editorContext_;

        void RecursiveDrawLevel(std::shared_ptr<Level> level)
        {
            const ImGuiTreeNodeFlags flags =
                (!level->GetNumRootGameObjects() != 0 ? ImGuiTreeNodeFlags_Leaf : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow;

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.467f, 0.686f, 1.0f, 1.0f));

            auto level_lable = "Level: " + level->GetLevelName();

            const bool opened = ImGui::TreeNodeEx(
                level_lable.c_str(),
                flags, level_lable.c_str());

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Prefab))) {
                    AssetHandle* handle = (AssetHandle*)payload->Data;
                    auto prefab = Engine::Instance().ResourceManager()->GetAsset<PrefabAsset>(*handle);
                    el::Loggers::getLogger(LogWorld)->info("Loaded Prefab %v", handle->id.ToString());

                    auto new_go = prefab->Instantiate(std::nullopt, std::nullopt);
                    new_go->AttachToLevelRoot(level);
                }

                ImGui::EndDragDropTarget();
            }

            ImGui::PopStyleColor();

            if (ImGui::BeginPopupContextItem(level_lable.c_str()))
            {
                if (ImGui::MenuItem("Add GameObject"))
                {
                    GameObject::CreateEmptyGameObject({.LevelOverride = level});
                }
                if (ImGui::MenuItem("Save"))
                {
                    auto database = Engine::Instance().ResourceManager()->Database();
                    std::dynamic_pointer_cast<EditorAssetDatabase>(database)->SaveAsset(std::dynamic_pointer_cast<AssetBase>(level));
                }

                ImGui::EndPopup();
            }


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
            ImGuiTreeNodeFlags flags =
                (!gameObject->GetChildrenCount() != 0 ? ImGuiTreeNodeFlags_Leaf : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow;

            if (!editorContext_->selectedGameObject.expired())
                flags |= (editorContext_->selectedGameObject.lock() == gameObject ? ImGuiTreeNodeFlags_Selected : 0);


            if (gameObject->prefab_handle_ != AssetHandle{})
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));

            const bool opened = ImGui::TreeNodeEx(
                gameObject->GetUuid().ToString().c_str(),
                flags,
                gameObject->name.c_str());

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(AssetTypeToStaticString(AssetType::Prefab))) {
                    AssetHandle* handle = (AssetHandle*)payload->Data;
                    auto prefab = Engine::Instance().ResourceManager()->GetAsset<PrefabAsset>(*handle);
                    el::Loggers::getLogger(LogWorld)->info("Loaded Prefab %v", handle->id.ToString());

                    auto new_go = prefab->Instantiate(std::nullopt, std::nullopt);
                    new_go->SetParent(gameObject);
                }

                ImGui::EndDragDropTarget();
            }

            if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
            {
                if (auto go = editorContext_->selectedGameObject.lock())
                {
                    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
                    {
                        go->Destroy();
                        editorContext_->selectedGameObject.reset();
                    }
                }
            }

            if (gameObject->prefab_handle_ != AssetHandle{})
                ImGui::PopStyleColor();

            if (ImGui::BeginPopupContextItem(gameObject->GetUuid().ToString().c_str()))
            {
                if (ImGui::MenuItem("Add Child"))
                {
                    auto kid = GameObject::CreateEmptyGameObject({.Owner = gameObject});
                    kid->SetParent(gameObject);
                }

                if (ImGui::MenuItem("Remove GameObject"))
                {
                    gameObject->Destroy();
                }

                if (ImGui::MenuItem("Save as Prefab"))
                {
                    auto database = Engine::Instance().ResourceManager()->Database();
                    auto prefab = std::make_shared<PrefabAsset>(AssetHandle{Uuid::New(), 0}, gameObject);
                    std::dynamic_pointer_cast<EditorAssetDatabase>(database)->CreateAsset(prefab, std::string{gameObject->name + ".prefab"});
                }

                ImGui::EndPopup();
            }

            if (ImGui::IsItemClicked())
            {
                //if (selected_)
                //{
                //    deselect(selected_);
                //}

                editorContext_->selectedGameObject = gameObject;

                //select(selected_);
            }

            if (opened)
            {
                for (auto kid : gameObject->GetChildren())
                {
                    RecursiveDrawGameObject(kid);
                }
                ImGui::TreePop();
            }
        }
    };
}
