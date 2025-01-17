#ifndef SHADOW_MAPPING_GEOM_SHADER
#define SHADOW_MAPPING_GEOM_SHADER

#include "CascadeShadowShader.hlsl"

StructuredBuffer<CascadeData> Cascades : register(t0);

[maxvertexcount(3)]
void GSMain(triangle GS_IN p[3], inout TriangleStream<GS_OUT> stream)
{
    for (int id = 0; id < lightData.cascadeCount; ++id)
    {
        [unroll]
        for (int i = 0; i < 3; ++i)
        {
            GS_OUT gs = (GS_OUT)0;
            gs.pos = mul(float4(p[i].pos.xyz, 1.0f), Cascades[id].ViewProj);
            gs.arrInd = id;
            stream.Append(gs);
        }
    }
    stream.RestartStrip();
}

#endif
