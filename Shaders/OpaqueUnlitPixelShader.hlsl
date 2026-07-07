#include "ShaderHeader.hlsl"
#include "MaterialHeader.hlsl"

float4 PSMain(PS_INPUT input) : SV_Target
{
    float4 output = 0;

    // Sample emissive color texture
    float3 emiss_col = EmissiveColor.Sample(sampl, input.Tex).xyz * material.EmissiveColorTint_;

    // Set Light Accumulation to the emissive color
    output.xyz = emiss_col.rgb;
    output.w = input.Pos.z;

    return output;
}
