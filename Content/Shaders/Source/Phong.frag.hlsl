Texture2D<float4> Diffuse : register(t0, space2);
SamplerState Sampler : register(s0, space2);

cbuffer Light : register(b0, space3) 
{
    float3 u_lightcolor;
};

struct Input
{
    float4 Position : SV_Position;
    float4 Normal : POSITION0;
    float3 FragPos  : POSITION1;
    float3 LightPos : POSITION2;
    float2 TexCoord : TEXCOORD0;
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

    float4 texColor = Diffuse.Sample(Sampler, input.TexCoord);
    float4 result = float4(ambient + diffuse + specular, 1.0f) * texColor;
    return result;
}
