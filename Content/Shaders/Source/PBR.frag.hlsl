#include "Common.hlsl"

Texture2D<float4> AlbedoTex : register(t0, space2);
Texture2D<float4> NormalTex : register(t1, space2);
Texture2D<float4> EmissiveTex : register(t2, space2);
Texture2D<float4> MetallicTex : register(t3, space2);
Texture2D<float4> RoughnessTex : register(t4, space2);
Texture2D<float4> AOTex : register(t5, space2);
SamplerState Sampler : register(s0, space2);

struct Input {
  float3 ViewPos : POSITION0;
  float3 FragPos : POSITION1;
  float3 LightsPos[MAX_LIGHTS] : POSITION2;
  float3 Normal : NORMAL0;
  float3 Tangent : TANGENT0;
  float3 Bitangent : TANGENT1;
  float2 UV : TEXCOORD0;
  uint NumLights : BLENDINDICES0;
};

struct TexSamples {
  float3 albedo;
  float3 normal;
  float3 emissive;
  float metallic;
  float roughness;
  float ao;
};

float3 BlinnPhong(TexSamples samples, float3x3 TBN, float3 viewPos, float3 fragPos, float3 lightPos,
                  float3 lightColor) {
  const float radius = 15.0f;
  float dist = length(lightPos - fragPos);
  if (dist >= radius) return float3(0, 0, 0);

  float3 norm = normalize(samples.normal * 2.0f - 1.0f);
  norm = normalize(mul(norm, TBN));
  float3 lightDir = normalize(lightPos - fragPos);

  // diffuse
  float diff = max(dot(norm, lightDir), 0.0);

  // specular (Blinn-Phong)
  float3 viewDir = normalize(viewPos - fragPos);
  float3 halfwayDir = normalize(lightDir + viewDir);
  float shininess = max(2.0f, (1.0f - samples.roughness) * 256.0f);
  float spec = pow(max(dot(norm, halfwayDir), 0.0f), shininess);

  // smooth distance attenuation that reaches 0 at radius
  float distAtten = 1.0f / (dist * dist + 1.0f);
  float windowing = pow(saturate(1.0f - pow(dist / radius, 4.0f)), 2.0f);
  float attenuation = distAtten * windowing;

  // PBR metallic workflow
  float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), samples.albedo, samples.metallic);
  float3 diffuseContrib = diff * lightColor * samples.albedo * (1.0f - samples.metallic);
  float3 specularContrib = spec * lightColor * F0;

  return (diffuseContrib + specularContrib) * attenuation;
}

float4 main(Input input) : SV_Target0 {
  float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Normal);
  float3 u_lightcolor = float3(1.0f, 1.0f, 1.0f);
  float gamma = 1.0 / 2.2;

  TexSamples samples;
  samples.albedo    = AlbedoTex.Sample(Sampler, input.UV).rgb;
  samples.normal    = NormalTex.Sample(Sampler, input.UV).rgb;
  samples.emissive  = EmissiveTex.Sample(Sampler, input.UV).rgb;
  samples.metallic  = MetallicTex.Sample(Sampler, input.UV).b;   // glTF: blue channel
  samples.roughness = RoughnessTex.Sample(Sampler, input.UV).g;  // glTF: green channel
  samples.ao        = AOTex.Sample(Sampler, input.UV).r;

  // ambient: albedo scaled by AO
  float3 ambient = 0.03f * samples.albedo * samples.ao;

  // accumulate light contributions
  float3 result = ambient;
  for (uint i = 0; i < input.NumLights; ++i) {
    result += BlinnPhong(samples, TBN, input.ViewPos, input.FragPos, input.LightsPos[i],
                         u_lightcolor);
  }

  result += samples.emissive;
  result = pow(result, float3(gamma, gamma, gamma));
  return float4(result, 1.0f);
}
