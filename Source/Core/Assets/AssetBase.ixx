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

        AssetHandle GetId() const {
            return id_;
        }

        const std::vector<AssetHandle>& Dependencies() const { 
            return dependencies_;
        }

        const std::vector<AssetHandle>& Related() const { 
            return related_; 
        }

        virtual AssetType GetType() = 0;
    };
}  // namespace GiiGa
