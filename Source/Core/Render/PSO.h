#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<nvrhi/nvrhi.h>

#include<vector>
#include<memory>
#include<span>
#include<algorithm>

#include<RenderDevice.h>
#include<RenderContext.h>
#include<ShaderManager.h>
#include<PerObjectData.h>
#include<IObjectShaderResource.h>

namespace GiiGa
{
    // Builder for an nvrhi graphics pipeline + binding layout.
    //
    // The binding layout is declared explicitly (add_constant_buffer /
    // add_texture_srv / ...) and must cover exactly the registers the shaders
    // of this pipeline access. At draw time the pass assembles an
    // nvrhi::BindingSetDesc: pass-level items are appended directly, object
    // items (materials, lights) are appended through AppendShaderResources,
    // which filters the object's canonical items against the declared layout.
    class PSO
    {
    public:
        PSO() = default;

        //=============================SHADERS=============================

        PSO& set_vs(nvrhi::ShaderHandle vs)
        {
            VS_ = vs;
            return *this;
        }

        PSO& set_ps(nvrhi::ShaderHandle ps)
        {
            PS_ = ps;
            return *this;
        }

        PSO& set_gs(nvrhi::ShaderHandle gs)
        {
            GS_ = gs;
            return *this;
        }

        //=============================RENDER STATE=============================

        PSO& set_rasterizer_state(const nvrhi::RasterState& rasterizer_state)
        {
            render_state_.rasterState = rasterizer_state;
            return *this;
        }

        PSO& set_depth_stencil_state(const nvrhi::DepthStencilState& depth_stencil_state)
        {
            render_state_.depthStencilState = depth_stencil_state;
            return *this;
        }

        PSO& set_blend_state(const nvrhi::BlendState& blend_state)
        {
            render_state_.blendState = blend_state;
            return *this;
        }

        PSO& set_input_layout(const std::span<const nvrhi::VertexAttributeDesc>& attributes)
        {
            input_attributes_.assign(attributes.begin(), attributes.end());
            return *this;
        }

        PSO& set_primitive_type(nvrhi::PrimitiveType primitive_type)
        {
            primitive_type_ = primitive_type;
            return *this;
        }

        //=============================BINDING LAYOUT=============================

        PSO& add_constant_buffer(uint32_t slot)
        {
            layout_items_.push_back(nvrhi::BindingLayoutItem::ConstantBuffer(slot));
            return *this;
        }

        PSO& add_texture_srv(uint32_t slot)
        {
            layout_items_.push_back(nvrhi::BindingLayoutItem::Texture_SRV(slot));
            return *this;
        }

        PSO& add_structured_buffer_srv(uint32_t slot)
        {
            layout_items_.push_back(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(slot));
            return *this;
        }

        PSO& add_sampler(uint32_t slot, const nvrhi::SamplerDesc& desc)
        {
            layout_items_.push_back(nvrhi::BindingLayoutItem::Sampler(slot));
            sampler_descs_.emplace_back(slot, desc);
            return *this;
        }

        // Register slot the per-object WorldMatrices constant buffer binds to (b1 for
        // most passes, b0 for the shadow pass).
        PSO& set_per_object_slot(uint32_t slot)
        {
            per_object_slot_ = slot;
            return *this;
        }

        //=============================GENERATION=============================

        void GeneratePSO(RenderDevice& device)
        {
            // Same canonical order as RenderContext::GetOrCreateBindingSet, so
            // binding sets line up with the layout item-by-item.
            std::sort(layout_items_.begin(), layout_items_.end(),
                      [](const nvrhi::BindingLayoutItem& a, const nvrhi::BindingLayoutItem& b)
                      {
                          if (a.type != b.type)
                              return static_cast<int>(a.type) < static_cast<int>(b.type);
                          return a.slot < b.slot;
                      });

            nvrhi::BindingLayoutDesc layout_desc;
            layout_desc.visibility = nvrhi::ShaderType::All;
            for (const auto& item : layout_items_)
                layout_desc.addItem(item);

            binding_layout_ = device.NvDevice()->createBindingLayout(layout_desc);

            samplers_.clear();
            for (const auto& [slot, desc] : sampler_descs_)
                samplers_.emplace_back(slot, device.NvDevice()->createSampler(desc));

            if (!input_attributes_.empty())
            {
                input_layout_ = device.NvDevice()->createInputLayout(
                    input_attributes_.data(), static_cast<uint32_t>(input_attributes_.size()), VS_);
            }
        }

        // Pipelines require framebuffer info, so they are created lazily on first
        // draw. A pipeline stays valid for any framebuffer with the same info
        // (formats/sample count), which holds across viewport resizes.
        nvrhi::IGraphicsPipeline* GetOrCreatePipeline(RenderDevice& device, nvrhi::IFramebuffer* framebuffer)
        {
            if (!pipeline_)
            {
                nvrhi::GraphicsPipelineDesc desc;
                desc.primType = primitive_type_;
                desc.inputLayout = input_layout_;
                desc.VS = VS_;
                desc.GS = GS_;
                desc.PS = PS_;
                desc.renderState = render_state_;
                desc.bindingLayouts = {binding_layout_};

                pipeline_ = device.NvDevice()->createGraphicsPipeline(desc, framebuffer);
            }
            return pipeline_;
        }

        nvrhi::IBindingLayout* GetBindingLayout() const
        {
            return binding_layout_;
        }

        //=============================DRAW-TIME BINDING ASSEMBLY=============================

        void AppendSamplers(nvrhi::BindingSetDesc& desc) const
        {
            for (const auto& [slot, sampler] : samplers_)
                desc.addItem(nvrhi::BindingSetItem::Sampler(slot, sampler));
        }

        void AppendPerObjectData(nvrhi::BindingSetDesc& desc, PerObjectData& per_obj) const
        {
            desc.addItem(nvrhi::BindingSetItem::ConstantBuffer(per_object_slot_, per_obj.GetConstantBuffer()));
        }

        // Appends the object's canonical items, keeping only those whose
        // slot+type are declared in this pipeline's layout.
        void AppendShaderResources(nvrhi::BindingSetDesc& desc, IObjectShaderResource& resource) const
        {
            for (auto item : resource.GetBindingItems())
            {
                if (LayoutContains(item.type, item.slot))
                    desc.addItem(item);
            }
        }

    private:
        bool LayoutContains(nvrhi::ResourceType type, uint32_t slot) const
        {
            for (const auto& layout_item : layout_items_)
            {
                if (layout_item.type == type && layout_item.slot == slot)
                    return true;
            }
            return false;
        }

        nvrhi::ShaderHandle VS_;
        nvrhi::ShaderHandle PS_;
        nvrhi::ShaderHandle GS_;

        nvrhi::RenderState render_state_;
        nvrhi::PrimitiveType primitive_type_ = nvrhi::PrimitiveType::TriangleList;
        std::vector<nvrhi::VertexAttributeDesc> input_attributes_;

        std::vector<nvrhi::BindingLayoutItem> layout_items_;
        std::vector<std::pair<uint32_t, nvrhi::SamplerDesc>> sampler_descs_;
        std::vector<std::pair<uint32_t, nvrhi::SamplerHandle>> samplers_;
        uint32_t per_object_slot_ = 1;

        nvrhi::InputLayoutHandle input_layout_;
        nvrhi::BindingLayoutHandle binding_layout_;
        nvrhi::GraphicsPipelineHandle pipeline_;
    };
}
