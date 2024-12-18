module;

#include "FileWatch.hpp"
#include <filesystem>
#include <string>
#include <chrono>

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

        // Kostili because FileWatch is shit
        std::optional<std::filesystem::path> old_rename_path_;
        std::unordered_map<std::filesystem::path, std::chrono::steady_clock::time_point> removed_files_;

        bool CheckForMoves(const std::filesystem::path& path) {
            auto now = std::chrono::steady_clock::now();
            std::vector<std::filesystem::path> to_remove;
            std::vector<std::filesystem::path> to_remove_with_cb;
            bool flag = false;

            for (const auto& [removed_path, timestamp] : removed_files_) {
                if (now - timestamp > std::chrono::milliseconds(500)) {
                    to_remove_with_cb.push_back(removed_path);
                } else if (removed_path.filename() == path.filename()) {
                    OnFileMoved.Invoke(std::make_tuple(removed_path, path));
                    to_remove.push_back(removed_path);
                    flag = true;
                }
            }

            for (const auto& removed_path : to_remove_with_cb) {
                OnFileRemoved.Invoke(removed_path);
                removed_files_.erase(removed_path);
            }

            for (const auto& removed_path : to_remove) {
                removed_files_.erase(removed_path);
            }

            return flag;
        }

    public:
        ProjectWatcher(std::vector<std::string>&& dirs_to_watch) 
            : dirs_to_watch_(dirs_to_watch)
        {}

        EventDispatcher<std::filesystem::path> OnFileAdded;
        EventDispatcher<std::filesystem::path> OnFileRemoved;
        EventDispatcher<std::filesystem::path> OnFileModified;
        EventDispatcher<std::tuple<std::filesystem::path, std::filesystem::path>> OnFileRenamed;
        EventDispatcher<std::tuple<std::filesystem::path, std::filesystem::path>> OnFileMoved;

        void StartWatch() { 
            for (const auto& dir : dirs_to_watch_)
            {
                std::cout << "[DEBUG] Starting watch " << dir << std::endl;
                watchers_.emplace_back(std::make_unique<filewatch::FileWatch<std::string>>(dir,
                    [this](const std::string& path, const filewatch::Event change_type)
                    {
                        switch (change_type)
                        {
                            case filewatch::Event::added: {
                                if (!CheckForMoves(path)) {
                                    OnFileAdded.Invoke(path);
                                }
                                break;
                            }
                            case filewatch::Event::removed: {
                                removed_files_[path] = std::chrono::steady_clock::now();
                                break; 
                            }
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

        void ClearRemovedFiles() {
            for (const auto& removed_path : removed_files_) {
                OnFileRemoved.Invoke(removed_path.first);
            }
            removed_files_.clear();
        }
    };
}  // namespace GiiGa
