module;

#include <vector>
#include <string>

export module AssetMeta;

import AssetHandle;
import AssetType;
import Uuid;
import Misc;

namespace GiiGa
{
    export struct AssetMeta
    {
        Uuid id;
        std::string path;
    };
}  // namespace GiiGa
