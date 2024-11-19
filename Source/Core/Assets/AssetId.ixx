module;

export module AssetId;

import Uuid;
import AssetType;

namespace GiiGa
{
    export template<typename T>
    struct AssetId
    {
        Uuid id;

        bool operator==(const AssetId<T>& other) const { 
            return id == other.id; 
        }
    };
}  // namespace GiiGa
