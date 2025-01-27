#pragma once
#include<unordered_map>
#include<string>

namespace GiiGa
{
    enum class AssetType
    {
        Unknown,
        Mesh,
        SkeletalMesh,
        Texture2D,
        Level,
        Prefab,
        Behaviour,
        Audio,
        Material,
    };

    const char* AssetTypeToStaticString(AssetType type)
    {
        switch (type)
        {
            case AssetType::Mesh: return "Mesh";
            case AssetType::SkeletalMesh: return "SkeletalMesh";
            case AssetType::Texture2D: return "Texture2D";
            case AssetType::Level: return "Level";
            case AssetType::Prefab: return "Prefab";
            case AssetType::Behaviour: return "Behaviour";
            case AssetType::Audio: return "Audio";
        case AssetType::Material: return "Material";
            default: throw std::invalid_argument("Unknown AssetType");
        }
    }

    std::string AssetTypeToString(AssetType type)
    {
        return AssetTypeToStaticString(type);
    }

     AssetType StringToAssetType(const std::string& str)
    {
        static const std::unordered_map<std::string, AssetType> stringToTypeMap = {
            {"Mesh", AssetType::Mesh},
            {"SkeletalMesh", AssetType::SkeletalMesh}, 
            {"Texture2D", AssetType::Texture2D}, 
            {"Level", AssetType::Level},
            {"Prefab", AssetType::Prefab}, 
            {"Behaviour", AssetType::Behaviour}, 
            {"Audio", AssetType::Audio},
            {"Material", AssetType::Material}
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