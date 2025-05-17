Texture2D<float4> AlbedoTex    : register(t0, space2);
Texture2D<float4> NormalTex    : register(t1, space2);
Texture2D<float4> EmissiveTex  : register(t2, space2);
Texture2D<float4> MetallicTex  : register(t3, space2);
Texture2D<float4> RoughnessTex : register(t4, space2);
Texture2D<float4> AOTex        : register(t5, space2);
SamplerState      Sampler      : register(s0, space2);

// cbuffer Light : register(b0, space3) 
// {
//     float3 u_lightcolor;
// };

struct Input
{
    float4 Position : SV_Position;
    float3 ViewPos  : POSITION0;
    float3 FragPos  : POSITION1;
    float3 LightsPos[9] : POSITION2;
    float2 UV       : TEXCOORD0;
};

float3 BlinnPhong(float3 viewPos, float3 fragPos, float3 lightPos, float3 lightColor, float2 uv)
{
    const float radius = 5;
    float distance = length(lightPos - fragPos);
    float3 resultLight = float3(0,0,0);

    if (distance < radius) {
        float ambientStrength = 0.1f;
        float3 ambient = ambientStrength * lightColor;
        //float3 ambient = AOTex.Sample(Sampler, uv).rgb * lightColor;

        float3 norm = NormalTex.Sample(Sampler, uv).rgb;
        //norm = normalize(norm * 2.0f - 1.0f);
        //norm = normalize(norm);
        float3 lightDir = normalize(lightPos - fragPos);

        float diff = max(dot(norm, lightDir), 0.0);
        float3 diffuse = diff * lightColor;

        float specularStrength = 0.2f;
        float3 viewDir = normalize(viewPos - fragPos);
        float3 reflectDir = reflect(-lightDir, norm);
        float3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0f), 16.0f);
        float3 specular = specularStrength * spec * lightColor;

        //float attenuation = 1.0f / (distance * distance);
        float attenuation = radius / (pow(distance, 2) + 1);

        diffuse *= attenuation;
        specular *= attenuation;
        ambient *= attenuation;
        //resultLight = diffuse + specular + ambient;
        //resultLight = diffuse + specular;
        resultLight = norm;
    }

    return resultLight;
}

float4 main(Input input) : SV_Target0
{
    float3 u_lightcolor = float3(1.0f, 1.0f, 1.0f);
    float gamma = 1.0/2.2;
    float4 texColor = AlbedoTex.Sample(Sampler, input.UV);
    float3 result = texColor.rgb * 0.1f; 
    for (int i = 0; i < 9; ++i) {
        result += BlinnPhong(input.ViewPos, input.FragPos, input.LightsPos[i], u_lightcolor, input.UV);
    }
    //result *= texColor.rgb;
    //result = pow(result, float3(gamma, gamma, gamma));
    return float4(result, 1.0f);
}
