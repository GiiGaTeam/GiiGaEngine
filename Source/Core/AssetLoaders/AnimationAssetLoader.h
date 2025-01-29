#pragma once
#include <ozz/base/log.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include "AssetLoader.h"
#include "Engine.h"
#include <ozz/animation/runtime/animation.h>

#include "Animation.h"

namespace GiiGa
{
    class AnimationAssetLoader : public AssetLoader
    {
        public:
        AnimationAssetLoader()
        {
            id_ = Uuid::FromString("6a9122df-77a6-45c9-a052-7a85685a2f43").value();
            pattern_ = R"(.+)\.fbx_animation_data\(.+)_animation.ozz)";
            type_ = AssetType::Animation;
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

            auto rs = Engine::Instance().RenderSystem();
            auto device = rs->GetRenderDevice().GetDxDevice();

            std::shared_ptr<ozz::animation::Animation> animation;
            LoadAnimation(reinterpret_cast<const char*>(path.c_str()), animation.get());

            auto animation_asset = std::make_shared<Animation>(handle/*, скелетик сюдааа???*/, animation);
            return animation_asset;
        }

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            throw std::runtime_error("Saving animations is not supported.");
        }

        const char* GetName() override {
            return "Animation Loader";
        }

        private:
        void
        LoadAnimation(const char* _filename,
                   ozz::animation::Animation* _animation) {
            assert(_filename && _animation);
            ozz::log::Out() << "Loading animation archive: " << _filename << "."
                            << std::endl;
            ozz::io::File file(_filename, "rb");
            if (!file.opened()) {
                ozz::log::Err() << "Failed to open animation file " << _filename << "."
                                << std::endl;
                return ;
            }
            ozz::io::IArchive archive(&file);
            if (!archive.TestTag<ozz::animation::Animation>()) {
                ozz::log::Err() << "Failed to load animation instance from file "
                                << _filename << "." << std::endl;
                return ;
            }

            // Once the tag is validated, reading cannot fail.
            archive >> *_animation;
        }
    };
}
