module;

#include <vector>
#include <filesystem>

export module AssetBase;

import AssetHandle;
import AssetType; 
import Uuid;
import Misc;

namespace GiiGa
{
    export class AssetBase
    {
    private:
        AssetHandle id_;
        std::vector<AssetHandle> dependencies_;
        std::vector<AssetHandle> related_;

    public:
        AssetBase(AssetHandle uuid)
            : id_(uuid)
        {
        }

        virtual AssetType GetType() = 0;
    };
}  // namespace GiiGa
