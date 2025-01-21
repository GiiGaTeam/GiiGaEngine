#pragma once


#include <pybind11/conduit/wrap_include_python_h.h>



#include<memory>

#include<ResourceManager.h>
#include<BaseAssetDatabase.h>
#include<Input.h>
#include<Window.h>
#include<RenderSystem.h>
#include<WindowManager.h>
#include<ScriptSystem.h>
#include<Project.h>

namespace GiiGa
{

    class Engine
    {
    protected:
        static inline Engine* instance_ = nullptr;

        bool quit_ = false;
        std::shared_ptr<Window> window_;
        std::shared_ptr<RenderSystem> render_system_ = nullptr;
        std::shared_ptr<Project> project_;
        Input input;

        std::shared_ptr<ResourceManager> resource_manager_ = nullptr;
        std::shared_ptr<BaseAssetDatabase> asset_database_ = nullptr;

        std::shared_ptr<ScriptSystem> script_system_ = nullptr;

        virtual void Initialize(std::shared_ptr<Project> proj)
        {
            instance_ = this;
            project_ = proj;
            resource_manager_ = std::make_shared<GiiGa::ResourceManager>();
            auto settings = WindowSettings{"GiiGa Engine", 1240, 720};
            window_ = WindowManager::CreateWindowWithSettings(settings);

            input.Init(window_);

            window_->OnWindowClose.Register([this](const WindowCloseEvent& arg) { quit_ = true; });
            window_->OnQuit.Register([this](const QuitEvent& arg) { quit_ = true; });

            script_system_ = std::make_shared<GiiGa::ScriptSystem>(project_);
        }

        virtual void DeInitialize()
        {
            window_.reset();
            resource_manager_.reset();
            project_.reset();
        }

        /*
        IScriptSystem scriptSystem_;
        IAudioSystem audioSystem_;
        IPhysicsSystem physicsSystem_;
        IRenderSystem renderSystem_;
        ...
        */

    public:
        virtual void Run(std::shared_ptr<Project> proj) = 0;

        static Engine& Instance()
        {
            return *instance_;
        }

        std::shared_ptr<ResourceManager> ResourceManager()
        {
            return resource_manager_;
        }

        std::shared_ptr<RenderSystem> RenderSystem()
        {
            return render_system_;
        }

        std::shared_ptr<BaseAssetDatabase> AssetDatabase()
        {
            return asset_database_;
        }

        std::shared_ptr<Window> Window()
        {
            return window_;
        }
        
        std::shared_ptr<ScriptSystem> ScriptSystem()
        {
            return script_system_;
        }

        virtual ~Engine()
        {
        }
    };
}
