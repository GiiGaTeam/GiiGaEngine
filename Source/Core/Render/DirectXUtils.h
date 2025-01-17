#pragma once

#include<exception>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include<d3d12.h>

#include<IRenderDevice.h>

inline void hash_combine(size_t& seed, size_t value)
{
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <>
struct std::hash<D3D12_CONSTANT_BUFFER_VIEW_DESC>
{
    std::size_t operator()(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc) const noexcept
    {
        std::size_t h1 = std::hash<D3D12_GPU_VIRTUAL_ADDRESS>()(desc.BufferLocation);
        std::size_t h2 = std::hash<UINT>()(desc.SizeInBytes);
        return h1 ^ (h2 << 1); // Combine hashes
    }
};

// Equal_to specialization for D3D12_CONSTANT_BUFFER_VIEW_DESC
bool operator==(const D3D12_CONSTANT_BUFFER_VIEW_DESC& lhs, const D3D12_CONSTANT_BUFFER_VIEW_DESC& rhs)
{
    return lhs.BufferLocation == rhs.BufferLocation &&
        lhs.SizeInBytes == rhs.SizeInBytes;
};

template <>
struct std::hash<D3D12_SHADER_RESOURCE_VIEW_DESC>
{
    size_t operator()(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc) const noexcept
    {
        size_t seed = 0;
        hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.Format)));
        hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.ViewDimension)));
        hash_combine(seed, std::hash<UINT>{}(desc.Shader4ComponentMapping));

        switch (desc.ViewDimension)
        {
        case D3D12_SRV_DIMENSION_BUFFER:
            hash_combine(seed, std::hash<UINT64>{}(desc.Buffer.FirstElement));
            hash_combine(seed, std::hash<UINT>{}(desc.Buffer.NumElements));
            hash_combine(seed, std::hash<UINT>{}(desc.Buffer.StructureByteStride));
            hash_combine(seed, std::hash<UINT64>{}(desc.Buffer.Flags));
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1D:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1D.MostDetailedMip));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1D.MipLevels));
            hash_combine(seed, std::hash<float>{}(desc.Texture1D.ResourceMinLODClamp));
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1DArray.MostDetailedMip));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1DArray.MipLevels));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1DArray.FirstArraySlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1DArray.ArraySize));
            hash_combine(seed, std::hash<float>{}(desc.Texture1DArray.ResourceMinLODClamp));
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2D:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2D.MostDetailedMip));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2D.MipLevels));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2D.PlaneSlice));
            hash_combine(seed, std::hash<float>{}(desc.Texture2D.ResourceMinLODClamp));
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.MostDetailedMip));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.MipLevels));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.FirstArraySlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.ArraySize));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.PlaneSlice));
            hash_combine(seed, std::hash<float>{}(desc.Texture2DArray.ResourceMinLODClamp));
            break;
        case D3D12_SRV_DIMENSION_TEXTURE3D:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture3D.MostDetailedMip));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture3D.MipLevels));
            hash_combine(seed, std::hash<float>{}(desc.Texture3D.ResourceMinLODClamp));
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBE:
            hash_combine(seed, std::hash<UINT>{}(desc.TextureCube.MostDetailedMip));
            hash_combine(seed, std::hash<UINT>{}(desc.TextureCube.MipLevels));
            hash_combine(seed, std::hash<float>{}(desc.TextureCube.ResourceMinLODClamp));
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
            hash_combine(seed, std::hash<UINT>{}(desc.TextureCubeArray.MostDetailedMip));
            hash_combine(seed, std::hash<UINT>{}(desc.TextureCubeArray.MipLevels));
            hash_combine(seed, std::hash<UINT>{}(desc.TextureCubeArray.First2DArrayFace));
            hash_combine(seed, std::hash<UINT>{}(desc.TextureCubeArray.NumCubes));
            hash_combine(seed, std::hash<float>{}(desc.TextureCubeArray.ResourceMinLODClamp));
            break;
        }

        return seed;
    }
};

