module;

#include <imgui.h>
#include <string>
#include <iostream>
#include <filesystem>

export module Main;

import World;
import EditorEngine;
import ConsoleComponent;
import WindowManager;
import WindowSettings;
import Input;
import EventSystem;

export int main(int argc, char* argv[])
{
    try
    {
        std::filesystem::path project_path;
        if (argc > 1)
        {
            project_path = argv[1];
        }

        auto project = GiiGa::Project::CreateOrOpen(project_path);

        ImGui::CreateContext();

        GiiGa::EditorEngine engine = GiiGa::EditorEngine();

        engine.Run(project);

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
