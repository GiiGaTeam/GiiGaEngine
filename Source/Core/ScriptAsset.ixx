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
    };
}
