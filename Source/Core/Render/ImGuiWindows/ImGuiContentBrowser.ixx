export module ImGuiContentBrowser;

import<memory>;
import <imgui.h>;

import Engine;
import ResourceManager;
import IImGuiWindow;

namespace GiiGa
{
    export class ImGuiContentBrowser : public IImGuiWindow
    {
    public:
        ImGuiContentBrowser()
        {
        }

        void RecordImGui() override
        {
            ImGui::Begin("Content Browser");

            auto database = Engine::Instance().ResourceManager()->database_;

            if (database)
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
            }

            ImGui::End();
        }
    };
}
