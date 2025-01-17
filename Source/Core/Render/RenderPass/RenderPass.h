#pragma once

#include<unordered_map>

#include<SceneVisibility.h>
#include<PSO.h>

namespace GiiGa
{
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Draw(RenderContext& context) =0;

    protected:
        static bool GetPsoFromMapByMask(const std::unordered_map<ObjectMask, PSO>& mask_map, const ObjectMask& objectMask, PSO& res_pso)
        {
            for (const auto& [mask, pso] : mask_map)
            {
                if (mask.CoversMask(objectMask))
                {
                    res_pso = pso;
                    return true;
                }
            }
            return false;
        }
    };
}
