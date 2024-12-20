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
