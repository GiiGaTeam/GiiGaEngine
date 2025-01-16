module;

#include <pybind11/embed.h>

export module ScriptAssetLoader;

import <vector>;
import <filesystem>;


import AssetHandle;
import AssetMeta;
import AssetLoader;

import ScriptAsset;
import Logger;
import ScriptHelpers;

import Misc;

namespace GiiGa
{
    export class ScriptAssetLoader : public AssetLoader
    {
    public:
        ScriptAssetLoader()
        {
            id_ = Uuid::FromString("b2227623-70bb-4b66-bab0-1d92d292a75c").value();
            pattern_ = R"((.+)\.(py))";
            type_ = AssetType::Behaviour;
        }

        virtual ~ScriptAssetLoader() = default;

        virtual std::unordered_map<AssetHandle, AssetMeta> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
        {
            return {
                std::make_pair(AssetHandle{Uuid::New(), 0}, AssetMeta{
                                   type_,
                                   relative_path,
                                   id_,
                                   relative_path.stem().string()
                               })
            };
        }

        std::shared_ptr<AssetBase> Load(AssetHandle handle, const ::std::filesystem::path& script_path) override
        {
            //todo: replace all \ wiht . after Assets
            pybind11::module_ module;

            try
            {
                module = pybind11::module::import(script_path.stem().string().c_str());
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->info("ScriptAssetLoader()::Load error while importing %v", e.what());
                throw std::exception("cant import module");
            }

            std::string name = ScriptHelpers::GetComponentSubclassNameInModule(module);

            if (name.empty())
            {
                el::Loggers::getLogger(LogPyScript)->info("ScriptAssetLoader()::Load error while gather name");
                throw std::exception("cant find subclass name");
            }

            return std::make_shared<ScriptAsset>(handle, module, name);
        }

        void Update(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            auto script_asset = std::dynamic_pointer_cast<ScriptAsset>(asset);            

            script_asset->module_.reload();

            std::string name = ScriptHelpers::GetComponentSubclassNameInModule(script_asset->module_);

            if (name.empty())
            {
                el::Loggers::getLogger(LogPyScript)->info("ScriptAssetLoader()::Load error while gather name");
                throw std::exception("cant find subclass name");
            }
            
            script_asset->user_class_name_ = name.c_str();
        }

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            Todo(); // no support and need to script save
        }

        const char* GetName() override
        {
            return "Script Loader";
        }
    };
}
