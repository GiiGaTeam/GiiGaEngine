export module MaterialLoader;

import <filesystem>;
import<memory>;

import AssetLoader;
import GPULocalResource;
import Engine;

namespace GiiGa
{
    export class MaterialLoader : public AssetLoader
    {
    public:
        virtual ~MaterialLoader() = default;

        virtual std::vector<std::pair<AssetHandle, AssetMeta>> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path) {
            return {
                std::make_pair(AssetHandle { Uuid::New(), 0}, AssetMeta{
                    type_,
                    relative_path,
                    id_,
                    relative_path.stem().string()
                    })
            };
        }

        // todo
        ::std::shared_ptr<AssetBase> Load(AssetHandle handle, const ::std::filesystem::path& path) override
        {
            //json = json.parse(path);

            //std::shared_ptr<GPULocalResource> tex1 = Engine::Instance().ResourceManager().GetAsset<GPULocalResource>(Json.gettexture1ID())
            // ...
            // load parameters

            //return;
        }

        // todo
        void Save(AssetBase& asset, ::std::filesystem::path& path) override
        {
            // ToJson logic
        }
    };
}
