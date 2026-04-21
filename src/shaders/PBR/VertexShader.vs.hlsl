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
    
    //float3 T = mul(transpose((float3x3) modelConstant.InvWorld), input.tangent.xyz);
    //float3 B = cross(input.normal, T);
    //B = mul(transpose((float3x3) modelConstant.InvWorld), B) * input.tangent.w;
    //float3 N = output.normalW;
    //N = normalize(N);
    //T = normalize(T - dot(T, N) * N);
    
    //output.TBN = float3x3(T,B,N);
    output.world = (float3x3) modelConstant.World;
    return output;
}