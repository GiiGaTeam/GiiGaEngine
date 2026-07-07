#pragma once

// NvrhiPassCommon.h
//
// Reusable helpers shared by the NVRHI-backed deferred render passes. Collapses the
// per-pass boilerplate for: cached samplers, the per-view / per-object volatile
// constant-buffer slots (the stable-layout + stable-set + per-draw writeBuffer idiom),
// per-material binding layout/set construction, the GBuffer-derived "light" framebuffer
// (one color + shared depth), and a single-renderable draw helper.
//
// All public names live in GiiGa::rhi; they operate on raw nvrhi handles directly.

#include "NvrhiCommon.h"
#include "NvrhiResources.h"
#include "NvrhiConstants.h"
#include "NvrhiGBuffer.h"

#include <map>
#include <vector>

#include <IObjectShaderResource.h>
#include <IRenderable.h>

namespace GiiGa::rhi
{
    inline constexpr uint32_t kViewSlot = 0;       // b0: per-view constants
    inline constexpr uint32_t kObjectSlot = 1;     // b1: per-object constants
    inline constexpr uint32_t kMaterialCBSlot = 2; // b2: per-material / per-light constants

    // ---------------------------------------------------------------- samplers

    // Linear-filtering, wrap-address sampler (engine-wide "s0"). Cached per device.
    inline nvrhi::SamplerHandle getLinearWrapSampler(nvrhi::IDevice* device)
    {
        static std::map<nvrhi::IDevice*, nvrhi::SamplerHandle> cache;
        auto& s = cache[device];
        if (!s)
        {
            nvrhi::SamplerDesc d;
            d.setAllFilters(true).setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
            s = device->createSampler(d);
        }
        return s;
    }

    // Comparison sampler with border address + less-or-equal (engine-wide "s1" for shadow
    // comparisons). Cached per device.
    inline nvrhi::SamplerHandle getComparisonSampler(nvrhi::IDevice* device)
    {
        static std::map<nvrhi::IDevice*, nvrhi::SamplerHandle> cache;
        auto& s = cache[device];
        if (!s)
        {
            nvrhi::SamplerDesc d;
            d.setAllFilters(true)
                .setAllAddressModes(nvrhi::SamplerAddressMode::Border)
                .setReductionType(nvrhi::SamplerReductionType::Comparison);
            s = device->createSampler(d);
        }
        return s;
    }

    // ----------------------------------------------- volatile constant-buffer slots

    // A volatile constant buffer at a fixed slot with its own 1-item binding layout + set.
    // The layout/set are created once; per-draw/per-frame data is pushed via writeBuffer and
    // NVRHI re-binds the latest GPU VA automatically on setGraphicsState.
    struct VolatileConstantSlot
    {
        nvrhi::BufferHandle cb;
        nvrhi::BindingLayoutHandle layout;
        nvrhi::BindingSetHandle set;
        uint32_t slot = 0;

        void init(nvrhi::IDevice* device, uint32_t slotIndex, uint32_t byteSize, const char* debugName)
        {
            slot = slotIndex;
            cb = createVolatileConstantBuffer(device, byteSize, 16, debugName);

            nvrhi::BindingLayoutDesc ld;
            ld.setVisibility(nvrhi::ShaderType::All);
            ld.addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(slot));
            layout = device->createBindingLayout(ld);

