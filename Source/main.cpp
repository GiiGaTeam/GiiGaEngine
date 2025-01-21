
#define SDL_MAIN_HANDLED
#include <pybind11/embed.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <imgui.h>
#include<string>
#include<iostream>
#include <filesystem>
#include <dxgi1_4.h>
#include <wrl.h>
#include <DXGIDebug.h> // Ensure this header is included
#pragma comment(lib, "dxguid.lib")
#include <memory>

#include "easylogging++.h"
#include "Physics/PhysicsSystem.h"
INITIALIZE_EASYLOGGINGPP

#include<World.h>
#include<EditorEngine.h>
#include<WindowManager.h>

#include<py_module.h>

void DxReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
}

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char* argv[])
{
    /*SDL_SetMainReady();
    START_EASYLOGGINGPP(argc, argv);
    el::Loggers::configureFromGlobal("logging.conf");
    LOG(INFO) << "Main Function Start";
    try
    {
        {
            std::filesystem::path project_path;
            if (argc > 1)
            {
                project_path = argv[1];
            }

            auto project = GiiGa::Project::CreateOrOpen(project_path);

            GiiGa::EditorEngine engine = GiiGa::EditorEngine();

            engine.Run(project);
        }

        DxReportLiveObjects();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        DxReportLiveObjects();
        return 1;
    }*/

    GiiGa::PhysicsSystem::main(argc, argv);
    return 0;
}
