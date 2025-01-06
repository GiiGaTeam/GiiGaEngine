#include "ShaderHeader.hlsl"

PS_INPUT VSMain(uint id: SV_VertexID)
{
    PS_INPUT output = (PS_INPUT)0;

    //output.Tex = float2(id & 1, (id & 2) >> 1);
    output.Tex = float2(-1 * (id - 1) * (id - 2) / 2 + 1 & 3, id >> 1);
    output.Pos = float4(output.Tex * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}
