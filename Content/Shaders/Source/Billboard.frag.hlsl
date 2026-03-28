struct Input {
  float2 UV : TEXCOORD0;
  float4 Color : COLOR0;
};

float4 main(Input input) : SV_Target0 {
  // Circular falloff from center
  float2 centered = input.UV * 2.0 - 1.0;
  float dist = dot(centered, centered);
  if (dist > 1.0) discard;

  float alpha = smoothstep(1.0, 0.3, dist);
  return float4(input.Color.rgb, input.Color.a * alpha);
}