// Custom equality specialization
bool operator==(const D3D12_SHADER_RESOURCE_VIEW_DESC& lhs, const D3D12_SHADER_RESOURCE_VIEW_DESC& rhs)
{
    if (lhs.Format != rhs.Format ||
        lhs.ViewDimension != rhs.ViewDimension ||
        lhs.Shader4ComponentMapping != rhs.Shader4ComponentMapping)
    {
        return false;
    }

    switch (lhs.ViewDimension)
    {
    case D3D12_SRV_DIMENSION_BUFFER:
        return lhs.Buffer.FirstElement == rhs.Buffer.FirstElement &&
            lhs.Buffer.NumElements == rhs.Buffer.NumElements &&
            lhs.Buffer.StructureByteStride == rhs.Buffer.StructureByteStride &&
            lhs.Buffer.Flags == rhs.Buffer.Flags;
    case D3D12_SRV_DIMENSION_TEXTURE1D:
        return lhs.Texture1D.MostDetailedMip == rhs.Texture1D.MostDetailedMip &&
            lhs.Texture1D.MipLevels == rhs.Texture1D.MipLevels &&
            lhs.Texture1D.ResourceMinLODClamp == rhs.Texture1D.ResourceMinLODClamp;
    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        return lhs.Texture1DArray.MostDetailedMip == rhs.Texture1DArray.MostDetailedMip &&
            lhs.Texture1DArray.MipLevels == rhs.Texture1DArray.MipLevels &&
            lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice &&
            lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize &&
            lhs.Texture1DArray.ResourceMinLODClamp == rhs.Texture1DArray.
                                                          ResourceMinLODClamp;
    case D3D12_SRV_DIMENSION_TEXTURE2D:
        return lhs.Texture2D.MostDetailedMip == rhs.Texture2D.MostDetailedMip &&
            lhs.Texture2D.MipLevels == rhs.Texture2D.MipLevels &&
            lhs.Texture2D.PlaneSlice == rhs.Texture2D.PlaneSlice &&
            lhs.Texture2D.ResourceMinLODClamp == rhs.Texture2D.ResourceMinLODClamp;
    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        return lhs.Texture2DArray.MostDetailedMip == rhs.Texture2DArray.MostDetailedMip &&
            lhs.Texture2DArray.MipLevels == rhs.Texture2DArray.MipLevels &&
            lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice &&
            lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize &&
            lhs.Texture2DArray.PlaneSlice == rhs.Texture2DArray.PlaneSlice &&
            lhs.Texture2DArray.ResourceMinLODClamp == rhs.Texture2DArray.
                                                          ResourceMinLODClamp;
    case D3D12_SRV_DIMENSION_TEXTURE3D:
        return lhs.Texture3D.MostDetailedMip == rhs.Texture3D.MostDetailedMip &&
            lhs.Texture3D.MipLevels == rhs.Texture3D.MipLevels &&
            lhs.Texture3D.ResourceMinLODClamp == rhs.Texture3D.ResourceMinLODClamp;
    case D3D12_SRV_DIMENSION_TEXTURECUBE:
        return lhs.TextureCube.MostDetailedMip == rhs.TextureCube.MostDetailedMip &&
            lhs.TextureCube.MipLevels == rhs.TextureCube.MipLevels &&
            lhs.TextureCube.ResourceMinLODClamp == rhs.TextureCube.ResourceMinLODClamp;
    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
        return
            lhs.TextureCubeArray.MostDetailedMip == rhs.TextureCubeArray.MostDetailedMip &&
            lhs.TextureCubeArray.MipLevels == rhs.TextureCubeArray.MipLevels &&
            lhs.TextureCubeArray.First2DArrayFace == rhs.TextureCubeArray.First2DArrayFace &&
            lhs.TextureCubeArray.NumCubes == rhs.TextureCubeArray.NumCubes &&
            lhs.TextureCubeArray.ResourceMinLODClamp == rhs.TextureCubeArray.ResourceMinLODClamp;
    default:
        return true;
    }
};

