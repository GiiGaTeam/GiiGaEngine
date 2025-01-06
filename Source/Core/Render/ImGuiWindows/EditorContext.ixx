#include <imgui.h>
#include <ImGuizmo.h>

export module EditorContext;

import <memory>;

import GameObject;


namespace GiiGa
{
    export struct EditorContext
    {
        EditorContext() = default;
        std::weak_ptr<GameObject> selectedGameObject;
        ImGuizmo::OPERATION currentOperation_ = ImGuizmo::OPERATION::UNIVERSAL;
        ImGuizmo::MODE currentMode_ = ImGuizmo::MODE::WORLD;
    };
}
