Texture2D<float4> AlbedoTex    : register(t0, space2);
Texture2D<float4> NormalTex    : register(t1, space2);
Texture2D<float4> EmissiveTex  : register(t2, space2);
//Texture2D<float4> MetallicTex  : register(t3, space2);
//Texture2D<float4> RoughnessTex : register(t4, space2);
//Texture2D<float4> AOTex        : register(t5, space2);
SamplerState Sampler    : register(s0, space2);

cbuffer Light : register(b0, space3) 
{
    float3 u_lightcolor;
};

struct Input
{
    float4 Position      : SV_Position;
    float3 WorldPosition : POSITION0;
    float3 FragPos       : POSITION1;
    float3 LightPos      : POSITION2;
    float4 Normal        : NORMAL0;
    float2 UV            : TEXCOORD0;
};

float4 main(Input input) : SV_Target0
{
    float ambientStrength = 0.1f;
    float3 ambient = ambientStrength * u_lightcolor;

    float3 norm = normalize(input.Normal).xyz;
    float3 lightDir = normalize(input.LightPos - input.FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * u_lightcolor;

    float specularStrength = 0.5f;
    float3 viewDir = normalize(-input.FragPos);
    float3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32.0f);
    float3 specular = specularStrength * spec * u_lightcolor;

    float4 texColor = AlbedoTex.Sample(Sampler, input.UV);
    float4 result = float4(ambient + diffuse + specular, 1.0f) * texColor;
    return result;
}
