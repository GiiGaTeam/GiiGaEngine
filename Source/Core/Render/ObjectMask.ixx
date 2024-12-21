export module ObjectMask;

export import <bitset>;

import VertexTypes;

namespace GiiGa
{
    constexpr uint16_t BitMaskSize = 64;

    constexpr uint16_t VertexTypeRegionSize = 2;
    using VertexTypeMask = std::bitset<VertexTypeRegionSize>;
    constexpr uint16_t VertexTypeRegionOffset = 0;

    constexpr uint16_t BlendModeRegionSize = 4;
    constexpr uint16_t ShadingModelRegionSize = 4;
    constexpr uint16_t MaterialRegionSize = ShadingModelRegionSize + BlendModeRegionSize;
    constexpr uint16_t MaterialRegionOffset = VertexTypeRegionSize;

    constexpr uint16_t ShadingModelOffset = 0 + MaterialRegionOffset;
    constexpr uint16_t BlendModeOffset = ShadingModelRegionSize + MaterialRegionOffset;

    using ShadingModelMask = std::bitset<ShadingModelRegionSize>;
    using BlendModeMask = std::bitset<BlendModeRegionSize>;

    export enum class VertexTypes
    {
        None = 0,
        VertexPosition = 1,
        VertexPNTBT = 2,
        VertexBoned = 4,
        All = VertexPosition | VertexPNTBT | VertexBoned,
    };
    
    export enum class BlendMode
    {
        None = 0,
        Opaque = 1,
        Masked = 2,
        Translucent = 4
    };

    export enum class ShadingModel
    {
        None = 0,
        DefaultLit = 1,
        Unlit = 2,
        All = DefaultLit | Unlit
    };

    template <typename Enum>
    constexpr auto toUnderlying(Enum e) noexcept
    {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }

    export constexpr BlendMode operator|(BlendMode lhs, BlendMode rhs)
    {
        return static_cast<BlendMode>(toUnderlying(lhs) | toUnderlying(rhs));
    }

    export constexpr ShadingModel operator|(ShadingModel lhs, ShadingModel rhs)
    {
        return static_cast<ShadingModel>(toUnderlying(lhs) | toUnderlying(rhs));
    }

    export constexpr VertexTypes operator|(VertexTypes lhs, VertexTypes rhs)
    {
        return static_cast<VertexTypes>(toUnderlying(lhs) | toUnderlying(rhs));
    }

    /*
     * MaterialRegion: BlendMode | ShadingModel
     * ObjectMask : MaterialRegion | VertexTypeRegion
     */
    export class ObjectMask
    {
    public:
        ObjectMask& SetShadingModel(const ShadingModel& shadingMode)
        {
            SetRegion(ShadingModelMask(static_cast<int>(shadingMode)), BlendModeOffset);
            return *this;
        }

        ObjectMask& SetBlendMode(const BlendMode& blendMode)
        {
            SetRegion(BlendModeMask(static_cast<int>(blendMode)), BlendModeOffset);
            return *this;
        }

        ObjectMask& SetVertexType(const VertexTypes& in_vertex_type)
        {
            SetRegion(VertexTypeMask(static_cast<int>(in_vertex_type)), VertexTypeRegionOffset);
            return *this;
        }

        bool any()
        {
            return mask_.any();
        }

        // ======================== OPERATORS =========================================
        // Bitwise OR operator
        ObjectMask operator|(const ObjectMask& other) const
        {
            ObjectMask result = *this;
            result.mask_ |= other.mask_;
            return result;
        }

        // Bitwise AND operator
        ObjectMask operator&(const ObjectMask& other) const
        {
            ObjectMask result = *this;
            result.mask_ &= other.mask_;
            return result;
        }

        // Bitwise XOR operator
        ObjectMask operator^(const ObjectMask& other) const
        {
            ObjectMask result = *this;
            result.mask_ ^= other.mask_;
            return result;
        }

        // Bitwise NOT operator
        ObjectMask operator~() const
        {
            ObjectMask result = *this;
            result.mask_ = ~result.mask_;
            return result;
        }

        // Bitwise OR assignment
        ObjectMask& operator|=(const ObjectMask& other)
        {
            mask_ |= other.mask_;
            return *this;
        }

        // Bitwise AND assignment
        ObjectMask& operator&=(const ObjectMask& other)
        {
            mask_ &= other.mask_;
            return *this;
        }

        // Bitwise XOR assignment
        ObjectMask& operator^=(const ObjectMask& other)
        {
            mask_ ^= other.mask_;
            return *this;
        }

        // Equality operator
        bool operator==(const ObjectMask& other) const
        {
            return mask_ == other.mask_;
        }

        // Inequality operator
        bool operator!=(const ObjectMask& other) const
        {
            return mask_ != other.mask_;
        }

        std::bitset<BitMaskSize> GetMask() const
        {
            return mask_;
        }

    private:
        std::bitset<BitMaskSize> mask_;

        // Utility function to set a region in a larger bitset
        template <size_t RegionSize>
        void SetRegion(std::bitset<RegionSize> region_mask, size_t offset)
        {
            static_assert(RegionSize <= BitMaskSize, "Region size must be less than or equal to the full size");

            // Clear the target region in the full mask
            std::bitset<BitMaskSize> region_clear_mask = (~(std::bitset<BitMaskSize>((1ULL << RegionSize) - 1) << offset));
            mask_ &= region_clear_mask;

            // Set the new region bits
            std::bitset<BitMaskSize> shifted_region_mask = (std::bitset<BitMaskSize>(region_mask.to_ullong()) << offset);
            mask_ |= shifted_region_mask;
        }
    };
}

export template <>
struct std::hash<GiiGa::ObjectMask>
{
    size_t operator()(const GiiGa::ObjectMask& mask) const noexcept
    {
        // Convert the internal bitset to a hashable value (e.g., an integer or a string)
        return std::hash<std::bitset<GiiGa::BitMaskSize>>{}(mask.GetMask());
    }
};