            nvrhi::BindingSetDesc sd;
            sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(slot, cb));
            set = device->createBindingSet(sd, layout);
        }

        template <typename T>
        void write(nvrhi::ICommandList* cmd, const T& value) const
        {
            if (cb) cmd->writeBuffer(cb, &value, sizeof(T));
        }
    };

    // b0 per-view constants.
    struct ViewConstantSlot : VolatileConstantSlot
    {
        void init(nvrhi::IDevice* device) { VolatileConstantSlot::init(device, kViewSlot, sizeof(ViewConstants), "view vcb"); }
        void write(nvrhi::ICommandList* cmd, const ViewConstants& v) const { if (cb) cmd->writeBuffer(cb, &v, sizeof(v)); }
    };

    // b1 per-object constants (written once per draw).
    struct ObjectConstantSlot : VolatileConstantSlot
    {
        void init(nvrhi::IDevice* device) { VolatileConstantSlot::init(device, kObjectSlot, sizeof(ObjectConstants), "object vcb"); }
        void init(nvrhi::IDevice* device, uint32_t slotIndex) { VolatileConstantSlot::init(device, slotIndex, sizeof(ObjectConstants), "object vcb"); }
        void write(nvrhi::ICommandList* cmd, const ObjectConstants& v) const { if (cb) cmd->writeBuffer(cb, &v, sizeof(v)); }
    };

    // --------------------------------------------------------- material bindings

    // Builds a Pixel-visibility binding layout for the per-material block:
    //   ConstantBuffer(kMaterialCBSlot) + Texture_SRV(slot) for each slot in `textureSlots`
    //   + Sampler(0) when `withSampler`.
    inline nvrhi::BindingLayoutHandle makeMaterialBindingLayout(nvrhi::IDevice* device,
                                                                const std::vector<uint32_t>& textureSlots,
                                                                bool withSampler)
    {
        nvrhi::BindingLayoutDesc ld;
        ld.setVisibility(nvrhi::ShaderType::Pixel);
        ld.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(kMaterialCBSlot));
        for (uint32_t slot : textureSlots)
            ld.addItem(nvrhi::BindingLayoutItem::Texture_SRV(slot));
        if (withSampler)
            ld.addItem(nvrhi::BindingLayoutItem::Sampler(0));
        return device->createBindingLayout(ld);
    }

    // Fills a binding set for `layout` from a MaterialBindings block:
    //   constantBuffer -> b(kMaterialCBSlot), textures[slot] -> t(slot), sampler -> s(0).
    // All items are always present — missing material resources use the provided fallbacks
    // (nullCb / nullTex) so the binding set count matches the layout count (NVRHI requirement).
    inline nvrhi::BindingSetHandle makeMaterialBindingSet(nvrhi::IDevice* device,
                                                          nvrhi::IBindingLayout* layout,
                                                          const MaterialBindings& mb,
                                                          nvrhi::ISampler* sampler,
                                                          const std::vector<uint32_t>& textureSlots,
                                                          nvrhi::IBuffer* nullCb = nullptr,
                                                          nvrhi::ITexture* nullTex = nullptr)
    {
        nvrhi::BindingSetDesc sd;
        sd.addItem(nvrhi::BindingSetItem::ConstantBuffer(kMaterialCBSlot,
            mb.constantBuffer ? mb.constantBuffer : nullCb));
        for (uint32_t slot : textureSlots)
            sd.addItem(nvrhi::BindingSetItem::Texture_SRV(slot,
                mb.textures[slot] ? mb.textures[slot] : nullTex));
        if (sampler)
            sd.addItem(nvrhi::BindingSetItem::Sampler(0, sampler));
        return device->createBindingSet(sd, layout);
    }

    // ----------------------------------------------- GBuffer-derived framebuffers

    // Returns (building+caching once per GBuffer version) a 1-color (LightAccumulation) +
    // shared-depth framebuffer used by the light/forward/debug passes. Rebuilds automatically
    // when the owning GBuffer is resized.
    inline nvrhi::IFramebuffer* ensureLightFramebuffer(nvrhi::IDevice* device,
                                                       GBuffer& gb,
                                                       nvrhi::FramebufferHandle& cached,
                                                       nvrhi::IFramebuffer*& lastSource)
    {
        if (cached && lastSource == gb.getFramebuffer())
            return cached.Get();

        nvrhi::FramebufferDesc fbd;
        fbd.addColorAttachment(gb.getColor(GBuffer::Order::LightAccumulation));
        fbd.setDepthAttachment(gb.getDepth());
        cached = device->createFramebuffer(fbd);
        lastSource = gb.getFramebuffer();
        return cached.Get();
    }

    // ----------------------------------------------- per-renderable draw helper

    // Writes per-object constants from `r`, fills vertex/index buffers, and issues an indexed
    // draw. `state` must already have pipeline/framebuffer/viewport/bindings set; only the
    // volatile object CB and the vertex/index buffers are updated here.
    inline void drawRenderable(nvrhi::ICommandList* cmd, nvrhi::GraphicsState& state,
                               const ObjectConstantSlot& objSlot, IRenderable& r)
    {
        ObjectConstants obj;
        r.GetPerObjectData().Fill(obj);
        objSlot.write(cmd, obj);

        state.vertexBuffers.resize(0);
        if (auto* vb = r.GetVertexBuffer())
        {
            nvrhi::VertexBufferBinding vbb;
            vbb.buffer = vb;
            vbb.slot = 0;
            vbb.offset = 0;
            state.addVertexBuffer(vbb);
        }

        if (auto* ib = r.GetIndexBuffer())
        {
            nvrhi::IndexBufferBinding ibb;
            ibb.buffer = ib;
            ibb.format = r.GetIndexFormat();
            ibb.offset = 0;
            state.setIndexBuffer(ibb);
        }

        cmd->setGraphicsState(state);
        cmd->drawIndexed(nvrhi::DrawArguments().setVertexCount(r.GetIndexCount()));
    }

    // Convenience: a viewport + matching scissor rect covering the whole target.
    inline nvrhi::ViewportState fullViewportState(uint32_t width, uint32_t height)
    {
        nvrhi::Viewport vp(static_cast<float>(width), static_cast<float>(height));
        return nvrhi::ViewportState().addViewportAndScissorRect(vp);
    }
}
