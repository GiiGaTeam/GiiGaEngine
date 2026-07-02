#pragma once

// NvrhiDevice.h
//
// NVRHI does not create the underlying D3D12 device or command queues — the application
// owns them and hands raw pointers to nvrhi::d3d12::createDevice (which stores them
// without refcounting). Their lifetime must therefore outlive the nvrhi::DeviceHandle.
//
// `Rhi` is the owner that guarantees that lifetime, plus the bootstrap factory. It is a
// once-per-process object (created by Rhi::create and held by the render system), so the
// factory returns std::unique_ptr<Rhi> and builds it in place — there is deliberately NO
// copy or move because nvrhi::IMessageCallback is non-copyable.
//
// There are NO forwarding methods: everything uses the `nvrhi::DeviceHandle` (and
// `nvrhi::CommandListHandle`) directly — that's the point of switching to NVRHI.

#include "NvrhiCommon.h"

#include <memory>

#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#ifdef _DEBUG
#include <nvrhi/validation.h>
#endif

namespace GiiGa::rhi
{
    struct DeviceDesc
    {
        bool enableDebugLayer = true;        // D3D12 debug layer + GPU-based validation (debug only)
        bool enableValidation = false;       // wrap with nvrhi::validation (debug only)
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        // 0 = use NVRHI defaults for the descriptor-heap sizes forwarded to createDevice.
        uint32_t rtvHeapSize = 0;
        uint32_t dsvHeapSize = 0;
        uint32_t srvHeapSize = 0;
        uint32_t samplerHeapSize = 0;
    };

    // Owner of the D3D12 device, graphics command queue, and the NVRHI device that wraps them.
    // Members are public and used directly. Construct once via Rhi::create().
    struct Rhi
    {
        MessageCallback messages;                               // owned; NVRHI holds a raw pointer
        Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        Microsoft::WRL::ComPtr<ID3D12Device> nativeDevice;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> graphicsQueue;
        nvrhi::DeviceHandle device;                             // use this directly everywhere
    };

    // Factory: creates the D3D12 device + graphics queue and wraps them with NVRHI.
    inline std::unique_ptr<Rhi> createRhi(const DeviceDesc& desc = {})
    {
        auto rhi = std::make_unique<Rhi>();

        // --- DXGI factory + adapter ---
        {
            UINT flags = 0;
#ifdef _DEBUG
            flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
            if (FAILED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&rhi->factory))))
            {
                ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&rhi->factory)), "CreateDXGIFactory");
            }

            Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
            for (UINT i = 0; rhi->factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC1 ad{};
                if (SUCCEEDED(adapter->GetDesc1(&ad)) && !(ad.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
                {
                    rhi->adapter = adapter;
                    break;
                }
            }
            if (!rhi->adapter && adapter) rhi->adapter = adapter; // fall back to whatever was enumerated
        }

        // --- D3D12 device (+ debug layer) ---
#ifdef _DEBUG
        if (desc.enableDebugLayer)
        {
            Microsoft::WRL::ComPtr<ID3D12Debug3> debug;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
            {
                debug->EnableDebugLayer();
                debug->SetEnableGPUBasedValidation(true);
            }
        }
#endif
        ThrowIfFailed(D3D12CreateDevice(rhi->adapter.Get(), desc.featureLevel, IID_PPV_ARGS(&rhi->nativeDevice)),
                      "D3D12CreateDevice");
#ifdef _DEBUG
        if (desc.enableDebugLayer)
        {
            Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
            if (SUCCEEDED(rhi->nativeDevice.As(&infoQueue)))
            {
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
            }
        }
#endif

        // --- Graphics command queue ---
        {
            D3D12_COMMAND_QUEUE_DESC qd{};
            qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            qd.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            ThrowIfFailed(rhi->nativeDevice->CreateCommandQueue(&qd, IID_PPV_ARGS(&rhi->graphicsQueue)),
                          "CreateCommandQueue");
        }

        // --- Wrap with NVRHI ---
        nvrhi::d3d12::DeviceDesc nd;
        nd.errorCB = &rhi->messages;
        nd.pDevice = rhi->nativeDevice.Get();
        nd.pGraphicsCommandQueue = rhi->graphicsQueue.Get();
        if (desc.rtvHeapSize) nd.renderTargetViewHeapSize = desc.rtvHeapSize;
        if (desc.dsvHeapSize) nd.depthStencilViewHeapSize = desc.dsvHeapSize;
        if (desc.srvHeapSize) nd.shaderResourceViewHeapSize = desc.srvHeapSize;
        if (desc.samplerHeapSize) nd.samplerHeapSize = desc.samplerHeapSize;

        nvrhi::DeviceHandle raw = nvrhi::d3d12::createDevice(nd);
        if (!raw) throw std::runtime_error("nvrhi::d3d12::createDevice returned null");

#ifdef _DEBUG
        if (desc.enableValidation)
        {
            if (nvrhi::DeviceHandle validated = nvrhi::validation::createDevice(raw.Get()))
            {
                rhi->device = validated;
                return rhi;
            }
        }
#endif
        rhi->device = raw;
        return rhi;
    }
}
