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

namespace GiiGa
{
    export class MaterialLoader : public AssetLoader
    {
    public:
        MaterialLoader()
        {
            pattern_ = R"((.+)\.(mat))";
            type_ = AssetType::Material;
        }

        virtual ~MaterialLoader() = default;

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

        // todo
        void Save(AssetBase& asset, ::std::filesystem::path& path) override
        {
            // ToJson logic
        }

        const char* GetName() override
        {
            return "Material Loader";
        }
    };
}
