// Self-contained smoke-test pixel shader: samples a texture and applies a per-frame tint
// (from a volatile constant buffer) to validate the binding-set model end-to-end.
cbuffer Params : register(b0)
{
    float4 tint;
};

Texture2D    tex  : register(t0);
SamplerState samp : register(s0);

struct PSIn
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 PSMain(PSIn i) : SV_Target
{
    return tex.Sample(samp, i.uv) * tint;
}
