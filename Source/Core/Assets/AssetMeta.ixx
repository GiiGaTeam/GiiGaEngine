export module AssetMeta;

import <vector>;
import <string>;
import <filesystem>;
import <json/json.h>;

import AssetHandle;

namespace GiiGa
{
    export struct AssetMeta
    {
        AssetType type;
        std::filesystem::path path;

        Json::Value ToJson() const
        {
            Json::Value json;

            json["path"] = path.string();
            json["type"] = AssetTypeToString(type);

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

            if (!json.isMember("type") || !json["type"].isString())
            {
                throw std::invalid_argument("Invalid JSON: Missing or invalid 'type'");
            }

            AssetMeta meta;

            meta.path = json["path"].asString();
            meta.type = StringToAssetType(json["type"].asString());

            return meta;
        }
    };
}  // namespace GiiGa
