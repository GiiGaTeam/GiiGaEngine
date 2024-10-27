module;
#include <memory>
#include <vector>

export module RenderSystem;
import IDrawable;

namespace GiiGa
{

export class RenderSystem
{
public:
    std::vector<std::shared_ptr<IDrawable>> drawable_objets_;
};

static int const NUM_FRAMES_IN_FLIGHT = 3;

}
