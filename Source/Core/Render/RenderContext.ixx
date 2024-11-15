module;

#include<memory>
#include<vector>
#include<d3d12.h>

export module RenderContext;

import RenderSystemSettings;
import RenderDevice;
import FrameContext;
import Window;

namespace GiiGa
{

    export class RenderContext final
    {
    public:
        void Create(RenderDevice& device)
        {
            {
                D3D12_COMMAND_QUEUE_DESC desc = {};
                desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 1;
                graphics_command_queue_ = device.CreateCommandQueue(desc);
            }

            {
                frame_contexts_.reserve(RenderSystemSettings::NUM_FRAMES_IN_FLIGHT);
                for (UINT i = 0; i < RenderSystemSettings::NUM_FRAMES_IN_FLIGHT; i++)
                    frame_contexts_.emplace_back(device);
            }

            {
                graphics_command_list_ = device.CreateGraphicsCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT,frame_contexts_[0].command_allocator);
            }
        }
        
    private:
        std::shared_ptr<ID3D12CommandQueue> graphics_command_queue_;
        std::shared_ptr<ID3D12GraphicsCommandList> graphics_command_list_;
        std::vector<FrameContext> frame_contexts_;
    };
};