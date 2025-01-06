export module PrefabInstance;

import <string>;
import <variant>;
import <unordered_map>;
import <unordered_set>;
import <json/json.h>;

import Uuid;
import AssetHandle;

namespace GiiGa
{
    export using PropertyModificationKey = std::pair<Uuid, std::string>;

    // idk why but std not work
    struct PropertyModificationKeyHash
    {
        std::size_t operator()(const PropertyModificationKey& key) const
        {
            auto hash1 = std::hash<Uuid>{}(key.first);
            auto hash2 = std::hash<std::string>{}(key.second);
            return hash1 ^ (hash2 << 1); // Combine the hashes
        }
    };

    export using PropertyValue = Json::Value;

    export using PropertyModifications = std::unordered_map<PropertyModificationKey, PropertyValue, PropertyModificationKeyHash>;

    export struct PrefabInstanceModifications
    {
        std::unordered_map<Uuid, Uuid> InPrefabUuid_to_Instance;
        PropertyModifications PropertyModifications;
        std::unordered_set<Uuid> Removed_GOs_Comps;
        // todo: no support for this
        //std::unordered_map<Uuid, Json::Value> Added_GOs;
        //std::unordered_map<Uuid, Json::Value> Added_Comps;

        PrefabInstanceModifications() = default;

        PrefabInstanceModifications(Json::Value js)
        {
            for (const auto& map : js["InPrefabUuid_to_Instance"])
            {
                Uuid target = Uuid::FromString(map["Target"].asString()).value();
                Uuid value = Uuid::FromString(map["Value"].asString()).value();
                InPrefabUuid_to_Instance.insert({target, value});
            }

            for (const auto& mod : js["PropertyModifications"])
            {
                Uuid Target = Uuid::FromString(mod["Target"].asString()).value();
                std::string prop_name = mod["PropertyName"].asString();
                Json::Value prop_val = mod["PropertyValue"];
                PropertyModifications.insert({{Target, prop_name}, prop_val});
            }

            for (const auto& removed_js : js["Removed_GOs_Comps"])
            {
                Uuid removed = Uuid::FromString(removed_js.asString()).value();
                Removed_GOs_Comps.insert(removed);
            }
        }

        Json::Value ToJson() const
        {
            Json::Value result;

            for (const auto& map : InPrefabUuid_to_Instance)
            {
                Json::Value map_js;
                map_js["Target"] = map.first.ToString();
                map_js["Value"] = map.second.ToString();

                result["InPrefabUuid_to_Instance"].append(map_js);
            }

            for (const auto& values : PropertyModifications)
            {
                Json::Value modification;
                modification["Target"] = values.first.first.ToString();
                modification["PropertyName"] = values.first.second;
                modification["PropertyValue"] = values.second;

                result["PropertyModifications"].append(modification);
            }

            for (const auto& removed : Removed_GOs_Comps)
            {
                Json::Value removed_js = removed.ToString();

                result["Removed_GOs_Comps"].append(removed_js);
            }

            return result;
        }
    };
}
