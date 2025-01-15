export module ImGuiContentBrowser;

import <memory>;
import <filesystem>;
import <imgui.h>;
import <unordered_map>;

import <directxtk12/WICTextureLoader.h>;
import <directxtk12/ResourceUploadBatch.h>;

import <iostream>;

import Logger;
import Engine;

import ResourceManager;
import IImGuiWindow;
import BaseAssetDatabase;
import EditorAssetDatabase;
import GPULocalResource;
import RenderDevice;

import AssetHandle;
import AssetType;

import EventSystem;
import Window;

import GameObject;
import PrefabAsset;
import Material;

namespace GiiGa
{
    enum class CreateAssetState {
        None,
        Folder,
        Material
    };

    std::unique_ptr<GPULocalResource> LoadEditorIcon(
        RenderDevice& rd,
        DirectX::ResourceUploadBatch& batcher,
        const std::filesystem::path& path
    );

    export class ImGuiContentBrowser : public IImGuiWindow
    {
    private:
        std::shared_ptr<EditorAssetDatabase> database_;
        std::filesystem::path current_path_;

        std::unordered_map<AssetType, std::unique_ptr<GPULocalResource>> icons_;
        std::unordered_map<AssetType, std::shared_ptr<BufferView<ShaderResource>>> icons_srv_;

        float padding = 16.0f;
        float thumbnail_size = 128.0f;

        EventHandle<DropFileEvent> drop_file_evt_ = EventHandle<DropFileEvent>::Null();
        CreateAssetState create_asset_state_ = CreateAssetState::None;

    public:
        ImGuiContentBrowser(const ImGuiContentBrowser& obj) = delete;
        ImGuiContentBrowser& operator=(const ImGuiContentBrowser& obj) = delete;

        ImGuiContentBrowser()
        {
            database_ = std::dynamic_pointer_cast<EditorAssetDatabase>(Engine::Instance().ResourceManager()->Database());
            assert(database_);
            current_path_ = database_->AssetPath();

            auto rs = Engine::Instance().RenderSystem();
            auto device = rs->GetRenderDevice().GetDxDevice();

            DirectX::ResourceUploadBatch upload_batch(device.get());
            upload_batch.Begin();

            {
                auto icon = LoadEditorIcon(rs->GetRenderDevice(), upload_batch, "EditorData/folder.png");

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                auto srv = icon->EmplaceShaderResourceBufferView(srvDesc);

                icons_.emplace(AssetType::Unknown, std::move(icon));
                icons_srv_.emplace(AssetType::Unknown, srv);
            }

            {
                auto icon = LoadEditorIcon(rs->GetRenderDevice(), upload_batch, "EditorData/texture.png");

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                auto srv = icon->EmplaceShaderResourceBufferView(srvDesc);

                icons_.emplace(AssetType::Texture2D, std::move(icon));
                icons_srv_.emplace(AssetType::Texture2D, srv);
            }

            {
                auto icon = LoadEditorIcon(rs->GetRenderDevice(), upload_batch, "EditorData/obj.png");

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                auto srv = icon->EmplaceShaderResourceBufferView(srvDesc);

                icons_.emplace(AssetType::Mesh, std::move(icon));
                icons_srv_.emplace(AssetType::Mesh, srv);
            }
            
            {
                auto icon = LoadEditorIcon(rs->GetRenderDevice(), upload_batch, "EditorData/material.png");

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                auto srv = icon->EmplaceShaderResourceBufferView(srvDesc);

                icons_.emplace(AssetType::Material, std::move(icon));
                icons_srv_.emplace(AssetType::Material, srv);
            }

            {
                auto icon = LoadEditorIcon(rs->GetRenderDevice(), upload_batch, "EditorData/level.png");

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                auto srv = icon->EmplaceShaderResourceBufferView(srvDesc);

                icons_.emplace(AssetType::Level, std::move(icon));
                icons_srv_.emplace(AssetType::Level, srv);
            }

            {
                auto icon = LoadEditorIcon(rs->GetRenderDevice(), upload_batch, "EditorData/prefab.png");

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
                ZeroMemory(&srvDesc, sizeof(srvDesc));
                srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                auto srv = icon->EmplaceShaderResourceBufferView(srvDesc);

                icons_.emplace(AssetType::Prefab, std::move(icon));
                icons_srv_.emplace(AssetType::Prefab, srv);
            }

            auto future = upload_batch.End(rs->GetRenderContext().getGraphicsCommandQueue().get());
            future.wait();

            drop_file_evt_ = Engine::Instance().Window()->OnDropFile.Register([this](const auto& evt) {
                try {
                    std::filesystem::copy(evt.path, current_path_);
                }
                catch (const std::exception& e) {
                    el::Loggers::getLogger(LogEditor)->error("Failed drop file: %v", e.what());
                }
                });
        }

