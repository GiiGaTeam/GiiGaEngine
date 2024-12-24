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

            float padding = 16.0f;
            float thumbnail_size = 128.0f;
            float cell_size = padding + thumbnail_size;
            float panel_width = ImGui::GetContentRegionAvail().x;

            int column_count = std::max((int)(panel_width / cell_size), 1);

            ImGui::Columns(column_count, nullptr, false);
            if (current_path_ != database_->AssetPath()) 
            {
                ImGui::Button("..", { thumbnail_size, thumbnail_size });
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    current_path_ = current_path_.parent_path();
                }
                ImGui::Text("..");

                ImGui::NextColumn();
            }

            for (auto& entry : std::filesystem::directory_iterator(current_path_))
            {
                const auto& path = entry.path();
                auto relative_path = std::filesystem::relative(path, database_->AssetPath());
                std::string filename = path.filename().string();

                if (entry.is_directory()) 
                {
                    ImGui::Button(filename.c_str(), { thumbnail_size, thumbnail_size });
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        current_path_ /= path.filename();
                    }
                    ImGui::Text(filename.c_str());
                }
                else 
                {
                    if (database_->IsRegisteredPath(relative_path)) 
                    {
                        if (ImGui::Button(filename.c_str(), { thumbnail_size, thumbnail_size }))
                        {
                        }
                        ImGui::Text(filename.c_str());
                    }
                }
               
                ImGui::NextColumn();
            }

            ImGui::Columns(1);

            ImGui::End();
        }
    };
}
