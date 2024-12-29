export module MaterialLoader;

import <filesystem>;
import<memory>;
import <json/json.h>;

import AssetLoader;
import GPULocalResource;
import Engine;
import Material;
import TextureAsset;
import Logger;
import AssetMeta;
import Misc;

namespace GiiGa
{
    export class MaterialLoader : public AssetLoader
    {
    public:
        MaterialLoader()
        {
            id_ = Uuid::FromString("a2227623-70bb-4b66-bab0-1d92d292a75c").value();
            pattern_ = R"((.+)\.(mat))";
            type_ = AssetType::Material;
        }

        virtual ~MaterialLoader() = default;

        virtual std::vector<std::pair<AssetHandle, AssetMeta>> Preprocess(const std::filesystem::path& absolute_path, const std::filesystem::path& relative_path)
        {
            return {
                std::make_pair(AssetHandle{Uuid::New(), 0}, AssetMeta{
                                   type_,
                                   relative_path,
                                   id_,
                                   relative_path.stem().string()
                               })
            };
        }

        /* Material Json Structure:
         * {
         *      "Name":"",
         *      "BlendMode": int,
         *      "ShadingModel": int,
         *      "Properties":
         *      {
         *          "BaseColorTint":
         *          "EmissiveColorTint":
         *          ....
         *          "OpacityScale":
         *      }
         *      "Textures": // ordered array
         *      [
         *          { // BaseColor
         *              "AssetHandle":"",
         *              "Mapping":"ComponentMapping"
         *          },
         *          { // Metallic
         *               "AssetHandle":"",
         *               "Mapping":"ComponentMapping"
         *          },
         *          .......
         *          ,{ // Normal
         *               "AssetHandle":"",
         *               "Mapping":"ComponentMapping"
         *          },
         *      ]
         * }
         */
        std::shared_ptr<AssetBase> Load(AssetHandle handle, const ::std::filesystem::path& mat_path) override
        {
            if (!std::filesystem::exists(mat_path))
            {
                auto msg = "Material file not found in: " + mat_path.string();
                el::Loggers::getLogger(LogResourceManager)->warn(msg);
                throw std::runtime_error(msg);
            }

            std::ifstream level_file(mat_path);
            if (!level_file.is_open())
            {
                auto msg = "Failed to open Material file: " + mat_path.string();
                el::Loggers::getLogger(LogResourceManager)->warn(msg);
                throw std::runtime_error(msg);
            }

            Json::CharReaderBuilder reader_builder;
            Json::Value mat_json;
            std::string errs;

            if (!Json::parseFromStream(reader_builder, level_file, &mat_json, &errs))
                throw std::runtime_error("Failed to parse Material file: " + errs);

            auto rs = Engine::Instance().RenderSystem();
            auto rm = Engine::Instance().ResourceManager();

            std::array<std::shared_ptr<TextureAsset>, MaxTextureCount> textures;
            std::array<ComponentMapping, MaxTextureCount> comp_map;

            for (int i = 0; i < mat_json["Textures"].size(); ++i)
            {
                auto tex_handle = AssetHandle::FromJson(mat_json["Textures"][i]["AssetHandle"]);

                if (tex_handle.id != Uuid::Null())
                    textures[i] = rm->GetAsset<TextureAsset>(tex_handle);

                comp_map[i] = mat_json["Textures"][i]["Mapping"].asInt();
            }

            Material::MaterialData material_data(mat_json["Properties"]);

            ShadingModel sm = static_cast<ShadingModel>(mat_json["ShadingModel"].asInt());

            BlendMode bm = static_cast<BlendMode>(mat_json["BlendMode"].asInt());

            std::shared_ptr<Material> result = std::make_shared<Material>(handle, bm, sm, material_data, textures, comp_map);

            result->name = mat_json["Name"].asString();

            return result;
        }

        void Save(std::shared_ptr<AssetBase> asset, const std::filesystem::path& path) override
        {
            Todo();
        }

        const char* GetName() override
        {
            return "Material Loader";
        }
    };
}
