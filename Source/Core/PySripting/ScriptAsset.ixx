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
        ScriptAsset(AssetHandle assetHandle, const pybind11::module_ module, const std::string user_class_name):
            AssetBase(assetHandle)
            , module_(module)
            , user_class_name_(user_class_name)
        {
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
                auto obj = module_.attr(user_class_name_.c_str())();
                return pybind11::cast<std::shared_ptr<PyBehaviourTrampoline>>(obj);
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptAsset()::CreateBehaviourComponent %v", e.what());
            }

            return {};
        }

    private:
        pybind11::module_ module_;
        std::string user_class_name_;
    };
}
