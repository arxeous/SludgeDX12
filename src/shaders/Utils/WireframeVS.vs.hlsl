#define SHADER_MODEL_6_6
#include "BindlessRS.hlsl"

cbuffer Resources : register(b0)
{
    uint VertexBuffID;
    uint modelConstantID;
};

// technically we dont use most of these data members. But the structured buffer that holds all the data is for vertices of this structure.
// Id rather deal with some memory overhead than 40 different vertex types that all occupy varrying memory sizes. Same applies for the model constants. 
struct VertexData
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float4 tangent;
};

struct ModelConstants
{
    matrix World;
    matrix ViewProj;
    matrix InvWorld;
    float4 albedo;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

[RootSignature(RootSig)]
VSOutput main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<VertexData> verts = ResourceDescriptorHeap[VertexBuffID];
    ConstantBuffer<ModelConstants> modelConstant = ResourceDescriptorHeap[modelConstantID];
    VSOutput output;
    VertexData input = verts[vertexID];
    
    matrix mvp = mul(modelConstant.ViewProj, modelConstant.World);

    // Transform to homogeneous clip space.
    output.position = mul(mvp, float4(input.position, 1.0f));
    output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
	
    return output;
}