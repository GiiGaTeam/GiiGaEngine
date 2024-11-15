module;

#include <cstdint>
#include <OCIdl.h>

export module RenderSystemSettings;


namespace GiiGa
{
    export struct RenderSystemSettings
    {
        // default value 3, can be changed 
        // on engine startup and settings parse
        static inline UINT8 NUM_FRAMES_IN_FLIGHT = 4;
        static inline UINT8 NUM_BACK_BUFFERS = 3;
        static inline UINT32 NUM_CBV_SRV_UAVs = 1 << 16;

        static inline uint32_t CPUDescriptorHeapAllocationSize[4]
        {
            8192, // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            2048, // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
            1024, // D3D12_DESCRIPTOR_HEAP_TYPE_RTV
            1024 // D3D12_DESCRIPTOR_HEAP_TYPE_DSV
        };

        static inline uint32_t GPUDescriptorHeapSize[2]
        {
            16384, // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            1024 // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        };

        static inline uint32_t GPUDescriptorHeapDynamicSize[2]
        {
            8192, // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
            1024 // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        };
    };
}