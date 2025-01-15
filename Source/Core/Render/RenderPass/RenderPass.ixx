export module RenderPass;

import <memory>;
import <vector>;
import <unordered_map>;

import IRenderable;
import RenderTypes;
import SceneVisibility;
export import RenderContext;
import PSO;
namespace GiiGa
{
    export class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Draw(RenderContext& context) =0;

    protected:
        static bool GetPsoFromMapByMask(const std::unordered_map<ObjectMask, PSO>& mask_map, const ObjectMask& objectMask, PSO& res_pso)
        {
            for (const auto& [mask, pso] : mask_map)
            {
                if ((mask & objectMask) == objectMask)
                {
                    res_pso = pso;
                    return true;
                }
            }
            return false;
        }
    };
}
