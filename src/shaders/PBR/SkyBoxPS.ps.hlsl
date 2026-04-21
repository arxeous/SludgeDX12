#include "../Utils/BindlessRS.hlsl"
static const float GAMMA = 0.454545455f;
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 worldPosition : POSITION;
};

cbuffer Resources : register(b0)
{
    uint geometryID;
    uint modelConstantID;
    uint textureID;
};

[RootSignature(RootSig)]
float4 main(VSOutput input) : SV_Target
{
    TextureCube environmentTexture = ResourceDescriptorHeap[textureID];
    float3 samplingVector = normalize(input.worldPosition.xyz);

    float4 color = environmentTexture.SampleLevel(linearWrap, samplingVector, 0.0);
    return pow(color, GAMMA);
}