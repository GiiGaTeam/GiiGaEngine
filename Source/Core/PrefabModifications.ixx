export module PrefabModifications;

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

    export struct PrefabModifications
    {
        std::unordered_map<PropertyModificationKey, PropertyValue, PropertyModificationKeyHash> Modifications;
        std::unordered_set<Uuid> Removed_GOs_Comps;
        // todo: no support for this
        //std::unordered_map<Uuid, Json::Value> Added_GOs;   // parent to GO
        //std::unordered_map<Uuid, Json::Value> Added_Comps; // owner to Comp

        PrefabModifications() = default;

        PrefabModifications(Json::Value js)
        {
            for (const auto& mod : js["Modifications"])
            {
                Uuid Target = Uuid::FromString(mod["Target"].asString()).value();
                std::string prop_name = mod["PropertyName"].asString();
                Json::Value prop_val = mod["PropertyValue"];
                Modifications.insert({{Target, prop_name}, prop_val});
            }
        }

        Json::Value ToJson() const
        {
            Json::Value result;

            for (const auto& values : Modifications)
            {
                Json::Value modification;
                modification["Target"] = values.first.first.ToString();
                modification["PropertyName"] = values.first.second;
                modification["PropertyValue"] = values.second;

                result["Modifications"].append(modification);
            }

            return result;
        }
    };
}
