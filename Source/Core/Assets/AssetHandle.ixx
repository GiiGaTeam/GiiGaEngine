module;

export module AssetHandle;

import Uuid;
import AssetType;

namespace GiiGa
{
    export struct AssetHandle
    {
        Uuid id;
        AssetType type;
    };
}
