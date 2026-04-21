#include "../Utils/BindlessRS.hlsl"
#define SHADER_MODEL_6_6
static const float GAMMA = 0.454545455f;

cbuffer Resources : register(b0)
{
    uint VertexBuffID;
    uint textureID;
};

struct VertexData
{
    float2 position;
    float2 texCoord;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

struct Settings
{
    float exposure;
};

[RootSignature(RootSig)]
float4 main(VSOutput input) : SV_Target
{
    Texture2D rtvTexture = ResourceDescriptorHeap[textureID];
    return rtvTexture.Sample(pointClamp, input.texCoord);
    //float3 color = rtvTexture.Sample(pointSampler, input.texCoord);
    //return float4(pow(color, GAMMA), 1.0f);
}