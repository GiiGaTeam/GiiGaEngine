export module EditorContext;

import <memory>;

import GameObject;


namespace GiiGa
{
    export struct EditorContext
    {
        EditorContext() = default;
        std::weak_ptr<GameObject> selectedGameObject;
    };
}
