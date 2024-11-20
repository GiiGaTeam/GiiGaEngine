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

            json["id"] = id.ToJson();
            json["path"] = path.string();

            for (const auto& dep : dependencies) {
                json["dependencies"].append(dep.ToJson());
            }

            for (const auto& rel : related) {
                json["related"].append(rel.ToJson());
            }

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

            if (json.isMember("dependencies") && json["dependencies"].isArray())
            {
                for (const auto& depJson : json["dependencies"])
                {
                    meta.dependencies.push_back(AssetHandle::FromJson(depJson));
                }
            }

            if (json.isMember("related") && json["related"].isArray())
            {
                for (const auto& relJson : json["related"])
                {
                    meta.related.push_back(AssetHandle::FromJson(relJson));
                }
            }

            return meta;
        }
    };
}  // namespace GiiGa
