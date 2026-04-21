#include "../Utils/BindlessRS.hlsl"
struct VertexData
{
    float3 pos;
    float3 normal;
    float2 texCoord;
    float4 tangent;
};
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

struct ModelConstants
{
    matrix gWorld;
    matrix gViewProj;
    matrix gInvWorld;
};

[RootSignature(RootSig)]
VSOutput main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<VertexData> verts = ResourceDescriptorHeap[geometryID];
    VSOutput output;
    VertexData input = verts[vertexID];
    ConstantBuffer<ModelConstants> modelConstant = ResourceDescriptorHeap[modelConstantID];
    
    // vector that will be used to sample into the cubemap texture is just the position of the cube vertex which then gets interpolated across the cube faces.
    output.worldPosition = float4(input.pos, 1.0f);
    
    output.position = mul(modelConstant.gViewProj, float4(input.pos, 0.0f));
    output.position.z = output.position.w;
    return output;
}



