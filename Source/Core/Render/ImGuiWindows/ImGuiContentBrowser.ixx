export module ImGuiContentBrowser;

import <memory>;
import <filesystem>;
import <imgui.h>;

import Engine;
import ResourceManager;
import IImGuiWindow;
import BaseAssetDatabase;

namespace GiiGa
{
    export class ImGuiContentBrowser : public IImGuiWindow
    {
    private:
        std::shared_ptr<BaseAssetDatabase> database_;
        std::filesystem::path current_path_;

    public:
        ImGuiContentBrowser()
        {
            database_ = Engine::Instance().ResourceManager()->Database();
            assert(database_);
            current_path_ = database_->AssetPath();
        }

        void RecordImGui() override
        {
            ImGui::Begin("Content Browser");

            if (current_path_ != database_->AssetPath()) 
            {
                if (ImGui::Button("<=")) 
                {
                    current_path_ = current_path_.parent_path();
                }
            }

            for (auto& entry : std::filesystem::directory_iterator(current_path_))
            {
                const auto& path = entry.path();
                auto relative_path = std::filesystem::relative(path, database_->AssetPath());
                std::string filename = path.filename().string();

                if (entry.is_directory()) 
                {
                    if (ImGui::Button(filename.c_str()))
                    {
                        current_path_ /= path.filename();
                    }
                }
                else 
                {
                    if (database_->IsRegisteredPath(relative_path)) {
                        ImGui::Text(filename.c_str());
                    }
                }
            }
            /*if (database)
            {
                // Display registry path
                ImGui::Text("Registry Path: %s", database->registry_path_.string().c_str());

                // Display asset path
                ImGui::Text("Asset Path: %s", database->asset_path_.string().c_str());

                // Display registry map details
                if (ImGui::CollapsingHeader("Asset Registry"))
                {
                    for (const auto& [handle, meta] : database->registry_map_)
                    {
                        // Display AssetHandle and AssetMeta
                        ImGui::PushID(&handle);
                        if (ImGui::TreeNode("Asset Handle"))
                        {
                            ImGui::InputText("UUID:", const_cast<char*>(handle.id.ToString().c_str()), handle.id.ToString().length(), ImGuiInputTextFlags_ReadOnly);
                            ImGui::Text("Subresource: %d", handle.subresource);
                            ImGui::TreePop();
                        }

                        ImGui::Text("Type: %s", AssetTypeToString(meta.type).c_str());
                        ImGui::Text("Path: %s", meta.path.string().c_str());
                        ImGui::Separator();
                        ImGui::PopID();
                    }
                }
            }
            else
            {
                ImGui::Text("No asset database available.");
            }*/

            ImGui::End();
        }
    };
}
