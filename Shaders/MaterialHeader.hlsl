#ifndef MATERIAL_DATA
#define MATERIAL_DATA

struct MaterialData
{
    float3 BaseColorTint_;
    float3 EmissiveColorTint_;
    float MetallicScale_;
    float SpecularScale_;
    float RoughnessScale_;
    float AnisotropyScale_;
    float OpacityScale_;
};

cbuffer Material : register(b2)
{
    MaterialData material;
}

Texture2D BaseColor : register(t0);
Texture2D Metallic : register(t1);
Texture2D Specular : register(t2);
Texture2D Roughness : register(t3);
Texture2D Anisotropy : register(t4);
Texture2D EmissiveColor : register(t5);
Texture2D Opacity : register(t6);
Texture2D Normal : register(t7);

SamplerState sampl : register(s0);

#endif
