export module AssetMeta;

import <vector>;
import <string>;
import <filesystem>;
import <json/json.h>;

import AssetHandle;
import Uuid;

namespace GiiGa
{
    export struct AssetMeta
    {
        AssetType type;
        std::filesystem::path path;
        Uuid loader_id = Uuid::Null();

        Json::Value ToJson() const
        {
            Json::Value json;

            json["path"] = path.string();
            json["type"] = AssetTypeToString(type);
            json["loader_id"] = loader_id.ToString();

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

            if (!json.isMember("loader_id") || !json["loader_id"].isString())
            {
                throw std::invalid_argument("Invalid JSON: Missing or invalid 'loader_id'");
            }

            AssetMeta meta;

            meta.path = json["path"].asString();
            meta.type = StringToAssetType(json["type"].asString());
            meta.loader_id = Uuid::FromString(json["loader_id"].asString()).value_or(Uuid::Null());

            return meta;
        }
    };
}  // namespace GiiGa
