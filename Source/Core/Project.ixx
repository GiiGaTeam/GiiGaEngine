export module Project;

import <memory>;
import <filesystem>;
import <fstream>;
import <stdexcept>;
import <iostream>;
import <json/json.h>;

namespace GiiGa
{
    export class Project : public std::enable_shared_from_this<Project>
    {
    private:
        std::filesystem::path project_path_;
        std::filesystem::path default_level_path_;

        Json::Value project_settings_;

        void LoadProjectSettings(const std::filesystem::path& path)
        {
            std::ifstream project_file(path);
            if (!project_file.is_open())
            {
                throw std::runtime_error("Failed to open project file: " + path.string());
            }

            Json::CharReaderBuilder reader_builder;
            std::string errs;

            if (!Json::parseFromStream(reader_builder, project_file, &project_settings_, &errs))
            {
                throw std::runtime_error("Failed to parse project file: " + errs);
            }

            if (project_settings_.isMember("DefaultLevel"))
            {
                default_level_path_ = project_settings_["DefaultLevel"].asString();
            }
            else
            {
                throw std::runtime_error("DefaultLevel is missing in project settings.");
            }
        }

        void OpenProject(const std::filesystem::path& path)
        {
            project_path_ = path;

            auto project_file_path = project_path_ / "project.giga";

            if (!std::filesystem::exists(project_file_path))
            {
                throw std::runtime_error("Project file 'project.giga' not found in: " + project_path_.string());
            }

            LoadProjectSettings(project_file_path);
        }


        Project(const std::filesystem::path& path)
        {
            OpenProject(path);
        }
    public:
        Project() = delete;
        Project(const Project&) = delete;
        Project& operator=(const Project&) = delete;

        Project(Project&& other) noexcept
            : project_path_(std::move(other.project_path_)),
            default_level_path_(std::move(other.default_level_path_)),
            project_settings_(std::move(other.project_settings_))
        {
        }

        Project& operator=(Project&& other) noexcept
        {
            if (this != &other)
            {
                project_path_ = std::move(other.project_path_);
                default_level_path_ = std::move(other.default_level_path_);
                project_settings_ = std::move(other.project_settings_);
            }
            return *this;
        }

        static std::shared_ptr<Project> CreateOrOpen(const std::filesystem::path& path = "")
        {
            if (!path.empty())
            {
                return std::shared_ptr<Project>(new Project(path));
            }
            else
            {
                auto temp_path = std::filesystem::temp_directory_path() / "TemplateProject";

                std::filesystem::path template_path = "TemplateProject";
                if (!std::filesystem::exists(template_path))
                {
                    throw std::runtime_error("Template project folder not found: " + template_path.string());
                }


                std::filesystem::copy(template_path, temp_path, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

                return std::shared_ptr<Project>(new Project(temp_path));
            }
        }

        void SetDefaultLevelPath(const std::string& path)
        {
            default_level_path_ = path;
            project_settings_["DefaultLevelPath"] = default_level_path_.c_str();
        }

        void SaveProjectSettings() const
        {
            auto project_file_path = project_path_ / "project.giga";
            std::ofstream project_file(project_file_path);
            if (!project_file.is_open())
            {
                throw std::runtime_error("Failed to open project file for writing: " + project_file_path.string());
            }

            Json::StreamWriterBuilder writer_builder;
            project_file << Json::writeString(writer_builder, project_settings_);
        }

        const std::filesystem::path& GetDefaultLevelPath() const {
            return default_level_path_;
        };

        const std::filesystem::path& GetProjectPath() const
        {
            return project_path_;
        }
    };
}
