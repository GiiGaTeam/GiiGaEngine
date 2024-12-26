export module Viewport;

import<vector>;
import<d3d12.h>;
import<memory>;
import<directxtk/SimpleMath.h>;

import RenderDevice;
export import GPULocalResource;
export import DescriptorHeap;
export import RenderPassViewData;
import BufferView;
import CameraComponent;
import RenderContext;
import GBuffer;

namespace GiiGa
{
    export class RenderGraph;

    export class Viewport
    {
    public:
        explicit Viewport(RenderDevice& device):
            device_(device),
            gbuffer_(std::make_shared<GBuffer>(device, viewport_size_))
        {
        }

        virtual void Init(RenderContext& context) = 0;

        virtual ~Viewport() = default;

        virtual RenderPassViewData GetCameraInfo() =0;

        virtual void Execute(RenderContext& context) =0;

        void Resize(DirectX::SimpleMath::Vector2 new_size)
        {
            viewport_size_ = new_size;

            gbuffer_->Resize(new_size);
        }

    protected:
        RenderDevice& device_;
        DirectX::SimpleMath::Vector2 viewport_size_ = DirectX::SimpleMath::Vector2(300, 300);
        std::shared_ptr<RenderGraph> renderGraph_;
        std::shared_ptr<GBuffer> gbuffer_;
    };
}
