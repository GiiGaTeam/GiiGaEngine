module;
#include <memory>
#include <vector>

export module SceneVisibility;
import IRenderable;
import ITickable;

namespace GiiGa
{
    export class SceneVisibility : public ITickable
    {
    public:
        SceneVisibility()
        {
        };
        virtual ~SceneVisibility() = default;

        void Extract(int32_t render_filter_type, std::vector<std::weak_ptr<IRenderable>>& filtered_renderables)
        {
            //TODO
        }

    protected:
        std::vector<std::weak_ptr<IRenderable>> renderables_;
    };
}