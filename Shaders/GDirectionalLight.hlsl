#include "ShaderHeader.hlsl"
#include "GLightHeader.hlsl"

cbuffer CameraMatricies : register(b0)
{
    CameraMatricies cameraMatricies;
}

cbuffer WorldMatricies : register(b1)
{
    WorldMatrices worldMatricies;
}

cbuffer ScreenDimensions : register (b2)
{
    float2 ScreenDimensions;
}

cbuffer DirectionLight : register(b3)
{
    DirectionLightData directionLight;
}

Texture2D Diffuse : register(t0);
Texture2D MatProp : register(t1);
Texture2D NormalWS : register(t2);
Texture2D DepthVS : register(t3);

SamplerState sampl : register(s0);

/*
 *  GBuffer Structure:
 *                          R       G       B       A
 *      LightAccum          R       G       B    depth(copy)
 *      Diffuse             R       G       B      NU
 *      MatProps         Metal    Spec    Rough    Aniso
 *      NormalWS            X       Y       Z      NU
 *      
 **** Depth/Stencil              D24_UNORM     S8_UINT
 */
struct PixelShaderOutput
{
    float4 LightAccumulation : SV_Target0;
};

//https://lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html


float3 CalcDirectionalLight(float3 surf_col, float3 normal, float3 viewDir, float shininess)
{
    float3 lightDir = directionLight.dirWS;
    // diffuse shading
    float diff = max(dot(lightDir, normal), 0.0);
    // specular shading
    float3 reflectDir = normalize(reflect(-lightDir, normal));

    float spec = pow(max(dot(reflectDir, viewDir), 0.0), shininess);
    // combine results
    return directionLight.max_intensity * directionLight.color * (diff + spec) * surf_col;
}

[earlydepthstencil]
PixelShaderOutput PSMain(PS_INPUT input)
{
    PixelShaderOutput output = (PixelShaderOutput)0;

    int2 texCoord = input.Pos.xy;
    // Sample the textures from the GBuffer
    float4 DiffuseColor = Diffuse.Load(int3(texCoord, 0));
    float4 MatProps = MatProp.Load(int3(texCoord, 0));
    float4 NormalTex = NormalWS.Load(int3(texCoord, 0));
    //float4 PositionTex = PositionWS.Load(int3(texCoord, 0));
    float depth = DepthVS.Load(int3(texCoord, 0)).r;
    matrix ProjView = mul(cameraMatricies.InvProj, cameraMatricies.InvView);
    float4 PositionTex = ScreenToWorld(float4(texCoord, depth, 1.0f), ProjView, ScreenDimensions);


    // Retrieve the WorldSpace position and normal from the textures
    float3 fragPosWS = PositionTex.xyz;
    float3 normalWS = normalize(NormalTex.xyz);

    // Calculate the view direction
    float3 viewDir = normalize(fragPosWS - cameraMatricies.CamPos); // Assuming the camera is at the origin

    // Calculate the surface color from the diffuse texture
    float3 surfColor = DiffuseColor.rgb;

    // Calculate the light accumulation
    float3 lightAccum = CalcDirectionalLight(surfColor, normalWS, viewDir, MatProps.z);

    // Output the light accumulation
    output.LightAccumulation = float4(lightAccum, 1.0); // Alpha can be 1.0 or another value if needed

    return output;
}
