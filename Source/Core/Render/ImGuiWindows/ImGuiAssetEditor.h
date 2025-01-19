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
#include<Component.h>
#include<CameraComponent.h>
#include<StaticMeshComponent.h>
#include<PointLightComponent.h>
#include<DirectionalLightComponent.h>
#include<LightComponent.h>
#include<TransformComponent.h>
#include<Material.h>
#include<Engine.h>
#include<PyBehaviourSchemeComponent.h>
#include<ScriptHelpers.h>
#include<AssetType.h>
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
        }

        void RecordImGui() override
        {
            ImGui::Begin("Asset Editor");

            ImGui::End();
        }

    private:
        std::shared_ptr<EditorContext> editorContext_;
    };
}
