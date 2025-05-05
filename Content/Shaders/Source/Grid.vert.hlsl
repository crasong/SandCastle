cbuffer Camera : register(b0, space1)
{
    float4x4 u_view;
    float4x4 u_proj;
    float4x4 u_viewProj;
    float3   u_viewPos;
};

cbuffer Model : register(b1, space1)
{
    float4x4 u_model;
};

struct Input
{
    float3 Position : POSITION0;
    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float3 Bitangent: TANGENT1;
    float2 UV       : TEXCOORD0;
};

struct Output
{
    float4 Position : SV_Position;
    float3 WorldPosition : POSITION0;
    float2 TexCoord : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.TexCoord = input.UV;
    float3 pos = input.Position;
    pos.x += u_viewPos.x;
    pos.z += u_viewPos.z;
    output.Position = mul(u_viewProj, float4(pos, 1.0f));
    output.WorldPosition = pos;
    return output;
}
