module;

#include <vector>
#include <string>
#include <filesystem>
#include <json/json.h>

export module AssetMeta;

import AssetHandle;

namespace GiiGa
{
    export struct AssetMeta
    {
        AssetHandle id;
        std::filesystem::path path;

        std::vector<AssetHandle> dependencies;
        std::vector<AssetHandle> related;

        Json::Value ToJson() const
        {
            Json::Value json;

            //json["id"] = id.ToJson();
            //json["path"] = path.string();

            return json;
        }
    };
}  // namespace GiiGa
