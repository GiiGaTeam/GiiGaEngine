module;

#include <imgui.h>
#include<string>
#include<iostream>

export module Main;

import World;
import EditorEngine;
import ConsoleComponent;
import WindowManager;
import WindowSettings;
import Input;
import EventSystem;

export int main()
{
    ImGui::CreateContext();

    std::shared_ptr<GiiGa::Project> proj;
    
    GiiGa::EditorEngine engine = GiiGa::EditorEngine();

    engine.Run(proj);

    return 0;
}
