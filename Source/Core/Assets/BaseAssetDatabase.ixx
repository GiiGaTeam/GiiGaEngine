module;

#include <unordered_map>
#include <string>
#include <filesystem>
#include <fstream>

export module BaseAssetDatabase;

import AssetHandle;
import AssetMeta;
import Misc;

namespace GiiGa
{
    export class BaseAssetDatabase
    {
    protected:
        std::filesystem::path registry_path_;
        std::ofstream registry_file_;

        // key - AssetHandle, value - Asset meta
        std::unordered_map<AssetHandle, AssetMeta> registry_map_;

    public:
        AssetMeta GetAssetMeta(AssetHandle handle) { 
            Todo<AssetMeta>();
        }
    };
}  // namespace GiiGa