// Hash specialization
template <>
struct std::hash<D3D12_UNORDERED_ACCESS_VIEW_DESC>
{
    size_t operator()(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc) const noexcept
    {
        size_t hashValue = std::hash<DXGI_FORMAT>()(desc.Format) ^
            std::hash<int>()(desc.ViewDimension);

        switch (desc.ViewDimension)
        {
        case D3D12_UAV_DIMENSION_BUFFER:
            hashValue ^= std::hash<UINT64>()(desc.Buffer.FirstElement) ^
                std::hash<UINT>()(desc.Buffer.NumElements) ^
                std::hash<UINT>()(desc.Buffer.StructureByteStride) ^
                std::hash<UINT64>()(desc.Buffer.CounterOffsetInBytes) ^
                std::hash<D3D12_BUFFER_UAV_FLAGS>()(desc.Buffer.Flags);
            break;

        case D3D12_UAV_DIMENSION_TEXTURE1D:
            hashValue ^= std::hash<UINT>()(desc.Texture1D.MipSlice);
            break;

        case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
            hashValue ^= std::hash<UINT>()(desc.Texture1DArray.MipSlice) ^
                std::hash<UINT>()(desc.Texture1DArray.FirstArraySlice) ^
                std::hash<UINT>()(desc.Texture1DArray.ArraySize);
            break;

        case D3D12_UAV_DIMENSION_TEXTURE2D:
            hashValue ^= std::hash<UINT>()(desc.Texture2D.MipSlice) ^
                std::hash<UINT>()(desc.Texture2D.PlaneSlice);
            break;

        case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
            hashValue ^= std::hash<UINT>()(desc.Texture2DArray.MipSlice) ^
                std::hash<UINT>()(desc.Texture2DArray.FirstArraySlice) ^
                std::hash<UINT>()(desc.Texture2DArray.ArraySize) ^
                std::hash<UINT>()(desc.Texture2DArray.PlaneSlice);
            break;

        case D3D12_UAV_DIMENSION_TEXTURE3D:
            hashValue ^= std::hash<UINT>()(desc.Texture3D.MipSlice) ^
                std::hash<UINT>()(desc.Texture3D.FirstWSlice) ^
                std::hash<UINT>()(desc.Texture3D.WSize);
            break;

        default:
            break;
        }

        return hashValue;
    }
};

// Equality operator specialization
bool operator==(const D3D12_UNORDERED_ACCESS_VIEW_DESC& lhs, const D3D12_UNORDERED_ACCESS_VIEW_DESC& rhs)
{
    if (lhs.Format != rhs.Format || lhs.ViewDimension != rhs.ViewDimension)
    {
        return false;
    }

    switch (lhs.ViewDimension)
    {
    case D3D12_UAV_DIMENSION_BUFFER:
        return lhs.Buffer.FirstElement == rhs.Buffer.FirstElement &&
            lhs.Buffer.NumElements == rhs.Buffer.NumElements &&
            lhs.Buffer.StructureByteStride == rhs.Buffer.StructureByteStride &&
            lhs.Buffer.CounterOffsetInBytes == rhs.Buffer.CounterOffsetInBytes &&
            lhs.Buffer.Flags == rhs.Buffer.Flags;

    case D3D12_UAV_DIMENSION_TEXTURE1D:
        return lhs.Texture1D.MipSlice == rhs.Texture1D.MipSlice;

    case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
        return lhs.Texture1DArray.MipSlice == rhs.Texture1DArray.MipSlice &&
            lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice &&
            lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize;

    case D3D12_UAV_DIMENSION_TEXTURE2D:
        return lhs.Texture2D.MipSlice == rhs.Texture2D.MipSlice &&
            lhs.Texture2D.PlaneSlice == rhs.Texture2D.PlaneSlice;

    case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
        return lhs.Texture2DArray.MipSlice == rhs.Texture2DArray.MipSlice &&
            lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice &&
            lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize &&
            lhs.Texture2DArray.PlaneSlice == rhs.Texture2DArray.PlaneSlice;

    case D3D12_UAV_DIMENSION_TEXTURE3D:
        return lhs.Texture3D.MipSlice == rhs.Texture3D.MipSlice &&
            lhs.Texture3D.FirstWSlice == rhs.Texture3D.FirstWSlice &&
            lhs.Texture3D.WSize == rhs.Texture3D.WSize;

    default:
        return true;
    }
}

