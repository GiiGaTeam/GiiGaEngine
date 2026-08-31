#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>

#include<cassert>
#include<memory>
#include<vector>
#include<iostream>
#include<stdexcept>
#include<tracy/tracy/Tracy.hpp>
#include<tracy/tracy/TracyD3D12.hpp>

#include<IRenderDevice.h>
#include<DirectXUtils.h>
#include<NvrhiUtils.h>
#include<RenderSystemSettings.h>

namespace GiiGa
{
    // A single slot in the ImGui shader-visible SRV heap.
    struct ImGuiSrvHandle
    {
        uint32_t index = UINT32_MAX;
        D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
        D3D12_GPU_DESCRIPTOR_HANDLE gpu{};

        bool IsValid() const { return index != UINT32_MAX; }
    };

    // The GPU device layer. Owns the native D3D12 device and graphics queue,
    // and wraps them into an NVRHI device that the rest of the renderer works with.
    // Also owns the dedicated shader-visible SRV heap used by Dear ImGui.
    class RenderDevice : public IRenderDevice
    {
        friend class RenderSystem;

    private:
        RenderDevice():
            IRenderDevice(RenderSystemSettings::NUM_BACK_BUFFERS)
        {
            device_ = std::shared_ptr<ID3D12Device>{CreateDevice(), DXDeleter{}};

            {
                D3D12_COMMAND_QUEUE_DESC desc{};
                desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                desc.NodeMask = 1;

                ID3D12CommandQueue* queue = nullptr;
                ThrowIfFailed(device_->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
                graphics_queue_ = std::shared_ptr<ID3D12CommandQueue>(queue, DXDeleter{});
            }

            {
                nvrhi::d3d12::DeviceDesc device_desc;
                device_desc.errorCB = &NvrhiMessageCallback::GetInstance();
                device_desc.pDevice = device_.get();
                device_desc.pGraphicsCommandQueue = graphics_queue_.get();

                nvrhi_device_ = nvrhi::d3d12::createDevice(device_desc);

#ifndef NDEBUG
                nvrhi_device_ = nvrhi::validation::createValidationLayer(nvrhi_device_);
#endif
            }

            // Tracy GPU profiling context for the graphics queue.
            tracy_gpu_ctx_ = TracyD3D12Context(device_.get(), graphics_queue_.get());

            CreateImGuiSrvHeap();
        }

    public:
        ~RenderDevice() override
        {
            if (tracy_gpu_ctx_)
                TracyD3D12Destroy(tracy_gpu_ctx_);

            for (auto&& vec : delete_queue)
            {
                vec.clear();
            }
        }

        nvrhi::IDevice* NvDevice() override
        {
            return nvrhi_device_;
        }

        std::shared_ptr<ID3D12Device> GetDxDevice()
        {
            return device_;
        }

        std::shared_ptr<ID3D12CommandQueue> GetGraphicsQueue()
        {
            return graphics_queue_;
        }

        tracy::D3D12QueueCtx* GetTracyGpuContext()
        {
            return tracy_gpu_ctx_;
        }

        ///////////////////////// IMGUI SRV HEAP /////////////////////////////////////////////////

        ID3D12DescriptorHeap* GetImGuiSrvHeap()
        {
            return imgui_srv_heap_.get();
        }

        ImGuiSrvHandle AllocateImGuiDescriptor()
        {
            if (imgui_free_slots_.empty())
                throw std::runtime_error("ImGui SRV heap exhausted");

            uint32_t index = imgui_free_slots_.back();
            imgui_free_slots_.pop_back();

            ImGuiSrvHandle handle;
            handle.index = index;
            handle.cpu = imgui_heap_cpu_start_;
            handle.cpu.ptr += static_cast<SIZE_T>(index) * imgui_descriptor_size_;
            handle.gpu = imgui_heap_gpu_start_;
            handle.gpu.ptr += static_cast<UINT64>(index) * imgui_descriptor_size_;
            return handle;
        }

        void FreeImGuiDescriptor(uint32_t index)
        {
            imgui_free_slots_.push_back(index);
        }

        // Frees the slot only after all frames potentially referencing it have retired.
        void FreeImGuiDescriptorDeferred(uint32_t index)
        {
            struct DeferredFree
            {
                RenderDevice* device;
                uint32_t index;

                DeferredFree(RenderDevice* device, uint32_t index): device(device), index(index)
                {
                }

                DeferredFree(DeferredFree&& other) noexcept: device(other.device), index(other.index)
                {
                    other.device = nullptr;
                }

                ~DeferredFree()
                {
                    if (device) device->FreeImGuiDescriptor(index);
                }
            };

            KeepAliveForFramesInFlight(unique_any(DeferredFree{this, index}));
        }

        // Creates an SRV for the given nvrhi texture in the ImGui heap so it can be
        // displayed with ImGui::Image. The returned gpu handle is the ImTextureID.
        ImGuiSrvHandle CreateImGuiTextureSrv(nvrhi::ITexture* texture)
        {
            ZoneScoped;
            ImGuiSrvHandle handle = AllocateImGuiDescriptor();

            auto native_resource = static_cast<ID3D12Resource*>(texture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource).pointer);
            const auto& tex_desc = texture->getDesc();

            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
            srv_desc.Format = SrvFormatFor(nvrhi::d3d12::convertFormat(tex_desc.format));
            srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv_desc.Texture2D.MostDetailedMip = 0;
            srv_desc.Texture2D.MipLevels = tex_desc.mipLevels;

            device_->CreateShaderResourceView(native_resource, &srv_desc, handle.cpu);

            return handle;
        }

