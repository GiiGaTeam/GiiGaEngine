module;

export module AssetType;

#include <json/json.h>
#include <unordered_map>
#include <string>

namespace GiiGa
{
    export enum class AssetType
    {
        Unknown,
        Mesh,
        SkeletalMesh,
        Texture2D,
        Scene,
        Prefab,
        Behaviour,
        Audio,
        Material,
    };

    export std::string AssetTypeToString(AssetType type)
    {
        switch (type)
        {
            case AssetType::Mesh: return "Mesh";
            case AssetType::SkeletalMesh: return "SkeletalMesh";
            case AssetType::Texture2D: return "Texture2D";
            case AssetType::Scene: return "Scene";
            case AssetType::Prefab: return "Prefab";
            case AssetType::Behaviour: return "Behaviour";
            case AssetType::Audio: return "Audio";
            default: throw std::invalid_argument("Unknown AssetType");
        }
    }

    export AssetType StringToAssetType(const std::string& str)
    {
        static const std::unordered_map<std::string, AssetType> stringToTypeMap = {
            {"Mesh", AssetType::Mesh},
            {"SkeletalMesh", AssetType::SkeletalMesh}, 
            {"Texture2D", AssetType::Texture2D}, 
            {"Scene", AssetType::Scene},
            {"Prefab", AssetType::Prefab}, 
            {"Behaviour", AssetType::Behaviour}, 
            {"Audio", AssetType::Audio}
        };

        auto it = stringToTypeMap.find(str);
        if (it != stringToTypeMap.end())
        {
            return it->second;
        }
        else
        {
            throw std::invalid_argument("Unknown AssetType string: " + str);
        }
    }
}  // namespace GiiGa