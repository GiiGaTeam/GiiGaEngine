// Self-contained smoke-test vertex shader: clip-space position + passthrough UV.
struct VSIn
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

VSOut VSMain(VSIn i)
{
    VSOut o;
    o.pos = float4(i.pos, 1.0);
    o.uv = i.uv;
    return o;
}
