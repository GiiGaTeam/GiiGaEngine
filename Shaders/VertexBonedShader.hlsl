#include "ShaderHeader.hlsl"


cbuffer CameraMatricies : register(b0)
{
    CameraMatricies cameraMatricies;
}

cbuffer WorldMatricies : register(b1)
{
    WorldMatrices worldMatricies;
}

cbuffer cbSkinned : register(b3)
{
    // Max support of 96 bones per character.
    float4x4 gBoneTransforms[96];
};

PS_INPUT VSMain(VS_BONED_INPUT input, uint instID: SV_InstanceID)
{
    PS_INPUT output = (PS_INPUT)0;
    
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = input.BoneWeights.x;
    weights[1] = input.BoneWeights.y;
    weights[2] = input.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];
    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    float3 tangentL = float3(0.0f, 0.0f, 0.0f);
    for(int i = 0; i < 4; ++i)
    {
        // Assume no nonuniform scaling when transforming normals, so
        // that we do not have to use the inverse-transpose.
        posL += weights[i] * mul(float4(input.Pos, 1.0f),
        gBoneTransforms[input.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(input.Norm,
        (float3x3)gBoneTransforms[input.BoneIndices[i]]);
        tangentL += weights[i] * mul(input.Tangent.xyz,
        (float3x3)gBoneTransforms[input.BoneIndices[i]]);
    }
    input.Pos = posL;
    input.Norm = normalL;
    input.Tangent.xyz = tangentL;
    
    output.PosWS = mul(float4(input.Pos.xyz, 1), worldMatricies.World);
    output.PosVS = mul(float4(output.PosWS, 1), cameraMatricies.View);
    output.Pos = mul(float4(output.PosVS, 1), cameraMatricies.Proj);

    output.NormWS = mul(float4(input.Norm.xyz, 0), worldMatricies.World).xyz;
    output.NormVS = mul(mul(float4(input.Norm.xyz, 0), worldMatricies.World), cameraMatricies.View).xyz;
    output.NormWS = normalize(output.NormWS);
    output.NormVS = normalize(output.NormVS);

    output.TangentWS = mul(float4(input.Tangent.xyz, 0), worldMatricies.World).xyz;
    output.TangentVS = mul(mul(float4(input.Tangent.xyz, 0), worldMatricies.World), cameraMatricies.View).xyz;
    output.TangentWS = normalize(output.TangentWS);
    output.TangentVS = normalize(output.TangentVS);

    output.BitangentWS = mul(float4(input.Bitangent.xyz, 0), worldMatricies.World).xyz;
    output.BitangentVS = mul(mul(float4(input.Bitangent.xyz, 0), worldMatricies.World), cameraMatricies.View).xyz;
    output.BitangentWS = normalize(output.BitangentWS);
    output.BitangentVS = normalize(output.BitangentVS);

    output.Tex = input.Tex;

    return output;
}
