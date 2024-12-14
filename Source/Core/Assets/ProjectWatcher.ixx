module;

#include "FileWatch.hpp"
#include <filesystem>
#include <string>

#include <iostream>

export module ProjectWatcher;

import EventSystem;

namespace GiiGa
{
    export class ProjectWatcher
    {
    private:
        std::vector<std::string> dirs_to_watch_;
        std::vector<std::unique_ptr<filewatch::FileWatch<std::string>>> watchers_;

        std::optional<std::filesystem::path> old_rename_path_;
    public:
        ProjectWatcher(std::vector<std::string>&& dirs_to_watch) 
            : dirs_to_watch_(dirs_to_watch)
        {}

        EventDispatcher<std::filesystem::path> OnFileAdded;
        EventDispatcher<std::filesystem::path> OnFileRemoved;
        EventDispatcher<std::filesystem::path> OnFileModified;
        EventDispatcher<std::tuple<std::filesystem::path, std::filesystem::path>> OnFileRenamed;

        void StartWatch() { 
            for (const auto& dir : dirs_to_watch_)
            {
                std::cout << "Starting watch " << dir << std::endl;
                watchers_.emplace_back(std::make_unique<filewatch::FileWatch<std::string>>(dir,
                    [this](const std::string& path, const filewatch::Event change_type)
                    {
                        switch (change_type)
                        {
                            case filewatch::Event::added: OnFileAdded.Invoke(path); break;
                            case filewatch::Event::removed: OnFileRemoved.Invoke(path); break;
                            case filewatch::Event::modified: OnFileModified.Invoke(path); break;
                            case filewatch::Event::renamed_old:
                                old_rename_path_ = path;
                                break;
                            case filewatch::Event::renamed_new:
                                if (old_rename_path_)
                                {
                                    OnFileRenamed.Invoke({*old_rename_path_, path});
                                    old_rename_path_.reset();
                                }
                                break;
                        }
                    }));
            }
        }
    };
}  // namespace GiiGa
