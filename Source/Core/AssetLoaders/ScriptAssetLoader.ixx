module;

#include <pybind11/embed.h>

export module ScriptAssetLoader;

import <vector>;
import <filesystem>;


import AssetHandle;
import AssetMeta;
import AssetLoader;

import ScriptSystem;
import Logger;

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

        virtual std::vector<std::pair<AssetHandle, AssetMeta>> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
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

        //C:\Users\olege\AppData\Local\Temp\TemplateProject\Assets\PyBeh1.py
        std::shared_ptr<AssetBase> Load(AssetHandle handle, const ::std::filesystem::path& script_path) override
        {
            try
            {
                pybind11::module_ c_e = pybind11::module_::import("PyBeh1");
            }catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem():: %v",e.what());
            }
            return std::make_shared<ScriptAsset>(handle);
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
