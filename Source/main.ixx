module;

#include <imgui.h>
#include<string>
#include<iostream>
#include <filesystem>
#include <dxgi1_4.h>
#include <wrl.h>
#include <DXGIDebug.h> // Ensure this header is included
#pragma comment(lib, "dxguid.lib")

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

        ImGui::DestroyContext();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    IDXGIDebug1* dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }

    return 0;
}
