﻿#pragma once

#include<AssetBase.h>

namespace GiiGa
{
    class LevelAsset : public AssetBase
    {
        friend class Level;
        friend class LevelAssetLoader;
    public:
        LevelAsset(const AssetHandle& handle, const Json::Value& level_json):
            AssetBase(handle),
            json_(level_json)
        {
        }

        AssetType GetType() override
        {
            return AssetType::Level;
        }

        std::string GetLevelName()const
        {
            return json_["LevelSettings"]["Name"].asString();
        }

        void SetLevelName(const std::string& level_name)
        {
            json_["LevelSettings"]["Name"] = level_name;
        }

    private:
        Json::Value json_;
    };
}
