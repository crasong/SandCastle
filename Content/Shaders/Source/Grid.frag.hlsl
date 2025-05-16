cbuffer Params : register(b0, space3)
{
    float2 u_offset; // Offset at 64 bytes
    int u_numCells;
    float u_thickness;
    float u_scroll; // in [1, 2]
};

struct Input
{
    float4 Position : SV_Position;
    float3 WorldPosition : POSITION0;
    float3 CamWorldPosition : POSITION1;
    float2 TexCoord : TEXCOORD0;
};

float ease_inout_exp(float x)
{
    return x<=0.0f ? 0.0f : pow(2, 10.0f * (x - 1.0f));
}

float ease_inout_quad(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x < 0.5f) return 2.0f * x * x;
    else return 1.0f - pow(-2.0f * x + 2.0f, 2.0f) / 2.0f;
}

bool on_grid(float2 pos, float thickness)
{
    thickness /= u_scroll;
    return (pos.y < thickness) || (pos.y > 1.0f - thickness) ||
           (pos.x < thickness) || (pos.x > 1.0f - thickness);
}

float max2(float2 v)
{
    return max(v.x, v.y);
}

float getLodAlpha(float3 worldPos, float gridCellSize, float2 dudv)
{
    float2 modVec = (fmod(abs(worldPos.xz), gridCellSize) / dudv);
    float alpha = max2(float2(1.0f, 1.0f) - abs(saturate(modVec) * 2.0f - float2(1.0f, 1.0f)));
    return alpha;
}

float4 main(Input input) : SV_Target0
{
    const float minPixelsBetweenCells = 2.0f;
    const float gridSize = 100.0f;
    const float4 gridColorThin  = float4(0.3f, 0.3f, 0.3f, 1.0f);
    const float4 gridColorThick = float4(0.7f, 0.7f, 0.7f, 1.0f);

    float4 color; // final result

    float2 dvx = float2(ddx(input.WorldPosition.x), ddy(input.WorldPosition.x));
    float2 dvy = float2(ddx(input.WorldPosition.z), ddy(input.WorldPosition.z));

    float2 dudv = float2(length(dvx), length(dvy));
    float l = length(dudv);


    float LOD = max(0.0f, log10((l * minPixelsBetweenCells) / u_thickness) + 1.0f);
    float gridCellSizeLod0 = u_thickness * pow(10.0, floor(LOD));
    float gridCellSizeLod1 = gridCellSizeLod0 * 10.0f;
    float gridCellSizeLod2 = gridCellSizeLod1 * 10.0f;

    dudv *= 4.0f;

    float Lod0a = getLodAlpha(input.WorldPosition, gridCellSizeLod0, dudv);
    float Lod1a = getLodAlpha(input.WorldPosition, gridCellSizeLod1, dudv);
    float Lod2a = getLodAlpha(input.WorldPosition, gridCellSizeLod2, dudv);

    float LOD_fade = frac(LOD);
    
    if (Lod2a > 0.0f) {
        color = gridColorThick;
        color.a *= Lod0a;
    }
    else {
        if (Lod1a > 0.0f) {
            color = lerp(gridColorThick, gridColorThin, LOD_fade);
            color.a *= Lod1a;
        }
        else {
            color = gridColorThin;
            color.a *= (Lod0a * (1.0f - LOD_fade));
        }
    }

    float opacityFalloff = (1.0f - saturate(length(input.WorldPosition.xz - input.CamWorldPosition.xz) / gridSize));
    color.a *= opacityFalloff;

    return color;
}