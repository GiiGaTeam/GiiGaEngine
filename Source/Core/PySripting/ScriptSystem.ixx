module;

#include <pybind11/embed.h>

export module ScriptSystem;

import <format>;

import AssetBase;
import Project;
import Logger;

namespace GiiGa
{
    export class ScriptSystem
    {
    public:
        //C:\Users\olege\AppData\Local\Temp\TemplateProject
        ScriptSystem(std::shared_ptr<Project> project)
        {
            PyConfig conf;
            pybind11::initialize_interpreter();

            // add sys
            {
                std::string ScriptsPath = (project->GetProjectPath() / "Assets").string();

                auto pyAddSys = std::format(R"(
import sys
sys.path.append(r'{}'))", ScriptsPath.c_str());

                try
                {
                    pybind11::exec(pyAddSys);
                }
                catch (pybind11::error_already_set e)
                {
                    el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem():: %v", e.what());
                }
            }

            try
            {
                engine_module = pybind11::module_::import("GiiGaPy");
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem():: %v", e.what());
            }
        }

        ~ScriptSystem()
        {
            try
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem()::Engine module ref count %v", engine_module.ref_count());
                engine_module.dec_ref();
                pybind11::finalize_interpreter();
            }
            catch (pybind11::error_already_set e)
            {
                el::Loggers::getLogger(LogPyScript)->debug("ScriptSystem():: %v", e.what());
            }
        }

    private:
        pybind11::module_ engine_module;
    };
}
