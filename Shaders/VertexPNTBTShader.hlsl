#include "ShaderHeader.hlsl"

PS_INPUT VSMain(VS_INPUT input, uint instID: SV_InstanceID)
{
    PS_INPUT output = (PS_INPUT)0;
    output.PosWS = mul(float4(input.Pos.xyz, 1), World);
    output.PosVS = mul(float4(output.PosWS, 1), View);
    output.Pos = mul(float4(output.PosVS, 1), Proj);

    output.NormWS = mul(float4(input.Norm.xyz, 0), World).xyz;
    output.NormVS = mul(mul(float4(input.Norm.xyz, 0), World), View).xyz;
    output.NormWS = normalize(output.NormWS);
    output.NormVS = normalize(output.NormVS);

    output.TangentWS = mul(float4(input.Tangent.xyz, 0), World).xyz;
    output.TangentVS = mul(mul(float4(input.Tangent.xyz, 0), World), View).xyz;
    output.TangentWS = normalize(output.TangentWS);
    output.TangentVS = normalize(output.TangentVS);

    output.BitangentWS = mul(float4(input.Bitangent.xyz, 0), World).xyz;
    output.BitangentVS = mul(mul(float4(input.Bitangent.xyz, 0), World), View).xyz;
    output.BitangentWS = normalize(output.BitangentWS);
    output.BitangentVS = normalize(output.BitangentVS);

    output.Tex = input.Tex;

    return output;
}
