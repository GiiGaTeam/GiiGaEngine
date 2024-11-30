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

        Json::Value ToJson() const
        {
            Json::Value json;

            json["id"] = id.ToJson();
            json["path"] = path.string();

            return json;
        }

        static AssetMeta FromJson(const Json::Value& json) {
            if (!json.isObject())
            {
                throw std::invalid_argument("Invalid JSON: Expected an object");
            }

            if (!json.isMember("path") || !json["path"].isString())
            {
                throw std::invalid_argument("Invalid JSON: Missing or invalid 'path'");
            }

            AssetMeta meta;

            meta.id = AssetHandle::FromJson(json["id"]);
            meta.path = json["path"].asString();

            return meta;
        }
    };
}  // namespace GiiGa
