#include "ShaderHeader.hlsl"
#include "MaterialHeader.hlsl"

Texture2D LightAccum : register(t0);

float4 PSMain(PS_INPUT input) : SV_Target
{
    float4 color = LightAccum.Sample(sampl, input.Tex);

    color = pow(color, 1 / 2.2);

    return color; // must return an RGBA colour
}
