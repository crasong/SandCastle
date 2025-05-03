Texture2D<float4> AlbedoTex    : register(t0, space2);
Texture2D<float4> NormalTex    : register(t1, space2);
//Texture2D<float4> EmissiveTex  : register(t2, space2);
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
    float4 Position : SV_Position;
    float3 ViewPos  : POSITION0;
    float3 FragPos  : POSITION1;
    float3 LightPos : POSITION2;
    float2 UV       : TEXCOORD0;
};

float3 BlinnPhong(float3 viewPos, float3 fragPos, float3 lightPos, float3 lightColor, float2 uv)
{
    float ambientStrength = 0.1f;
    float3 ambient = ambientStrength * u_lightcolor;

    float3 norm = NormalTex.Sample(Sampler, uv).rgb;
    norm = normalize(norm * 2.0f - 1.0f);
    float3 lightDir = normalize(lightPos - fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuse = diff * u_lightcolor;

    float specularStrength = 0.2f;
    float3 viewDir = normalize(viewPos - fragPos);
    float3 reflectDir = reflect(-lightDir, norm);
    float3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0f), 32.0f);
    float3 specular = specularStrength * spec * u_lightcolor;

    float distance = length(lightPos - fragPos);
    float attenuation = 1.0f / (distance);

    diffuse *= attenuation;
    specular *= attenuation;
    ambient *= attenuation;

    return diffuse + specular + ambient;
}

float4 main(Input input) : SV_Target0
{
    // float ambientStrength = 0.1f;
    // float3 ambient = ambientStrength * u_lightcolor;

    // float3 norm = NormalTex.Sample(Sampler, input.UV).rgb;
    // norm = normalize(norm * 2.0f - 1.0f);
    // float3 lightDir = normalize(input.LightPos - input.FragPos);

    // float diff = max(dot(norm, lightDir), 0.0);
    // float3 diffuse = diff * u_lightcolor;

    // float specularStrength = 0.2f;
    // float3 viewDir = normalize(input.ViewPos - input.FragPos);
    // float3 reflectDir = reflect(-lightDir, norm);
    // float3 halfwayDir = normalize(lightDir + viewDir);
    // float spec = pow(max(dot(norm, halfwayDir), 0.0f), 32.0f);
    // float3 specular = specularStrength * spec * u_lightcolor;

    // float distance = length(input.LightPos - input.FragPos);
    // float attenuation = 1.0f / (distance);

    // diffuse *= attenuation;
    // specular *= attenuation;


    float gamma = 1.0/2.2;
    float4 texColor = AlbedoTex.Sample(Sampler, input.UV);
    //float3 result = ambient + diffuse + specular;
    float3 result = BlinnPhong(input.ViewPos, input.FragPos, input.LightPos, u_lightcolor, input.UV);
    result *= texColor.rgb;
    //result = pow(result, float3(gamma, gamma, gamma));
    return float4(result, 1.0f);
}
