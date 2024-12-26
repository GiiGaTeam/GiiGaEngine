cbuffer WorldMatricies : register( b0 )
{
    matrix World;  
    matrix invWorld;
    //bool RenderState;
}

cbuffer WorldMatricies : register( b1 )
{
    matrix View;
    matrix Proj;
    //bool RenderState;
}

struct VS_INPUT                   
{
    float4 Pos : POSITION;         
};

struct PS_INPUT                   
{
    float4 Pos : SV_POSITION;    
    float4 PosWS : POSITION0;
    float4 PosVS : POSITION1;  
};

PS_INPUT VSMain( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.PosWS = mul(float4( input.Pos.xyz, 1), World );
    output.PosVS = mul( output.PosWS, View);
    output.Pos = mul( output.PosVS, Proj);
    
    return output;
}