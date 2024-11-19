module;

#include <vector>
#include <string>

export module AssetMeta;

import AssetHandle;

namespace GiiGa
{
    export struct AssetMeta
    {
        AssetHandle id;
        std::string path;

        std::vector<AssetHandle> dependencies;
        std::vector<AssetHandle> related;
    };
}  // namespace GiiGa
