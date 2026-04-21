#include "../Utils/BindlessRS.hlsl"
#define SHADER_MODEL_6_6

cbuffer Resources : register(b0)
{
    uint VertexBuffID;
    uint textureID;
    uint exposureID;
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


[RootSignature(RootSig)]
VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    StructuredBuffer<VertexData> verts = ResourceDescriptorHeap[VertexBuffID];
    output.position = float4(verts[vertexID].position.xy, 0.0f, 1.0f);
    output.texCoord = verts[vertexID].texCoord;

    return output;
}