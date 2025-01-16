#ifndef SHADER_HEADER
#define SHADER_HEADER

struct CameraMatricies
{
    matrix View;
    matrix Proj;
    matrix InvView;
    matrix InvProj;
    float3 CamPos;
};

struct WorldMatrices
{
    matrix World;
    matrix invWorld;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm : NORMAL;
    float3 Tangent : TANGENT;
    float3 Bitangent : BINORMAL;
    float2 Tex : TEXCOORD;
};

//TODO
//????????? ?? skeletal mesh
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 PosWS : TEXCOORD0;
    float3 PosVS : TEXCOORD1;
    float3 NormWS : TEXCOORD2;
    float3 NormVS : TEXCOORD3;
    float3 TangentWS : TEXCOORD4;
    float3 TangentVS : TEXCOORD5;
    float3 BitangentWS : TEXCOORD6;
    float3 BitangentVS : TEXCOORD7;
    float2 Tex : TEXCOORD8;
};

#endif
