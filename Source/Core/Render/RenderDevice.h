#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d12.h>
#include <wrl/client.h>

#include <iostream>
#include <stdexcept>

#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d12.h>
#include <nvrhi/validation.h>

#include <IRenderDevice.h>
#include <DirectXUtils.h>
#include <RenderSystemSettings.h>

namespace GiiGa
{
    // Bridges NVRHI diagnostic messages (validation layer + backend) to the console.
    class NvrhiMessageCallback : public nvrhi::IMessageCallback
    {
    public:
        void message(nvrhi::MessageSeverity severity, const char* messageText) override
        {
            const char* sev = "INFO";
            switch (severity)
            {
            case nvrhi::MessageSeverity::Info: sev = "INFO"; break;
            case nvrhi::MessageSeverity::Warning: sev = "WARNING"; break;
            case nvrhi::MessageSeverity::Error: sev = "ERROR"; break;
            case nvrhi::MessageSeverity::Fatal: sev = "FATAL"; break;
            }
            std::cerr << "[NVRHI][" << sev << "] " << messageText << std::endl;

            if (severity == nvrhi::MessageSeverity::Fatal)
                throw std::runtime_error(std::string("NVRHI fatal: ") + messageText);
        }
    };

    // Central GPU device wrapper. Owns the native ID3D12Device + graphics queue (still
    // needed for the DXGI swap chain and the imgui_impl_dx12 backend) and the NVRHI
    // device created on top of them. NVRHI owns all descriptor heaps, upload staging
    // and resource lifetime management.
    class RenderDevice : public IRenderDevice
    {
    public:
        RenderDevice()
        {
            CreateD3D12Device();

            D3D12_COMMAND_QUEUE_DESC queueDesc{};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            queueDesc.NodeMask = 1;
            ThrowIfFailed(d3d12_device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3d12_queue_)));

            nvrhi::d3d12::DeviceDesc deviceDesc;
            deviceDesc.errorCB = &message_callback_;
            deviceDesc.pDevice = d3d12_device_.Get();
            deviceDesc.pGraphicsCommandQueue = d3d12_queue_.Get();
            // The passes allocate a fresh binding set per draw and the viewport can recreate its
            // framebuffers on resize, so the descriptor heaps churn heavily. Size them generously
            // up front — NVRHI cannot grow a shader-visible heap mid-frame ("Failed to grow a
            // descriptor heap!"). Sampler heaps are hard-capped at 2048 shader-visible entries.
            deviceDesc.shaderResourceViewHeapSize = 1 << 18; // 262144
            deviceDesc.renderTargetViewHeapSize = 8192;
            deviceDesc.depthStencilViewHeapSize = 8192;
            deviceDesc.samplerHeapSize = 2048;

            nvrhi_d3d12_device_ = nvrhi::d3d12::createDevice(deviceDesc);

#ifndef NDEBUG
            nvrhi_device_ = nvrhi::validation::createValidationLayer(nvrhi_d3d12_device_);
#else
            nvrhi_device_ = nvrhi_d3d12_device_;
#endif
        }

        ~RenderDevice() override
        {
            if (nvrhi_device_)
                nvrhi_device_->waitForIdle();
        }

        RenderDevice(const RenderDevice&) = delete;
        RenderDevice& operator=(const RenderDevice&) = delete;

        nvrhi::IDevice* GetNvrhiDevice() override { return nvrhi_device_; }
        nvrhi::IDevice* Device() { return nvrhi_device_; }

        // The underlying (pre-validation) D3D12 NVRHI device — exposes D3D12-specific methods
        // like getDescriptorHeap(), needed to share NVRHI's shader-visible SRV heap with imgui.
        nvrhi::d3d12::IDevice* GetD3D12NvrhiDevice()
        {
            return static_cast<nvrhi::d3d12::IDevice*>(nvrhi_d3d12_device_.Get());
        }

        ID3D12Device* GetD3D12Device() const { return d3d12_device_.Get(); }
        ID3D12CommandQueue* GetD3D12Queue() const { return d3d12_queue_.Get(); }

    private:
        NvrhiMessageCallback message_callback_;
        Microsoft::WRL::ComPtr<ID3D12Device> d3d12_device_;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> d3d12_queue_;
        nvrhi::DeviceHandle nvrhi_d3d12_device_;
        nvrhi::DeviceHandle nvrhi_device_;

        void CreateD3D12Device()
        {
            D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

#ifndef NDEBUG
            Microsoft::WRL::ComPtr<ID3D12Debug3> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                std::cout << "Enabled D3D12 debug layer.\n";
                debugController->EnableDebugLayer();
            }
#endif

            ThrowIfFailed(D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&d3d12_device_)));

#ifndef NDEBUG
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(d3d12_device_->QueryInterface(IID_PPV_ARGS(&infoQueue))))
            {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            }
#endif
        }
    };
}
