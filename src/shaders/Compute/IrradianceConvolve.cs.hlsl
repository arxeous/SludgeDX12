#define SHADER_MODEL_6_6
#include "../Utils/BindlessRS.hlsl"

cbuffer Resources : register(b0)
{
    uint cubeTexID;
    uint outputTexID;
};

static const float PI = 3.14159265359;
float3 getSamplingVector(uint3 ThreadID, RWTexture2DArray<float4> outputTexture)
{
    float outputWidth, outputHeight, outputDepth;
    outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);
    float2 st = ThreadID.xy / float2(outputWidth, outputHeight);
    float2 uv = (ThreadID.xy + float2(0.5f, 0.5f)) / outputWidth;
    uv = uv * float2(2.0f, 2.0f) - float2(1.0f, 1.0f);
    // The uv here isnt being used to sample anymore, its being used to reconstruct a world space direction!
    // because of this we need to flip the uv.y. also notice the uv is no longer from -1 to 1! This stems from the fact that this uv
    // is not some uv that will be used to sample from a texture anymore. in the equirect2cube compute shader, the uv was fine being in 
    // local space of the object. That wont fly here, we want things to be in world space to sample from the hemisphere.
    uv.y *= -1.0f;
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
    TextureCube<float4> cubemap = ResourceDescriptorHeap[cubeTexID];
    RWTexture2DArray<float4> outputCube = ResourceDescriptorHeap[outputTexID];
    float3 normal = getSamplingVector(ThreadID, outputCube);
    normal = normalize(normal);
    
    float3 forward = normal;
    float3 up = float3(0.0f, 0.0f, 1.0f);
    float3 right = normalize(cross(up, forward));
    up = normalize(cross(forward, right));
    
    static const float INTEGRATION_STEP_COUNT = 100.0f;
    static const float SAMPLES = INTEGRATION_STEP_COUNT * INTEGRATION_STEP_COUNT;
    
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    
    // Rather than doing a for loop on a float value we, like here: https://learnopengl.com/PBR/IBL/Diffuse-irradiance
    // we do it across a set amount of integration steps, and then use each step to calculate a phi and theta . (the same phi and theta that go through
    // 0 to 2pi and 0 to 1/pi respectively.
    for (float i = 0.0f; i < INTEGRATION_STEP_COUNT; i++)
    {
        float phi = 2.0f * PI * (i / (INTEGRATION_STEP_COUNT - 1.0f));
        for (float j = 0.0f; j < INTEGRATION_STEP_COUNT; j++)
        {
            float theta = 0.5f * PI * (j / (INTEGRATION_STEP_COUNT - 1.0f));
            float3 tangentSpaceCoordinates = normalize(float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)));
            float3 worldSpaceCoordinates = tangentSpaceCoordinates.x * right + tangentSpaceCoordinates.y * up + tangentSpaceCoordinates.z * forward;
            float3 irradianceValue = cubemap.SampleLevel(linearWrap, worldSpaceCoordinates, 0.0f).xyz;
            
            irradiance += irradianceValue * cos(theta) * sin(theta);
        }
    }
    
    outputCube[ThreadID] = float4(PI * irradiance / SAMPLES, 0.0f);
}