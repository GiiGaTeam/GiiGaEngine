#pragma once

#include<unordered_map>

#include<SceneVisibility.h>
#include<Pipeline.h>

namespace GiiGa
{
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        virtual void Draw(RenderContext& context) =0;

    protected:
        // Selects the pipeline whose mask covers the given object mask.
        static bool GetPipelineFromMapByMask(const std::unordered_map<ObjectMask, MaskPipeline>& mask_map,
                                             const ObjectMask& objectMask, MaskPipeline& res)
        {
            for (const auto& [mask, pipeline] : mask_map)
            {
                if (mask.CoversMask(objectMask))
                {
                    res = pipeline;
                    return true;
                }
            }
            return false;
        }
    };
}