        void RecordImGui() override
        {
            ImGui::Begin("Content Browser");

            ImVec2 cursor_pos = ImGui::GetCursorPos();
            ImGui::Dummy(ImGui::GetContentRegionAvail());

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GOTOPREFAB")) {
                    std::shared_ptr<GameObject> go = *(std::shared_ptr<GameObject>*)payload->Data;

                    auto prefab = std::make_shared<PrefabAsset>(AssetHandle{ Uuid::New(), 0 }, go);
                    go->prefab_handle_ = database_->CreateAsset(prefab, current_path_ / std::string{ go->name + ".prefab" });
                }

                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginPopupContextItem("CONTENTBROWSER")) {
                if (ImGui::MenuItem("Create Folder")) {
                    create_asset_state_ = CreateAssetState::Folder;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Create Material")) {
                    create_asset_state_ = CreateAssetState::Material;
                }

                ImGui::EndPopup();
            }

            ImGui::SetCursorPos(cursor_pos);

            ImGui::TextWrapped("Current path: %s", current_path_.string().c_str());

            float cell_size = padding + thumbnail_size;
            float panel_width = ImGui::GetContentRegionAvail().x;

            int column_count = (int)(panel_width / cell_size);
            if (column_count < 1) {
                column_count = 1;
            }

            ImGui::Columns(column_count, nullptr, false);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            if (current_path_ != database_->AssetPath()) 
            {
                auto& icon = icons_srv_[AssetType::Unknown]->getDescriptor();
                ImGui::ImageButton("BackBtn", (ImTextureID)icon.getGPUHandle().ptr, { thumbnail_size, thumbnail_size });
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    current_path_ = current_path_.parent_path();
                }
                ImGui::Text("..");

