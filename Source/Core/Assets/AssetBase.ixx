module;

#include <vector>
#include <filesystem>

export module AssetBase;

import Engine;

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

    public:
        AssetBase(AssetHandle uuid)
            : id_(uuid)
        {
        }

        AssetHandle GetId() const {
            return id_;
        }

        virtual AssetType GetType() = 0;

        virtual ~AssetBase() { 
            Engine::Instance().ResourceManager().RemoveAsset(id_);
        }
    };
}  // namespace GiiGa
