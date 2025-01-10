module;

#include<pybind11/embed.h>

export module ScriptAsset;

import AssetBase;

namespace GiiGa
{
    export class ScriptAsset : public AssetBase
    {
    public:
        ScriptAsset(AssetHandle assetHandle): AssetBase(assetHandle)
        {
        }

        AssetType GetType() override
        {
            return AssetType::Behaviour;
        }

    private:
        pybind11::module_ module;
    };
}
