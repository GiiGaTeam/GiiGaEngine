module;

export module Engine;

namespace GiiGa
{

export class Engine
{
private:
    virtual void Initialize() = 0;

    /*
    IScriptSystem scriptSystem_;
    IAudioSystem audioSystem_;
    IPhysicsSystem physicsSystem_;
    IRenderSystem renderSystem_;
    ...
    */

public:
    virtual void Run() = 0;
};

}