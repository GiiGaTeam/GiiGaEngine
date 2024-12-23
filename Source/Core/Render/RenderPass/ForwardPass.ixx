#include<directxtk12/SimpleMath.h>

export module ForwardPass;
import <memory>;
import <vector>;
import <functional>;
import <d3d12.h>;
#include <directx/d3dx12_core.h>

import RenderPass;
import PSO;
import IRenderable;
import SceneVisibility;
import RenderPassViewData;
import Logger;
import ShaderManager;
import RenderContext;
import VertexTypes;
import CameraComponent;

namespace GiiGa
{
    export class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(const std::function<RenderPassViewMatricies()>& getCamDescFn):
            getCamInfoDataFunction_(getCamDescFn)
        {
        }

        void Draw(RenderContext& context) override
        {
              /*D3D12_INPUT_ELEMENT_DESC InputElements[5] =
            {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            };

            D3D12_INPUT_LAYOUT_DESC il = {InputElements, 5};*/

            
            auto pso = PSO();
            pso.set_vs(ShaderManager::GetShaderByName(VertexPNTBTShader));
            pso.set_ps(ShaderManager::GetShaderByName(OpaqueUnlitShader));
            D3D12_RASTERIZER_DESC rast_desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            rast_desc.CullMode = D3D12_CULL_MODE_NONE;
            pso.set_rasterizer_state(rast_desc);
            pso.set_input_layout(VertexPNTBT::InputLayout);
               
            pso.GeneratePSO(context.GetDevice(), 2, 0, 0);

            context.BindPSO(pso);
            context.SetSignature(pso);
            context.GetGraphicsCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            auto cam_info = getCamInfoDataFunction_();
            auto viewProj = cam_info.viewMatrix * cam_info.projMatrix;

            const auto& visibles = SceneVisibility::Extract(renderpass_filter, viewProj);
            // Getting renderables

            if (visibles.size() > 0)
                el::Loggers::getLogger(LogRendering)->debug("See some renderable");

            cam_info = {cam_info.viewMatrix.Transpose(), cam_info.projMatrix.Transpose()};
            const auto CameraMatricesSpan = std::span{reinterpret_cast<uint8_t*>(&cam_info), sizeof(RenderPassViewMatricies)};
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = D3D12_CONSTANT_BUFFER_VIEW_DESC(0, sizeof(RenderPassViewMatricies));
            std::shared_ptr<BufferView<Constant>> ConstantBufferView_ = context.AllocateDynamicConstantView(CameraMatricesSpan, 1, desc);

            for (auto& visible : visibles)
            {
                for (auto& common_resource_group : visible.second.common_resource_renderables)
                {
                    for (auto& renderable : common_resource_group.second.renderables)
                    {
                        context.BindDescriptorHandle(1, ConstantBufferView_->getDescriptor().getGPUHandle());
                        renderable.lock()->Draw(context);
                    }
                }
            }
        }

    private:
        ObjectMask renderpass_filter = ObjectMask().SetBlendMode(BlendMode::Opaque | BlendMode::Masked)
                                                   .SetShadingModel(ShadingModel::All)
                                                   .SetVertexType(VertexTypes::All);
        std::function<RenderPassViewMatricies()> getCamInfoDataFunction_;
    };
}
