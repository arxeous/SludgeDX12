#define SHADER_MODEL_6_6
#include "../Utils/BindlessRS.hlsl"

// TODO: Create mip map compute shader to get mip map for skybox texture,
// then use this along with the brdfs PDF (n(h) * 1/4(n*v) to sample from the skybox and get
// better looking results for higher roughness. Basically just some extra clean up.
static const float PI = 3.14159265359;
// Most of this can be found here:
// https://learnopengl.com/PBR/IBL/Specular-IBL and https://www.reedbeta.com/blog/hows-the-ndf-really-defined/
// Great articles for learning how basic specular IBL can be implemented into your own engine.
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 N)
{
    float a = roughness * roughness;
    // Sample in spherical coordinates, derivation can be found here: https://www.tobias-franke.eu/log/2014/03/30/notes_on_importance_sampling.html
    // and https://agraphicsguynotes.com/posts/sample_microfacet_brdf/
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    // Construct tangent space vector
    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    // Tangent to world space
    float3 upVector = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangentX = normalize(cross(upVector, N));
    float3 tangentY = cross(N, tangentX);
    return tangentX * H.x + tangentY * H.y + N * H.z;
}

float3 getSamplingVector(uint3 ThreadID, RWTexture2DArray<float4> outputTexture)
{
    float outputWidth, outputHeight, outputDepth;
    outputTexture.GetDimensions(outputWidth, outputHeight, outputDepth);
    float2 st = ThreadID.xy / float2(outputWidth, outputHeight);
    float2 uv = (ThreadID.xy + float2(0.5f, 0.5f)) / outputWidth;
    uv = uv * float2(2.0f, 2.0f) - float2(1.0f, 1.0f);
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

cbuffer Resources : register(b0)
{
    uint cubeTexID;
    uint outputTexID;
    float gRoughness;
};

static const uint SAMPLE_COUNT = 4096u;


[RootSignature(RootSig)]
[numthreads(32, 32, 1)]
void main(uint3 ThreadID : SV_DispatchThreadID)
{
    TextureCube<float4> cubemap = ResourceDescriptorHeap[cubeTexID];
    RWTexture2DArray<float4> outputCube = ResourceDescriptorHeap[outputTexID];
    // recall that in the prefiltered calculation we assume that the viewing direction and the normal are the SAME. so its used everywhere the V would be in the intergration.
    // also note that the view direction is directly sampled from the cube map to get a dircetion. in the BRDF portion of split sum, the sampling direction is taken from nDotv
    // and reconstructed to get V.
    float3 normal = getSamplingVector(ThreadID, outputCube);
    normal = normalize(normal);
    float totalWeight = 0.0f;
    float3 preFilteredColor = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, gRoughness, normal);
        float3 L = normalize(2.0f * dot(normal, H) * H - normal);
        
        float NDotL = dot(normal, L);
        if(NDotL > 0.0f)
        {
            preFilteredColor += cubemap.SampleLevel(linearWrap, L, 0.0f).rgb * NDotL;
            totalWeight += NDotL;

        }
        
    }

    preFilteredColor = preFilteredColor / totalWeight;
    
    outputCube[ThreadID] = float4(preFilteredColor, 1.0f);
    
}