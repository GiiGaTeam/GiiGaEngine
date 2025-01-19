#pragma once
#include <ImGuizmo.h>


#include<memory>

#include<GameObject.h>
#include<AssetBase.h>

namespace GiiGa
{
    struct EditorContext
    {
        EditorContext() = default;
        std::weak_ptr<GameObject> selectedGameObject;
        std::shared_ptr<AssetBase> selectedAsset;
        ImGuizmo::OPERATION currentOperation_ = ImGuizmo::OPERATION::UNIVERSAL;
        ImGuizmo::MODE currentMode_ = ImGuizmo::MODE::WORLD;
    };
}
