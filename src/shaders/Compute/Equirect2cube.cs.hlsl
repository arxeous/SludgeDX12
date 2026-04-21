#define SHADER_MODEL_6_6
#include "../Utils/BindlessRS.hlsl"
static const float PI = 3.141592;
static const float TwoPI = 2 * PI;
cbuffer Resources : register(b0)
{
    uint inputTexID;
    uint outputTexID;
};

// https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/equirect2cube.hlsl

float3 getSamplingVector(uint3 ThreadID, RWTexture2DArray<float4> outputTexture)
{
    float outputWidth, outputHeight, outputDepth;
    outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);

    // Use the thread ID which is effectively mapped to pixel positions on our cube map face
    // to get a normalized uv position.
    // https://www.jeremyong.com/graphics/2023/08/26/dispatch-ids-and-you/
    // Refresher for dispatch ids.
    float2 st = ThreadID.xy / float2(outputWidth, outputHeight);
    // The uv position is then rescaled to be in a range of -1 to 1. This position is in a "local space".
    // and is not actually our REAL uv. We just use -1 to 1 because the math is easier to get our real uv for 
    // texture sampling.
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - float2(1.0, 1.0);

	// Select vector based on cubemap face index.
    float3 ret;
    switch (ThreadID.z)
    {
        case 0:
            ret = float3(1.0, uv.y, -uv.x);
            break;
        case 1:
            ret = float3(-1.0, uv.y, uv.x);
            break;
        case 2:
            ret = float3(uv.x, 1.0, -uv.y);
            break;
        case 3:
            ret = float3(uv.x, -1.0, uv.y);
            break;
        case 4:
            ret = float3(uv.x, uv.y, 1.0);
            break;
        case 5:
            ret = float3(-uv.x, uv.y, -1.0);
            break;
    }
    return normalize(ret);
}

[RootSignature(RootSig)]
[numthreads(32, 32, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
    Texture2D inputTexture = ResourceDescriptorHeap[inputTexID];
    RWTexture2DArray<float4> outputCube = ResourceDescriptorHeap[outputTexID];
    float3 v = getSamplingVector(ThreadID, outputCube);
	// Convert Cartesian direction vector to spherical coordinates.
    float phi = atan2(v.z, v.x);
    float theta = acos(v.y);

	// Sample equirectangular texture.
    float4 color = inputTexture.SampleLevel(linearWrap, float2(phi / TwoPI, theta / PI), 0);

	// Write out color to output cubemap.
    outputCube[ThreadID] = color;
}