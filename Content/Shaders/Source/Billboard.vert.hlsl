cbuffer Camera : register(b0, space1) {
  float4x4 u_view;
  float4x4 u_proj;
  float4x4 u_viewProj;
  float3 u_viewPos;
};

cbuffer Billboard : register(b1, space1) {
  float3 u_position;
  float u_size;
  float4 u_color;
};

struct Output {
  float4 Position : SV_Position;
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

Output main(uint vertexID : SV_VertexID) {
  // Two-triangle quad from 6 vertices
  static const float2 corners[6] = {
    float2(-1, -1), float2(1, -1), float2(-1, 1),
    float2(1, -1), float2(1, 1), float2(-1, 1)
  };
  float2 corner = corners[vertexID];

  // Billboard: camera-facing quad
  float3 forward = normalize(u_position - u_viewPos);
  float3 right = normalize(cross(float3(0, 1, 0), forward));
  float3 up = cross(forward, right);

  float3 worldPos = u_position + (right * corner.x + up * corner.y) * u_size;

  Output output;
  output.Position = mul(u_viewProj, float4(worldPos, 1.0));
  output.UV = corner * 0.5 + 0.5;
  output.Color = u_color;
  return output;
}
