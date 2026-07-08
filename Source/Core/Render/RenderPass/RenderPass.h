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
        static PSO* GetPsoFromMapByMask(std::unordered_map<ObjectMask, PSO>& mask_map, const ObjectMask& objectMask)
        {
            for (auto& [mask, pso] : mask_map)
            {
                if (mask.CoversMask(objectMask))
                {
                    return &pso;
                }
            }
            return nullptr;
        }
    };
}
