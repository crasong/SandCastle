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

cbuffer Light : register(b2, space1)
{
    float3 u_lightpos;
};

struct Input
{
    float3 Position : POSITION0;
    float3 Normal : NORMAL0;
    float2 TexCoord : TEXCOORD0;
};

struct Output
{
    float4 Position : SV_Position;
    float4 Normal : POSITION0;
    float3 FragPos  : POSITION1;
    float3 LightPos : POSITION2;
    float2 TexCoord : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.TexCoord = input.TexCoord;
    //output.Position = mul(u_proj, mul(u_view, mul(u_model, float4(input.Position, 1.0f))));
    float4 modelPos = mul(u_model, float4(input.Position, 1.0f));
    output.Position = mul(u_viewProj, modelPos);
    output.FragPos = mul(u_view, modelPos).xyz;
    output.LightPos = mul(u_view, float4(u_lightpos, 1.0f)).xyz;
    output.Normal = mul(u_view, mul(u_model, float4(input.Normal, 0.0f)));
    return output;
}
