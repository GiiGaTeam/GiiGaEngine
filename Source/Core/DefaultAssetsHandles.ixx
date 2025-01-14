export module StubTexturesHandles;

import AssetHandle;

namespace GiiGa
{
    namespace DefaultAssetsHandles
    {
        ////////////////// TEXTURES //////////////////////////////
        export const AssetHandle BaseColor =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000001").value(), 0};
        export const AssetHandle Metallic =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000002").value(), 0};
        export const AssetHandle Specular =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000003").value(), 0};
        export const AssetHandle Roughness =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000004").value(), 0};
        export const AssetHandle Anisotropy =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000005").value(), 0};
        export const AssetHandle EmissiveColor =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000006").value(), 0};
        export const AssetHandle Opacity =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000007").value(), 0};
        export const AssetHandle Normal =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000008").value(), 0};

        ////////////////// MATERIALS //////////////////////////////
        
        export const AssetHandle DefaultMaterial =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-000000000009").value(), 0};
        export const AssetHandle DefaultLitMaterial =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-00000000000a").value(), 0};

        ////////////////// MESHES //////////////////////////////

        export const AssetHandle Cube =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-00000000000b").value(), 0};
        export const AssetHandle Sphere =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-00000000000c").value(), 0};
        export const AssetHandle Quad =
            AssetHandle{Uuid::FromString("00000000-0000-0000-0000-00000000000d").value(), 0};
    }
}
