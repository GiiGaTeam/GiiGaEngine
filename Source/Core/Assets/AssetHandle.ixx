module;

export module AssetHandle;

#include <json/json.h>

import Uuid;
import AssetType;
import AssetId;

namespace GiiGa
{
    export struct AssetHandle
    {
        Uuid id = Uuid::Null();
        AssetType type = AssetType::Unknown;

        AssetHandle() {}

        template<typename T>
        AssetHandle(AssetId<T> id) : id(id.id), type(T::GetType())
        {
        }

        bool operator==(const AssetHandle& other) const { 
            return type == other.type && id == other.id;
        }

        Json::Value ToJson() const
        {
            Json::Value json;
            json["id"] = id.ToString();
            json["type"] = AssetTypeToString(type);
            return json;
        }

        static AssetHandle FromJson(const Json::Value& json)
        {
            AssetHandle handle;

            auto uuid = Uuid::FromString(json["id"].asString());

            if (!uuid)
            {
                throw std::invalid_argument("Wrong UUID in json");
            }

            handle.id = *uuid;
            handle.type = StringToAssetType(json["type"].asString());

            return handle;
        }
    };
}

namespace std
{
    template <>
    struct hash<GiiGa::AssetHandle>
    {
        size_t operator()(const GiiGa::AssetHandle& handle) const
        {
            return hash<GiiGa::Uuid>()(handle.id) ^ (hash<int>()(static_cast<int>(handle.type)) << 1);
        }
    };
}  // namespace std
