module;

#include <vector>

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
        Uuid id_;
        std::vector<AssetHandle> dependencies_;
        std::vector<AssetHandle> related_;

    public:
        void AddDependency(AssetHandle dependency) { 
            Todo<void>();
        }

        void RemoveDependency(AssetHandle dependency) {
            Todo<void>();
        }

        virtual void Save() = 0;
        virtual AssetType GetType() = 0;
    };
}  // namespace GiiGa
