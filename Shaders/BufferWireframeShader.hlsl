#include "ShaderHeader.hlsl"

/*
 *  GBuffer Structure:
 *                          R       G       B       A
 *      LightAccum          R       G       B    depth(copy)
 *      Diffuse             R       G       B      NU
 *      MatProps         Metal    Spec    Rough    Aniso
 *      NormalVS            X       Y       Z      NU
 *      
 **** Depth/Stencil              D24_UNORM     S8_UINT
 */

float4 PSMain(PS_INPUT input) : SV_Target
{
    float4 output = 0;

    // Set Light Accumulation to the base color
    output = float4(0.5f, 0.0f, 0.5f, 1.0f);

    return output;
}
