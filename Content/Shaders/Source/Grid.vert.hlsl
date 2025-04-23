cbuffer Camera : register(b0, space1)
{
    float4x4 u_view;
    float4x4 u_proj;
    float4x4 u_viewProj;
};

cbuffer Model : register(b1, space1)
{
    float4x4 u_model;
};

struct Input
{
    float3 Position : TEXCOORD0;
    float2 TexCoord : TEXCOORD1;
};

struct Output
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_Position;
};

Output main(Input input)
{
    Output output;
    output.TexCoord = input.TexCoord;
    output.Position = mul(u_viewProj, mul(u_model, float4(input.Position, 1.0f)));
    return output;
}