    private:
        std::shared_ptr<ID3D12Device> device_;
        std::shared_ptr<ID3D12CommandQueue> graphics_queue_;
        nvrhi::DeviceHandle nvrhi_device_;
        tracy::D3D12QueueCtx* tracy_gpu_ctx_ = nullptr;

        std::shared_ptr<ID3D12DescriptorHeap> imgui_srv_heap_;
        D3D12_CPU_DESCRIPTOR_HANDLE imgui_heap_cpu_start_{};
        D3D12_GPU_DESCRIPTOR_HANDLE imgui_heap_gpu_start_{};
        UINT imgui_descriptor_size_ = 0;
        std::vector<uint32_t> imgui_free_slots_;

        void CreateImGuiSrvHeap()
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NumDescriptors = RenderSystemSettings::IMGUI_SRV_HEAP_SIZE;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            ID3D12DescriptorHeap* heap = nullptr;
            ThrowIfFailed(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
            imgui_srv_heap_ = std::shared_ptr<ID3D12DescriptorHeap>(heap, DXDeleter{});

            imgui_heap_cpu_start_ = heap->GetCPUDescriptorHandleForHeapStart();
            imgui_heap_gpu_start_ = heap->GetGPUDescriptorHandleForHeapStart();
            imgui_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            imgui_free_slots_.reserve(RenderSystemSettings::IMGUI_SRV_HEAP_SIZE);
            for (uint32_t i = RenderSystemSettings::IMGUI_SRV_HEAP_SIZE; i > 0; --i)
                imgui_free_slots_.push_back(i - 1);
        }

        // Depth formats cannot be used in SRVs directly; map to their color-readable equivalent.
        static DXGI_FORMAT SrvFormatFor(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
            case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_UNORM;
            default: return format;
            }
        }

        ID3D12Device* CreateDevice()
        {
            ID3D12Device* device;
            D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

#ifndef NDEBUG
            ID3D12Debug3* pdx12Debug = nullptr;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
            {
                std::cout << "Enabled debug.\n";
                pdx12Debug->EnableDebugLayer();
                pdx12Debug->SetEnableGPUBasedValidation(true);
            }
            else
            {
                throw std::runtime_error("Failed to get D3D12 debug layer.");
            }
#endif

            ThrowIfFailed(D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&device)));

#ifndef NDEBUG
            if (pdx12Debug != nullptr)
            {
                ID3D12InfoQueue* pInfoQueue = nullptr;
                device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                pInfoQueue->Release();
                pdx12Debug->Release();
            }
#endif

            return device;
        }
    };
}
