#include "ShaderHeader.hlsl"
#include "MaterialHeader.hlsl"

/*
 *  GBuffer Structure:
 *                          R       G       B       A
 *      LightAccum          R       G       B    depth(copy)
 *      Diffuse             R       G       B      NU
 *      MatProps         Metal    Spec    Rough    Aniso
 *      NormalWS            X       Y       Z      NU
 *      PostionWS           X       Y       Z      NU
 *      
 **** Depth/Stencil              D24_UNORM     S8_UINT
 */
struct PixelShaderOutput
{
    float4 LightAccumulation : SV_Target0;
    float4 Diffuse : SV_Target1;
    float4 MatProp : SV_Target2;
    float4 NormalWS : SV_Target3;
    //float4 PositionWS : SV_Target4;
};

PixelShaderOutput PSMain(PS_INPUT input)
{
    PixelShaderOutput output = (PixelShaderOutput)0;

    // Sample base color texture
    float3 emiss_col = EmissiveColor.Sample(sampl, input.Tex).xyz * material.EmissiveColorTint_;
    // Set Light Accumulation to the base color
    output.LightAccumulation.xyz = emiss_col.rgb;
    output.LightAccumulation.w = input.Pos.z;

    output.Diffuse.xyz = BaseColor.Sample(sampl, input.Tex).xyz * material.BaseColorTint_;

    output.MatProp.x = Metallic.Sample(sampl, input.Tex).x * material.MetallicScale_;
    output.MatProp.y = Specular.Sample(sampl, input.Tex).x * material.SpecularScale_;
    output.MatProp.z = Roughness.Sample(sampl, input.Tex).x * material.RoughnessScale_;
    output.MatProp.w = Anisotropy.Sample(sampl, input.Tex).x * material.AnisotropyScale_;

    // Sample normal map and unpack normal
    float3 normalMap = Normal.Sample(sampl, input.Tex).xyz * 2.0f - 1.0f;

    // Transform normal map from tangent space to World space
    float3x3 TBN = float3x3(input.TangentWS, input.BitangentWS, input.NormWS);
    float3 normalWS = normalize(mul(normalMap, TBN));

    // Write normal in world space to output
    output.NormalWS.xyz = normalWS;

    // Write normal in world space to output
    //output.PositionWS.xyz = input.PosWS;

    return output;
}
