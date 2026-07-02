// Self-contained smoke-test shader: projects a clip-space position directly.
struct VSOut
{
    float4 pos : SV_Position;
};

VSOut VSMain(float3 pos : POSITION)
{
    VSOut o;
    o.pos = float4(pos, 1.0);
    return o;
}
