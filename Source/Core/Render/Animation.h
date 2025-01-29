#pragma once
#include <memory>
#include <ozz/animation/runtime/animation.h>
//#include <ozz/animation/runtime/skeleton.h>

#include "AssetBase.h"

namespace GiiGa
{
    class Animation : public AssetBase, public std::enable_shared_from_this<Animation>
    {
    public:
        Animation(AssetHandle handle, std::shared_ptr<ozz::animation::Animation> animation
            /*,std::shared_ptr<ozz::animation::Skeleton> associated_skeleton*/)
            : AssetBase(handle), animation_(animation)/*, associated_skeleton_(associated_skeleton)*/ {}

        Animation(const Animation&) = delete;
        Animation& operator=(const Animation&) = delete;

        std::shared_ptr<ozz::animation::Animation> GetAnimation() {return animation_;}
        
        AssetType GetType() override
        {
            return AssetType::Animation;
        }
    private:
        std::shared_ptr<ozz::animation::Animation> animation_;
        //std::shared_ptr<ozz::animation::Skeleton> associated_skeleton_;
    };
}
