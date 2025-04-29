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
    float3 Normal   : NORMAL0;
    float2 UV       : TEXCOORD0;
};

struct Output
{
    float4 Position      : SV_Position;
    float3 WorldPosition : POSITION0;
    float3 FragPos       : POSITION1;
    float3 LightPos      : POSITION2;
    float4 Normal        : NORMAL0;
    float2 UV            : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.UV = input.UV;
    float4 modelPos = mul(u_model, float4(input.Position, 1.0f));
    output.WorldPosition = modelPos.xyz / modelPos.w;
    output.Position = mul(u_viewProj, modelPos);
    output.FragPos = mul(u_view, modelPos).xyz;
    output.LightPos = mul(u_view, float4(u_lightpos, 1.0f)).xyz;
    output.Normal = normalize(mul(u_view, mul(u_model, float4(input.Normal, 0.0f))));
    return output;
}
