#include "ShaderHeader.hlsl"

PS_INPUT VSMain( VS_INPUT_PNTBT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.PosWS = mul(float4( input.Pos.xyz, 1), World );
    output.PosVS = mul( output.PosWS, View);
    output.Pos = mul( output.PosVS, Proj);
    
    output.NormWS = mul( float4( input.Norm, 0), World ).xyz;
    output.NormVS = mul( mul(float4( input.Norm, 0), World), View ).xyz;
    output.NormWS = normalize(output.NormWS);
    output.NormVS = normalize(output.NormVS);
    
    output.TangentWS = mul( float4( input.Tangent, 0), World ).xyz;
    output.TangentVS = mul( mul(float4( input.Tangent, 0), World), View ).xyz;
    output.TangentWS = normalize(output.TangentWS);
    output.TangentVS = normalize(output.TangentVS);
    
    output.BitangentWS = mul( float4( input.Bitangent, 0), World ).xyz;
    output.BitangentVS = mul( mul(float4( input.Bitangent, 0), World), View ).xyz;
    output.BitangentWS = normalize(output.BitangentWS);
    output.BitangentVS = normalize(output.BitangentVS);
    
    output.Tex = input.Tex;
    
    return output;
}