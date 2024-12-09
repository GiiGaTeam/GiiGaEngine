module;

#include <vector>
#include <filesystem>

export module AssetBase;

import Engine;

export import AssetHandle;
export import Uuid;
import Misc;

namespace GiiGa
{
    export class AssetBase
    {
    private:
        AssetHandle id_;

    public:
        AssetBase()
        {

        }

        AssetBase(AssetHandle uuid)
            : id_(uuid)
        {
        }

        AssetHandle GetId() const {
            return id_;
        }

        void SetId(AssetHandle id) {
            id_ = id;
        }

        virtual AssetType GetType() = 0;

        virtual ~AssetBase() { 
            Engine::Instance().ResourceManager().RemoveAsset(id_);
        }
    };
}  // namespace GiiGa
