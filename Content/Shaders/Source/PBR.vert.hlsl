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

cbuffer Light : register(b2, space1)
{
    float3 u_lightpos;
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
    float3 ViewPos  : POSITION0;
    float3 FragPos  : POSITION1;
    float3 LightPos : POSITION2;
    float2 UV       : TEXCOORD0;
};

Output main(Input input)
{
    Output output;
    output.UV = input.UV;
    
    float3 Normal    = normalize(mul(u_model, float4(input.Normal, 0.0f))).xyz;
    float3 Tangent   = normalize(mul(u_model, float4(input.Tangent, 0.0f))).xyz;
    float3 Bitangent = normalize(mul(u_model, float4(cross(Tangent, Normal), 0.0f))).xyz;
    float3x3 TBN = transpose(float3x3(Tangent, Bitangent, Normal));
    float4 vertPos = mul(u_model, float4(input.Position, 1.0f));
    output.Position = mul(u_viewProj, vertPos);
    output.ViewPos = mul(TBN, u_viewPos);
    output.FragPos = mul(TBN, vertPos.xyz);
    output.LightPos = mul(TBN, u_lightpos);
    return output;
}
