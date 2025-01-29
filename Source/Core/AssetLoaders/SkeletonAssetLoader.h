#pragma once
#include <ozz/base/log.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include "AssetLoader.h"
#include "Engine.h"
#include <ozz/animation/runtime/skeleton.h>

namespace GiiGa
{
    class SkeletonAssetLoader : public AssetLoader
    {
        public:
        SkeletonAssetLoader()
        {
            id_ = Uuid::FromString("9daa1193-92db-4a2b-a54c-aa5748d9424d").value();
            pattern_ = R"(.+)\.fbx_animation_data\skeleton.ozz)";
            type_ = AssetType::Skeleton;
        }

        virtual std::unordered_map<AssetHandle, AssetMeta> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path) {
            return {
                std::make_pair(AssetHandle { Uuid::New(), 0}, AssetMeta{
                    type_,
                    relative_path,
                    id_,
                    relative_path.stem().string()
                    })
            };
        }

        std::shared_ptr<AssetBase> Load(AssetHandle handle, const std::filesystem::path& path) override
        {
            if (!std::filesystem::exists(path))
                throw std::runtime_error("File does not exist: " + path.string());

            auto ad = Engine::Instance().AssetDatabase();
            auto rm = Engine::Instance().ResourceManager();

            std::shared_ptr<ozz::animation::Skeleton> _skeleton;
            LoadSkeleton(reinterpret_cast<const char*>(path.c_str()), _skeleton.get());

            auto skeleton_asset = std::make_shared<Skeleton>(handle, _skeleton);
            for (auto const& dir_entry : std::filesystem::directory_iterator(path.parent_path()))
            {
                if (dir_entry.path().filename() == "skeleton.ozz")
                    continue;
                
                auto handles = ad->GetHandlesByPath(dir_entry.path());
                if (!handles.empty())
                {
                    auto animation = rm->GetAsset<Animation>(std::get<AssetHandle>(handles[0]));
                    skeleton_asset->AddAnimation(animation);
                }
            }
            
            return skeleton_asset;
        }

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving skeletons is not supported.");
        }

        const char* GetName() override {
            return "Skeleton Loader";
        }

        private:
        void LoadSkeleton(const char* _filename, ozz::animation::Skeleton* _skeleton) {
            assert(_filename && _skeleton);
            ozz::log::Out() << "Loading skeleton archive " << _filename << "."
                            << std::endl;
            ozz::io::File file(_filename, "rb");
            if (!file.opened()) {
                ozz::log::Err() << "Failed to open skeleton file " << _filename << "."
                                << std::endl;
                return ;
            }
            ozz::io::IArchive archive(&file);
            if (!archive.TestTag<ozz::animation::Skeleton>()) {
                ozz::log::Err() << "Failed to load skeleton instance from file "
                                << _filename << "." << std::endl;
                return ;
            }

            // Once the tag is validated, reading cannot fail.
            archive >> *_skeleton;
        }
    };
}
