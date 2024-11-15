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
    };
}  // namespace GiiGa
