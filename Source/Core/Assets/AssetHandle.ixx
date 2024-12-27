export module AssetHandle;

import <json/json.h>;

export import Uuid;
export import AssetType;

namespace GiiGa
{
    export struct AssetHandle
    {
        Uuid id = Uuid::Null();
        int subresource = 0;

        AssetHandle()
        {
        }

        AssetHandle(Uuid id, int subresource) : id(id), subresource(subresource)
        {
        }

        bool operator==(const AssetHandle& other) const
        {
            return id == other.id && subresource == other.subresource;
        }

        bool operator!=(const AssetHandle& other) const
        {
            return !(*this == other);
        }

        Json::Value ToJson() const
        {
            Json::Value json;
            json["id"] = id.ToString();
            json["subresource"] = subresource;
            return json;
        }

        static AssetHandle FromJson(const Json::Value& json)
        {
            AssetHandle handle;

            auto uuid = Uuid::FromString(json["id"].asString());

            if (!uuid)
            {
                throw std::invalid_argument("Invalid JSON: Wrong UUID");
            }

            handle.id = *uuid;
            handle.subresource = json["subresource"].asInt();

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
            return hash<GiiGa::Uuid>()(handle.id) ^ (hash<int>()(handle.subresource));
        }
    };
} // namespace std
