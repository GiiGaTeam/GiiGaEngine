#include "ShaderHeader.hlsl"
#include "MaterialHeader.hlsl"

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
    PixelShaderOutput output = (PixelShaderOutput) 0;

    // Sample base color texture
    float3 emiss_col = EmissiveColor.Sample(sampl, input.Tex).xyz * material.EmissiveColorTint_;

    // Set Light Accumulation to the base color
    output.LightAccumulation.xyz = emiss_col.rgb;
    output.LightAccumulation.w = input.Pos.z;

    return output;
}
