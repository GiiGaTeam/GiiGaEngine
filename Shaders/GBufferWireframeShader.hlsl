#include "ShaderHeader.hlsl"

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

/*
 *  GBuffer Structure:
 *                          R       G       B       A
 *      LightAccum          R       G       B    depth(copy)
 *      Diffuse             R       G       B      NU
 *      MatProps         Metal    Spec    Rough    Aniso
 *      NormalVS            X       Y       Z      NU
 *      
 **** Depth/Stencil              D24_UNORM     S8_UINT
 */
struct PixelShaderOutput
{
    float4 LightAccumulation : SV_Target0;
    float4 Diffuse : SV_Target1;
    float4 MatProp : SV_Target2;
    float4 NormalVS : SV_Target3;
};

PixelShaderOutput PSMain(PS_INPUT input)
{
    PixelShaderOutput output = (PixelShaderOutput)0;

    // Set Light Accumulation to the base color
    output.LightAccumulation.xyz = float3(0.7, 0.7, 0.7);
    output.LightAccumulation.w = input.Pos.z;

    return output;
}
