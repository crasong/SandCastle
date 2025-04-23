cbuffer Params : register(b0, space3)
{
    float2 u_offset; // Offset at 64 bytes
    int u_numCells;
    float u_thickness;
    float u_scroll; // in [1, 2]
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

float4 main(float2 uv : TEXCOORD0) : SV_Target
{
    const float3 gridColor = float3(0.5f, 0.5f, 0.5f);
    float2 gridPos = fmod(uv - 0.5f, 1.0f/u_numCells);
    gridPos *= u_numCells;

    float halfThickness = u_thickness / 2.0f;
    float2 halfGridPos = fmod(uv = 0.5f, 0.5f/u_numCells);
    halfGridPos *= u_numCells * 2.0f;

    float3 color = float3(0.0f, 0.0f, 0.0f);
    if (on_grid(halfGridPos, halfThickness)) {
        color += gridColor * ease_inout_quad(2.0f - 2.0f * u_scroll);
    }
    if (on_grid(gridPos, u_thickness)) {
        color += gridColor * ease_inout_quad(2.0f * u_scroll - 1.0f);
    }

    color = min(color, gridColor);

    float2 centeredPos = 2.0f * (u_offset - 0.5f - u_offset) / u_scroll;
    color *= max(2.5f * ease_inout_exp(1.0f - length(centeredPos)), 0.0f);
    
    return float4(color, 1.0f);
}