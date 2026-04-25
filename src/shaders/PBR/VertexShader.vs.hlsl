#define SHADER_MODEL_6_6
#include "../Utils/Utils.hlsl"

[RootSignature(RootSig)]
VSOutput main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<VertexData> verts = ResourceDescriptorHeap[geometryID];
    VSOutput output;
    VertexData input = verts[vertexID];
    ConstantBuffer<PassConstant> passConstant = ResourceDescriptorHeap[passConstantID];
    ConstantBuffer<ModelConstants> modelConstant = ResourceDescriptorHeap[modelConstantID];
    
    float4 posW = mul(modelConstant.World, float4(input.position, 1.0f));
    output.positionW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    output.normalW = mul(transpose((float3x3) modelConstant.InvWorld), input.normal);

    // Transform to homogeneous clip space.
    output.positionH = mul(modelConstant.ViewProj, posW);
	
	// Output vertex attributes for interpolation across triangle.
    output.texCoord = input.texCoord;
    output.tangent.w = input.tangent.w;
    output.tangent.xyz = mul(transpose((float3x3) modelConstant.InvWorld), input.tangent.xyz);
    
    output.world = (float3x3) modelConstant.World;
    return output;
}