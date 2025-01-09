module;

#include <pybind11/pybind11.h>;

export module ScriptAsset;

import <vector>;
import <filesystem>;

import AssetBase;
import GPULocalResource;

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
