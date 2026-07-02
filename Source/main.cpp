
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
INITIALIZE_EASYLOGGINGPP

#include<World.h>
#include<EditorEngine.h>
#include<WindowManager.h>

#include<py_module.h>

// NVRHI foundation (migration target — compiled here so the header-only TU validates it)
#include <Core/Render/Nvrhi/NvrhiCommon.h>
#include <Core/Render/Nvrhi/NvrhiDevice.h>
#include <Core/Render/Nvrhi/NvrhiSwapChain.h>
#include <Core/Render/Nvrhi/NvrhiShader.h>
#include <Core/Render/Nvrhi/NvrhiResources.h>
#include <Core/Render/Nvrhi/NvrhiConstants.h>
#include <Core/Render/Nvrhi/NvrhiGBuffer.h>
#include <Core/Render/Nvrhi/NvrhiFrame.h>
#include <Core/Render/Nvrhi/NvrhiRenderPass.h>
#include <Core/Render/Nvrhi/NvrhiClearPass.h>
#include <Core/Render/Nvrhi/NvrhiTrianglePass.h>
#include <Core/Render/Nvrhi/NvrhiTexturedQuadPass.h>
#include <Core/Render/Nvrhi/NvrhiRenderSystem.h>

void DxReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
}

// Self-contained NVRHI smoke test: opens a window, runs the NVRHI render system with a
// single clear pass for ~10s (or until the window is closed). Validates the foundation at
// runtime — device creation, swap-chain back-buffer wrapping, the keepInitialState=Present
// barrier assumption, per-frame execute/present, and garbage collection.
// Invoke with: GiiGaEngine.exe --nvrhi-smoke
static void RunNvrhiSmoke()
{
    using namespace GiiGa;

    WindowSettings ws;
    ws.title = "NVRHI Smoke Test";
    ws.width = 1280;
    ws.height = 720;
    auto window = GiiGa::WindowManager::CreateWindowWithSettings(ws);
    HWND hwnd = window->GetHandle();

    rhi::RenderSystem rs(hwnd, static_cast<uint32_t>(ws.width), static_cast<uint32_t>(ws.height));
    rs.addPass(std::make_shared<rhi::TexturedQuadPass>(rs.device(), rs.shaderCache(), &rs.swapChain()));
    rs.initialize();

    bool quit = false;
    auto quitHandle = window->OnQuit.Register([&](QuitEvent) { quit = true; });

    int frame = 0;
    for (; frame < 600 && !quit; ++frame)
    {
        window->ProcessEvents();
        rs.tick();
        SDL_Delay(16);
    }

    std::cout << "NVRHI smoke test OK: ran " << frame << " frames without crashing.\n";
}

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char* argv[])
{
    SDL_SetMainReady();
    START_EASYLOGGINGPP(argc, argv);
    el::Loggers::configureFromGlobal("logging.conf");
    LOG(INFO) << "Main Function Start";
    try
    {
        if (argc > 1 && std::string(argv[1]) == "--nvrhi-smoke")
        {
            RunNvrhiSmoke();
            DxReportLiveObjects();
            return 0;
        }
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
    }
}
