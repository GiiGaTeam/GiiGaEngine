module;

#include <imgui.h>
#include<string>
#include<iostream>
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

export int main()
{
    ImGui::CreateContext();

    std::shared_ptr<GiiGa::Project> proj;
    
    {
        GiiGa::EditorEngine engine = GiiGa::EditorEngine();

        engine.Run(proj);
    }

    ImGui::DestroyContext();

    IDXGIDebug1* dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }

    return 0;
}
