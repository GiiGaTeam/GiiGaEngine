module;

export module Engine;

import ResourceManager;

namespace GiiGa
{

export class Engine
{
protected:
    static inline Engine* instance_ = nullptr;

    ResourceManager* resource_manager_ = nullptr;

    virtual void Initialize() { 
        instance_ = this;
        resource_manager_ = new GiiGa::ResourceManager();
    }

    /*
    IScriptSystem scriptSystem_;
    IAudioSystem audioSystem_;
    IPhysicsSystem physicsSystem_;
    IRenderSystem renderSystem_;
    ...
    */

public:
    virtual void Run() = 0;

    static Engine& Instance() { 
        return *instance_;
    }

    ResourceManager& ResourceManager() { 
        return *resource_manager_;
    }

    virtual ~Engine() { 
        delete resource_manager_;
    }
};

}