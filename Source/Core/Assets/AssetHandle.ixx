module;

export module AssetHandle;

import Uuid;
import AssetType;
import AssetId;

namespace GiiGa
{
    export struct AssetHandle
    {
        Uuid id;
        AssetType type;

        template<typename T>
        AssetHandle(AssetId<T> id) : id(id.id), type(T::GetType())
        {
        }

        bool operator==(const AssetHandle& other) const { 
            return type == other.type && id == other.id;
        }
    };
}

namespace std
{
    template <>
    struct hash<GiiGa::AssetHandle>
    {
        size_t operator()(const GiiGa::AssetHandle& handle) const
        {
            return hash<GiiGa::Uuid>()(handle.id) ^ (hash<int>()(static_cast<int>(handle.type)) << 1);
        }
    };
}  // namespace std
