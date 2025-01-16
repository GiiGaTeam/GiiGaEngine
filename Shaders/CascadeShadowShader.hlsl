#ifndef SHADOW_MAPPING_SHADER
#define SHADOW_MAPPING_SHADER

#include "ShaderHeader.hlsl"
#include "GLightHeader.hlsl"

cbuffer WorldMatricies : register(b0)
{
    WorldMatrices worldMatricies;
}

cbuffer LightBuf : register(b1)
{
    DirectionLightData lightData;
}

struct GS_IN
{
    float4 pos : POSITION;
};

struct GS_OUT
{
    float4 pos : SV_POSITION;
    uint arrInd : SV_RenderTargetArrayIndex;
};

GS_IN VSMain(VS_INPUT input)
{
    GS_IN output = (GS_IN)0;

    output.pos = mul(float4(input.Pos.xyz, 1.0f), worldMatricies.World);
    //output.pos += sin(output.pos.x * 10.0f * lightData.kaSpecPowKsX.w);
    return output;
}
#endif
