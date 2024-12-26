#include "ShaderHeader.hlsl"

struct PointLightData
{
    float3 posWS;
    float3 color;
    float radius;
    float max_intensity;
    float falloff;
};

cbuffer PointLight : register(b2)
{
    PointLightData pointLight;
}

Texture2D Diffuse : register(t0);
Texture2D MatProp : register(t1);
Texture2D NormalWS : register(t2);
Texture2D PositionWS : register(t3);

SamplerState sampl : register(s0);

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
};

//https://lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html
float attenuate_cusp(float distance, float radius, float max_intensity, float falloff)
{
    float s = distance / radius;

    if (s >= 1.0)
        return 0.0;

    float s2 = sqrt(s);

    return max_intensity * sqrt(1 - s2) / (1 + falloff * s);
}

float3 CalcPointLight(float3 surf_col, float3 lightPos, float3 normal, float3 fragPos, float3 viewDir, float shininess)
{
    float3 lightDir = normalize(lightPos - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // attenuation
    float distance = length(lightPos - fragPos);
    // combine results
    return attenuate_cusp(distance, pointLight.radius, pointLight.max_intensity, pointLight.falloff) * pointLight.color * (diff + spec) * surf_col;
}

PixelShaderOutput PSMain(PS_INPUT input)
{
    PixelShaderOutput output = (PixelShaderOutput)0;

    int2 texCoord = input.Pos.xy;
    // Sample the textures from the GBuffer
    float4 DiffuseColor = Diffuse.Load(int3(texCoord,0));
    float4 MatProps = MatProp.Load(int3(texCoord,0));
    float4 NormalTex = NormalWS.Load(int3(texCoord,0));
    float4 PositionTex = PositionWS.Load(int3(texCoord,0));
    
    // Retrieve the WorldSpace position and normal from the textures
    float3 fragPosWS = PositionTex.xyz;
    float3 normalWS = normalize(NormalTex.xyz);
    
    // Calculate the view direction
    float3 viewDir = normalize(GetCameraPos() - fragPosWS);  // Assuming the camera is at the origin
    
    // Calculate the surface color from the diffuse texture
    float3 surfColor = DiffuseColor.rgb;
    
    // Calculate the light accumulation
    float3 lightAccum = CalcPointLight(surfColor, pointLight.posWS, normalWS, fragPosWS, viewDir, MatProps.z);
    
    // Output the light accumulation
    output.LightAccumulation = float4(lightAccum, 1.0);  // Alpha can be 1.0 or another value if needed
    
    return output;
}