                ImGui::NextColumn();
            }

            // Context menu logic
            switch (create_asset_state_)
            {
            case CreateAssetState::Folder:
            {
                auto& icon = icons_srv_[AssetType::Unknown]->getDescriptor();
                ImGui::ImageButton("BackBtn", (ImTextureID)icon.getGPUHandle().ptr, { thumbnail_size, thumbnail_size });

                char folder_name_buf[512];
                snprintf(folder_name_buf, sizeof(folder_name_buf), "%s", "");

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##", folder_name_buf, IM_ARRAYSIZE(folder_name_buf));

                if (!ImGui::IsItemActive() && strlen(folder_name_buf) > 0) {
                    create_asset_state_ = CreateAssetState::None;

                    std::filesystem::path new_folder_path = current_path_ / folder_name_buf;
                    try {
                        std::filesystem::create_directory(new_folder_path);
                    }
                    catch (const std::exception& e) {
                        el::Loggers::getLogger(LogEditor)->error("Failed to create folder: %v", e.what());
                    }
                }

                ImGui::NextColumn();
            }
            break;

            case CreateAssetState::Material:
            {
                auto& icon = icons_srv_[AssetType::Material]->getDescriptor();
                ImGui::ImageButton("MaterialBtn", (ImTextureID)icon.getGPUHandle().ptr, { thumbnail_size, thumbnail_size });

                char material_name_buf[512];
                snprintf(material_name_buf, sizeof(material_name_buf), "%s", "");

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##", material_name_buf, IM_ARRAYSIZE(material_name_buf));

                if (!ImGui::IsItemActive() && strlen(material_name_buf) > 0) {
                    create_asset_state_ = CreateAssetState::None;

                    auto material = std::make_shared<Material>(material_name_buf);
                    auto filename = material->name + ".mat";

                    database_->CreateAsset(material, current_path_ / filename);
                }

                ImGui::NextColumn();
            }
            break;

            default:
                break;
            }

            // Asset displaying
            for (auto& entry : std::filesystem::directory_iterator(current_path_))
            {
                const auto& path = entry.path();
                auto relative_path = std::filesystem::relative(path, database_->AssetPath());
                std::string filename = path.filename().string();
                auto stem = path.stem();
                auto extension = path.extension();

                if (entry.is_directory()) 
                {
                    auto& icon = icons_srv_[AssetType::Unknown]->getDescriptor();
                    ImGui::ImageButton(filename.c_str(), (ImTextureID)icon.getGPUHandle().ptr, {thumbnail_size, thumbnail_size});

                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        current_path_ /= path.filename();
                    }

                    if (ImGui::BeginPopupContextItem(filename.c_str())) {
                        if (ImGui::MenuItem("Remove")) {
                            std::filesystem::remove_all(path);
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::TextWrapped(filename.c_str());

                    ImGui::NextColumn();
                }
                else 
                {
                    auto range = database_->GetHandlesByPath(relative_path);

                    for (const auto& [handle, meta] : range) {
                        if (meta.type != AssetType::Unknown)
                        {
                            //ImGui::PushID(static_cast<int>(std::hash(handle)));
                            auto& icon = icons_srv_[meta.type]->getDescriptor();
                            auto id = handle.id.ToString() + std::to_string(handle.subresource);
                            ImGui::ImageButton(id.c_str(), (ImTextureID)icon.getGPUHandle().ptr, {thumbnail_size, thumbnail_size});

                            const char* raw_name = filename.c_str();

                            std::string name;

                            if (range.size() > 1) {
                                name = stem.string() + "_" + meta.name + extension.string();
                                raw_name = name.c_str();
                            }

                            if (ImGui::IsItemActive() && ImGui::BeginDragDropSource()) {
                                ImGui::SetDragDropPayload(AssetTypeToStaticString(meta.type), &handle, sizeof(AssetHandle));
                                ImGui::EndDragDropSource();
                            }

                            if (ImGui::BeginPopupContextItem()) {
                                ImGui::Text(
                                    "UUID: %s Subresource: %d",
                                    const_cast<char*>(handle.id.ToString().c_str()),
                                    handle.subresource
                                );

                                if (ImGui::MenuItem("Remove")) {
                                    std::filesystem::remove(path);
                                }
                                
                                ImGui::EndPopup();
                            }

                            //ImGui::PopID();

                            ImGui::TextWrapped(raw_name);

                            ImGui::NextColumn();
                        }
                    }
                }
            }

            ImGui::PopStyleColor();
            ImGui::Columns(1);

            ImGui::End();
        }
    };

    std::unique_ptr<GPULocalResource> LoadEditorIcon(
        RenderDevice& rd,
        DirectX::ResourceUploadBatch& batcher,
        const std::filesystem::path& path
    ) {
        ID3D12Resource* texture = nullptr;

        HRESULT hr = DirectX::CreateWICTextureFromFile(
            rd.GetDxDevice().get(),
            batcher,
            path.c_str(),
            &texture
        );

        if (FAILED(hr))
            throw std::runtime_error("Failed to load icon: " + path.string());

        auto texture_ptr = std::shared_ptr<ID3D12Resource>(texture, DXDelayedDeleter{ rd });

        return std::make_unique<GPULocalResource>(rd, texture_ptr);
    }
}