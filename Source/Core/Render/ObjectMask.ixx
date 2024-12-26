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

    constexpr uint16_t FillModeRegionSize = 2;
    constexpr uint16_t FillModeOffset = MaterialRegionSize + MaterialRegionOffset;

    constexpr uint16_t LigthTypeRegionSize = 2;
    constexpr uint16_t LigthTypeOffset = FillModeRegionSize + FillModeOffset;

    using ShadingModelMask = std::bitset<ShadingModelRegionSize>;
    using BlendModeMask = std::bitset<BlendModeRegionSize>;
    using FillModeMask = std::bitset<FillModeRegionSize>;
    using LigthTypeMask = std::bitset<FillModeRegionSize>;

    export enum class VertexTypes
    {
        None = 0,
        VertexPosition = 1,
        VertexPNTBT = 2,
        VertexBoned = 4,
        All = VertexPosition | VertexPNTBT | VertexBoned,
    };

    export enum class FillMode
    {
        None = 0,
        Solid = 1,
        Wire = 2,
        All = Solid | Wire,
    };

    export enum class LightType
    {
        None = 0,
        Point = 1,
        Direction = 2,
        Spot = 4,
        All = Point | Direction | Spot,
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
        // Sets the shading model, ensuring valid input
        ObjectMask& SetShadingModel(ShadingModel shadingModel)
        {
            SetRegion(ToBitset<ShadingModelMask>(shadingModel), ShadingModelOffset);
            return *this;
        }

        // Sets the blend mode, ensuring valid input
        ObjectMask& SetBlendMode(BlendMode blendMode)
        {
            SetRegion(ToBitset<BlendModeMask>(blendMode), BlendModeOffset);
            return *this;
        }

        ObjectMask& SetFillMode(FillMode fillMode)
        {
            SetRegion(ToBitset<FillModeMask>(fillMode), FillModeOffset);
            return *this;
        }

        ObjectMask& SetLightType(LightType light)
        {
            SetRegion(ToBitset<LigthTypeMask>(light), LigthTypeOffset);
            return *this;
        }

        // Sets the vertex type, ensuring valid input
        ObjectMask& SetVertexType(VertexTypes vertexType)
        {
            SetRegion(ToBitset<VertexTypeMask>(vertexType), VertexTypeRegionOffset);
            return *this;
        }

        // Gets the current blend mode
        BlendMode GetBlendMode() const
        {
            auto t = GetRegion<BlendModeRegionSize>(BlendModeOffset);
            return FromBitset<BlendModeMask, BlendMode>(t);
        }

        // Gets the current shading model
        ShadingModel GetShadingModel() const
        {
            auto t = GetRegion<ShadingModelRegionSize>(ShadingModelOffset);
            return FromBitset<ShadingModelMask, ShadingModel>(t);
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

        // Converts an enum to a bitset
        template <typename BitMask, typename Enum>
        static BitMask ToBitset(Enum value)
        {
            return BitMask(static_cast<int>(value));
        }

        // Converts a bitset back to an enum
        template <typename BitMask, typename Enum>
        static Enum FromBitset(const BitMask& bitset)
        {
            return static_cast<Enum>(bitset.to_ulong());
        }

        // Sets a specific region in the mask
        template <size_t RegionSize>
        void SetRegion(std::bitset<RegionSize> regionMask, size_t offset)
        {
            static_assert(RegionSize <= BitMaskSize, "Region size must be less than or equal to the full size");

            // Clear the target region
            mask_ &= ~(std::bitset<BitMaskSize>((1ULL << RegionSize) - 1) << offset);

            // Set the new region bits
            mask_ |= (std::bitset<BitMaskSize>(regionMask.to_ullong()) << offset);
        }

        // Gets a specific region from the mask
        template <size_t RegionSize>
        std::bitset<RegionSize> GetRegion(size_t offset) const
        {
            static_assert(RegionSize <= BitMaskSize, "Region size must be less than or equal to the full size");

            // Extract the region and convert it to the correct bitset size
            auto extracted = (mask_ >> offset) & std::bitset<BitMaskSize>((1ULL << RegionSize) - 1);
            return std::bitset<RegionSize>(extracted.to_ullong());
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