// Hash specialization for D3D12_RENDER_TARGET_VIEW_DESC
template <>
struct std::hash<D3D12_RENDER_TARGET_VIEW_DESC>
{
    std::size_t operator()(const D3D12_RENDER_TARGET_VIEW_DESC& desc) const
    {
        std::size_t seed = 0;
        hash_combine(seed, std::hash<DXGI_FORMAT>{}(desc.Format));
        hash_combine(seed, std::hash<D3D12_RTV_DIMENSION>{}(desc.ViewDimension));

        switch (desc.ViewDimension)
        {
        case D3D12_RTV_DIMENSION_BUFFER:
            hash_combine(seed, std::hash<UINT64>{}(desc.Buffer.FirstElement));
            hash_combine(seed, std::hash<UINT>{}(desc.Buffer.NumElements));
            break;

        case D3D12_RTV_DIMENSION_TEXTURE1D:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1D.MipSlice));
            break;

        case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1DArray.MipSlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1DArray.FirstArraySlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture1DArray.ArraySize));
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2D:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2D.MipSlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2D.PlaneSlice));
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.MipSlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.FirstArraySlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.ArraySize));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DArray.PlaneSlice));
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2DMS:
            // No additional fields to hash
            break;

        case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DMSArray.FirstArraySlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture2DMSArray.ArraySize));
            break;

        case D3D12_RTV_DIMENSION_TEXTURE3D:
            hash_combine(seed, std::hash<UINT>{}(desc.Texture3D.MipSlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture3D.FirstWSlice));
            hash_combine(seed, std::hash<UINT>{}(desc.Texture3D.WSize));
            break;
        }

        return seed;
    }
};

// Equality operator for D3D12_RENDER_TARGET_VIEW_DESC
bool operator==(const D3D12_RENDER_TARGET_VIEW_DESC& lhs, const D3D12_RENDER_TARGET_VIEW_DESC& rhs)
{
    if (lhs.Format != rhs.Format || lhs.ViewDimension != rhs.ViewDimension)
    {
        return false;
    }

    switch (lhs.ViewDimension)
    {
    case D3D12_RTV_DIMENSION_BUFFER:
        return lhs.Buffer.FirstElement == rhs.Buffer.FirstElement &&
            lhs.Buffer.NumElements == rhs.Buffer.NumElements;

    case D3D12_RTV_DIMENSION_TEXTURE1D:
        return lhs.Texture1D.MipSlice == rhs.Texture1D.MipSlice;

    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
        return lhs.Texture1DArray.MipSlice == rhs.Texture1DArray.MipSlice &&
            lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice &&
            lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize;

    case D3D12_RTV_DIMENSION_TEXTURE2D:
        return lhs.Texture2D.MipSlice == rhs.Texture2D.MipSlice &&
            lhs.Texture2D.PlaneSlice == rhs.Texture2D.PlaneSlice;

    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
        return lhs.Texture2DArray.MipSlice == rhs.Texture2DArray.MipSlice &&
            lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice &&
            lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize &&
            lhs.Texture2DArray.PlaneSlice == rhs.Texture2DArray.PlaneSlice;

    case D3D12_RTV_DIMENSION_TEXTURE2DMS:
        return true; // No additional fields to compare

    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
        return lhs.Texture2DMSArray.FirstArraySlice == rhs.Texture2DMSArray.FirstArraySlice &&
            lhs.Texture2DMSArray.ArraySize == rhs.Texture2DMSArray.ArraySize;

    case D3D12_RTV_DIMENSION_TEXTURE3D:
        return lhs.Texture3D.MipSlice == rhs.Texture3D.MipSlice &&
            lhs.Texture3D.FirstWSlice == rhs.Texture3D.FirstWSlice &&
            lhs.Texture3D.WSize == rhs.Texture3D.WSize;

    default:
        return true;
    }
}

