module;

#include <memory>

export module Engine;

import ResourceManager;
import BaseAssetDatabase;
import Input;
import Window;
import RenderSystem;
import WindowManager;
export import Project;

namespace GiiGa
{
    export class Engine
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

        virtual void Initialize(std::shared_ptr<Project> proj)
        {
            instance_ = this;
            project_ = proj;
            resource_manager_ = std::make_shared<GiiGa::ResourceManager>();
            auto settings = WindowSettings{"GiiGa Engine", 1240, 720};
            window_ = WindowManager::CreateWindow(settings);

            input.Init(window_);

            window_->OnWindowClose.Register([this](const WindowCloseEvent& arg) { quit_ = true; });
            window_->OnQuit.Register([this](const QuitEvent& arg) { quit_ = true; });
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

        virtual ~Engine()
        {
        }
    };
}
