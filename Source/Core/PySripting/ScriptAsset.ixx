module;

#include<pybind11/embed.h>

export module ScriptAsset;

import <memory>;

import AssetBase;
import Logger;
import PyBehaviourTrampoline;

namespace GiiGa
{
    export class ScriptAsset : public AssetBase
    {
    public:
        ScriptAsset(AssetHandle assetHandle, const std::string& module_name): AssetBase(assetHandle)
        {
            try
            {
                module = pybind11::module_::import(module_name.c_str());
                el::Loggers::getLogger(LogPyScript)->debug("ScriptAsset():: Imported %v", module_name.c_str());
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptAsset()::Error %v", e.what());
            }
        }

        AssetType GetType() override
        {
            return AssetType::Behaviour;
        }

        std::shared_ptr<PyBehaviourTrampoline> CreateBehaviourComponent() const
        {
            //todo temp MyPyBeh1
            try
            {
                auto obj = module.attr("MyPyBeh1")();
                return pybind11::cast<std::shared_ptr<PyBehaviourTrampoline>>(obj);
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptAsset()::CreateBehaviourComponent %v", e.what());
            }

            return {};
        }

    private:
        pybind11::module_ module;
    };
}
