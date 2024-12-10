module;

#include <vector>
#include <filesystem>

export module AssetBase;

export import AssetHandle;
export import Uuid;
import Misc;
import EventSystem;

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

        EventDispatcher<AssetHandle> OnDestroy;

        virtual ~AssetBase() { 
            OnDestroy.Invoke(id_);
        }
    };
}  // namespace GiiGa
