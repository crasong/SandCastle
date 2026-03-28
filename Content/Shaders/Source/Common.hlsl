#define MAX_LIGHTS 16

struct FragInput
{

};

struct Light
{
    float3 position;
    float _pad0;
    float3 color;
    float _pad1;
};
