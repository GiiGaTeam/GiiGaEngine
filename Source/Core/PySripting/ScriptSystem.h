#pragma once
#include <pybind11/embed.h>


#include<format>
#include<filesystem>

#include<AssetBase.h>
#include<Project.h>
#include<Logger.h>
#include<ScriptHelpers.h>

namespace GiiGa
{
    class ScriptSystem
    {
    public:
        //C:\Users\olege\AppData\Local\Temp\TemplateProject
        ScriptSystem(std::shared_ptr<Project> project)
        {
            PyConfig conf;
            pybind11::initialize_interpreter();

            // add user scripts to sys
            {
                std::string ScriptsPath = (project->GetProjectPath() / "Assets").string();

                auto pyAddSys = std::format(R"(
#include<sys
sys.path.append(r'{}'))", ScriptsPath.c_str());

                try
                {
                    pybind11::exec(pyAddSys);
                }
                catch (pybind11::error_already_set& e)
                {
                    el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem():: %v", e.what());
                }
            }

            // add helper scripts to sys
            {
                pybind11::exec(R"(
#include<os
#include<sys
sys.path.append(os.getcwd() + '/EditorData/ScriptHelpers'))");
            }

#ifdef _DEBUG
            try
            {
                if (!std::filesystem::exists("./EditorData/ScriptHelpers/GiiGaPy.pyi"))
                    pybind11::module::import("sgen").attr("generate")("./EditorData/ScriptHelpers");
            }
            catch
            (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem():: %v", e.what());
            }
#endif
            

            reference_types_holders =
            {
                {pybind11::module::import("GiiGaPy").attr("Component"), pybind11::module::import("GiiGaPy").attr("Uuid")},
                {pybind11::module::import("GiiGaPy").attr("GameObject"), pybind11::module::import("GiiGaPy").attr("Uuid")}
            };
        }

        std::optional<pybind11::type> GetReferenceTypeHolder(const pybind11::type& type)
        {
            for (auto ref_holder : reference_types_holders)
            {
                if (ScriptHelpers::IsEqOrSubClass(type, ref_holder.first))
                {
                    return ref_holder.second;
                }
            }

            return std::nullopt;
        }

        bool IsTypeReference(const pybind11::type& type)
        {
            for (auto ref_holder : reference_types_holders)
            {
                if (ScriptHelpers::IsEqOrSubClass(type, ref_holder.first))
                {
                    return true;
                }
            }
            return false;
        }

        bool IsTypeGameObject(const pybind11::type& type)
        {
            return type.is(pybind11::module::import("GiiGaPy").attr("GameObject"));
        }

        bool IsTypeComponent(const pybind11::type& type)
        {
            return ScriptHelpers::IsEqOrSubClass(type, pybind11::module::import("GiiGaPy").attr("Component"));
        }

        ~ScriptSystem()
        {
            try
            {
                pybind11::finalize_interpreter();
            }
            catch (pybind11::error_already_set& e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem():: %v", e.what());
            }
        }

        std::vector<std::pair<pybind11::type, pybind11::type>> reference_types_holders{};
    };
}
