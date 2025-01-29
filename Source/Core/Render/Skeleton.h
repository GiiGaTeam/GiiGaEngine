#pragma once
#include <memory>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

#include "Animation.h"
#include "AssetBase.h"

namespace GiiGa
{
    /*class SkeletonShaderResource : public IObjectShaderResource
    {
        friend class Skeleton;
        public:
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> GetDescriptors() override
        {
            std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> result;
            result.push_back(SkeletonCBV_->getDescriptor().getGPUHandle());
            return result;
        }
        private:
        std::shared_ptr<BufferView<Constant>> SkeletonCBV_;
    };
    */
    
    class Skeleton : public AssetBase, public std::enable_shared_from_this<Skeleton>
    {
        public:
        Skeleton(AssetHandle handle, std::shared_ptr<ozz::animation::Skeleton> skeleton)
            : AssetBase(handle), skeleton_(skeleton)
        {
            // Allocates runtime buffers.
            const int num_soa_joints = skeleton_->num_soa_joints();
            locals_.resize(num_soa_joints);
            const int num_joints = skeleton_->num_joints();
            models_.resize(num_joints);

            // Allocates a context that matches animation requirements.
            context_.Resize(num_joints);
        }

        std::shared_ptr<ozz::animation::Skeleton> GetSkeleton(){ return skeleton_; }

        void AddAnimation(std::shared_ptr<Animation> animation)
        {
            animations_.push_back(animation);
            current_animation_ = animation;
        }

        std::shared_ptr<Animation> GetCurrentAnimation() {return current_animation_;}

        std::shared_ptr<Animation> GetFirstAnimation()
        {
            if (animations_.size() > 0)
                return animations_[0];
            return nullptr;
        }

        Skeleton(const Skeleton&) = delete;
        Skeleton& operator=(const Skeleton&) = delete;
        
        AssetType GetType() override
        {
            return AssetType::Skeleton;
        }
        
        ozz::vector<ozz::math::SoaTransform> locals_;
        ozz::vector<ozz::math::Float4x4> models_;
        ozz::animation::SamplingJob::Context context_;
            
        private:
        std::shared_ptr<ozz::animation::Skeleton> skeleton_;
        std::shared_ptr<Animation> current_animation_;
        std::vector<std::shared_ptr<Animation>> animations_;
        
    };
}