// Hash specialization for D3D12_DEPTH_STENCIL_VIEW_DESC
template <>
struct std::hash<D3D12_DEPTH_STENCIL_VIEW_DESC>
{
    size_t operator()(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc) const
    {
        size_t hashValue = 0;

        // Combine the hash of basic members
        hashValue ^= std::hash<DXGI_FORMAT>()(desc.Format) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
        hashValue ^= std::hash<D3D12_DSV_FLAGS>()(desc.Flags) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
        hashValue ^= std::hash<D3D12_DSV_DIMENSION>()(desc.ViewDimension) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);

        // Combine the hash of the union members based on ViewDimension
        switch (desc.ViewDimension)
        {
        case D3D12_DSV_DIMENSION_TEXTURE1D:
            hashValue ^= std::hash<UINT>()(desc.Texture1D.MipSlice) + 0x9e3779b9 + (hashValue << 6) + (
                hashValue >> 2);
            break;

        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
            hashValue ^= std::hash<UINT>()(desc.Texture1DArray.MipSlice) + 0x9e3779b9 + (
                hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<UINT>()(desc.Texture1DArray.FirstArraySlice) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<UINT>()(desc.Texture1DArray.ArraySize) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2D:
            hashValue ^= std::hash<UINT>()(desc.Texture2D.MipSlice) + 0x9e3779b9 + (hashValue << 6) + (
                hashValue >> 2);
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
            hashValue ^= std::hash<UINT>()(desc.Texture2DArray.MipSlice) + 0x9e3779b9 + (
                hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<UINT>()(desc.Texture2DArray.FirstArraySlice) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<UINT>()(desc.Texture2DArray.ArraySize) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2DMS:
            // No additional fields to hash for multisample textures.
            break;

        case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
            hashValue ^= std::hash<UINT>()(desc.Texture2DMSArray.FirstArraySlice) + 0x9e3779b9 +
                (hashValue << 6) + (hashValue >> 2);
            hashValue ^= std::hash<UINT>()(desc.Texture2DMSArray.ArraySize) + 0x9e3779b9 + (hashValue << 6) + (hashValue >> 2);
            break;
        }

        return hashValue;
    }
};

bool operator==(const D3D12_DEPTH_STENCIL_VIEW_DESC& lhs, const D3D12_DEPTH_STENCIL_VIEW_DESC& rhs)
{
    if (lhs.Format != rhs.Format || lhs.ViewDimension != rhs.ViewDimension || lhs.Flags != rhs.Flags)
        return false;

    switch (lhs.ViewDimension)
    {
    case D3D12_DSV_DIMENSION_TEXTURE1D:
        return lhs.Texture1D.MipSlice == rhs.Texture1D.MipSlice;

    case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
        return lhs.Texture1DArray.MipSlice == rhs.Texture1DArray.MipSlice &&
            lhs.Texture1DArray.FirstArraySlice == rhs.Texture1DArray.FirstArraySlice &&
            lhs.Texture1DArray.ArraySize == rhs.Texture1DArray.ArraySize;

    case D3D12_DSV_DIMENSION_TEXTURE2D:
        return lhs.Texture2D.MipSlice == rhs.Texture2D.MipSlice;

    case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
        return lhs.Texture2DArray.MipSlice == rhs.Texture2DArray.MipSlice &&
            lhs.Texture2DArray.FirstArraySlice == rhs.Texture2DArray.FirstArraySlice &&
            lhs.Texture2DArray.ArraySize == rhs.Texture2DArray.ArraySize;

    case D3D12_DSV_DIMENSION_TEXTURE2DMS:
        return true; // No extra fields to compare for TEXTURE2DMS

    case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
        return lhs.Texture2DMSArray.FirstArraySlice == rhs.Texture2DMSArray.FirstArraySlice &&
            lhs.Texture2DMSArray.ArraySize == rhs.Texture2DMSArray.ArraySize;

    default:
        return true;
    }
}

// Specialization of std::hash for D3D12_INDEX_BUFFER_VIEW
template <>
struct std::hash<D3D12_INDEX_BUFFER_VIEW>
{
    size_t operator()(const D3D12_INDEX_BUFFER_VIEW& view) const noexcept
    {
        size_t seed = 0;
        // Combine hash of each field
        seed ^= std::hash<D3D12_GPU_VIRTUAL_ADDRESS>{}(view.BufferLocation) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<UINT>{}(view.SizeInBytes) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<DXGI_FORMAT>{}(view.Format) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

// Equality operator for D3D12_INDEX_BUFFER_VIEW
bool operator==(const D3D12_INDEX_BUFFER_VIEW& lhs, const D3D12_INDEX_BUFFER_VIEW& rhs)
{
    return lhs.BufferLocation == rhs.BufferLocation &&
        lhs.SizeInBytes == rhs.SizeInBytes &&
        lhs.Format == rhs.Format;
}

// Hash specialization for D3D12_VERTEX_BUFFER_VIEW
template <>
struct std::hash<D3D12_VERTEX_BUFFER_VIEW>
{
    size_t operator()(const D3D12_VERTEX_BUFFER_VIEW& view) const noexcept
    {
        size_t h1 = hash<D3D12_GPU_VIRTUAL_ADDRESS>()(view.BufferLocation);
        size_t h2 = hash<UINT>()(view.SizeInBytes);
        size_t h3 = hash<UINT>()(view.StrideInBytes);

        // Combine the hashes
        size_t seed = 0;
        seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};

// Equality operator for D3D12_VERTEX_BUFFER_VIEW
bool operator==(const D3D12_VERTEX_BUFFER_VIEW& lhs, const D3D12_VERTEX_BUFFER_VIEW& rhs)
{
    return lhs.BufferLocation == rhs.BufferLocation &&
        lhs.SizeInBytes == rhs.SizeInBytes &&
        lhs.StrideInBytes == rhs.StrideInBytes;
}

namespace GiiGa
{

    struct DXDeleter
    {
        void operator()(IUnknown* surface)
        {
            //std::cout << "DXDeleter\n";
            if (surface) surface->Release();
        }
    };
    
    class DXDelayedDeleter
    {
    public:
        DXDelayedDeleter(IRenderDevice& device):
            device_(device)
        {
        }

        void operator()(IUnknown* resource)
        {
            //std::cout << "DXDelayedDeleter\n";
            //ID3D12Object* d3d12Object = nullptr;
            //HRESULT hr = resource->QueryInterface(IID_PPV_ARGS(&d3d12Object));
            //if (SUCCEEDED(hr))
            //{
            //    wchar_t name[128] = {};
            //    UINT size = sizeof(name);
            //    d3d12Object->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);
            //    std::wcout << name << "\n";
            //    d3d12Object->Release();
            //}
            device_.KeepAliveForFramesInFlight(unique_any(std::unique_ptr<IUnknown, DXDeleter>(std::move(resource))));
        }

    private:
        IRenderDevice& device_;
    };



     inline void ThrowIfFailed(HRESULT hr)
    {
#ifndef NDEBUG
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::exception();
        }
#endif
    }
}
