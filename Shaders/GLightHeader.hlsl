#ifndef G_LIGHT_DATA
#define G_LIGHT_DATA

struct PointLightData
{
    float3 posWS;
    float3 color;
    float radius;
    float max_intensity;
    float falloff;
};

struct DirectionLightData
{
    float3 dirWS;
    float max_intensity;
    float3 color;
};

cbuffer ScreenDimensions : register (b2)
{
    float2 ScreenDimensions;
};

Texture2D Diffuse : register(t0);
Texture2D MatProp : register(t1);
Texture2D NormalWS : register(t2);
Texture2D DepthVS : register(t3);

float attenuate(float distance, float radius, float max_intensity, float falloff)
{
    return (1.0f - smoothstep(radius * falloff, radius, distance)) * max_intensity;
}

float attenuate_cusp(float distance, float radius, float max_intensity, float falloff)
{
    float s = distance / radius;

    if (s >= 1.0)
        return 0.0;

    float s2 = sqrt(s);

    return max_intensity * sqrt(1 - s2) / (1 + falloff * s);
}

float4 ClipToWorld(float4 clip, matrix inverseProjView)
{
    float4 posWS = mul(clip, inverseProjView);
    posWS = posWS / posWS.w;

    return posWS;
}

float4 ScreenToWorld(float4 screen, matrix inverseProjView)
{
    float2 texCoord = screen.xy / ScreenDimensions;
    float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

    return ClipToWorld(clip, inverseProjView);
}

#endif
