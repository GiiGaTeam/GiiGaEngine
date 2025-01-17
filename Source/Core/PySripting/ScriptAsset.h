#pragma once
#include<pybind11/embed.h>


#include<memory>

#include<AssetBase.h>
#include<Logger.h>
#include<PyBehaviourTrampoline.h>
#include<ScriptPropertyModifications.h>
#include<ScriptHelpers.h>

namespace GiiGa
{
    class ScriptAsset : public AssetBase
    {
        friend class ScriptAssetLoader;

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

        std::shared_ptr<PyBehaviourTrampoline> CreateBehaviourComponent(const Uuid& uuid) const
        {
            try
            {
                auto obj = module_.attr(user_class_name_.c_str())();
                auto beh = pybind11::cast<std::shared_ptr<PyBehaviourTrampoline>>(obj);
                beh->uuid_ = uuid;
                return beh;
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptAsset()::CreateBehaviourComponent %v", e.what());
            }

            return {};
        }

        // pair is type and default value
        std::unordered_map<std::string, PyProperty> GetPropertyAnnotaions()
        {
            std::unordered_map<std::string, PyProperty> result;
            auto py_prop = pybind11::cast<pybind11::dict>(module_.attr(user_class_name_.c_str()).attr("__annotations__"));
            for (auto item : py_prop)
            {
                std::string name = pybind11::cast<std::string>(item.first);
                pybind11::type type = pybind11::cast<pybind11::type>(item.second);
                pybind11::object value = module_.attr(user_class_name_.c_str()).attr(name.c_str());
                result.insert({name, {.script_type = type, .value_or_holder = value}});
            }

            return result;
        }

        std::string GetUserClassName() const
        {
            return user_class_name_;
        }

        pybind11::object GetUnderlyingType()const
        {
            try
            {
                return ScriptHelpers::GetTypeWithNameInModule(module_, user_class_name_);
            }
            catch (pybind11::error_already_set& e)
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
