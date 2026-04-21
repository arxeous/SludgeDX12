#define SHADER_MODEL_6_6
#include "../Utils/BindlessRS.hlsl"

cbuffer Resources : register(b0)
{
    uint outputTexID;
};
static const float PI = 3.14159265359;
static const float TwoPI = 2 * PI;
static const float Epsilon = 0.001;
static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

// Compute Van der Corput radical inverse
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Sample i-th point from Hammersley point set of NumSamples points total.
float2 sampleHammersley(uint i)
{
    return float2(i * InvNumSamples, radicalInverse_VdC(i));
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// This returns normalized half-vector between Li & Lo.
// For derivation see: http://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html
float3 sampleGGX(float u1, float u2, float roughness)
{
    float alpha = roughness * roughness;

    float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha * alpha - 1.0) * u2));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // Trig. identity
    float phi = TwoPI * u1;

	// Convert to Cartesian upon return.
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method (IBL version).
float gaSchlickGGX_IBL(float cosLi, float cosLo, float roughness)
{
    float r = roughness;
    float k = (r * r) / 2.0; // Epic suggests using this roughness remapping for IBL lighting.
    return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}


// NOTE: https://bruop.github.io/ibl/
// This above article on IBL is the best explanation that you can find why the below is sampled and structured the way it is.
// Give it a thourough read and youll be amazed at how clear things are working here, along with the original Epic games paper:
// https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
[numthreads(32, 32, 1)]
void main(uint2 ThreadID : SV_DispatchThreadID)
{
    
    RWTexture2D<float2> LUT = ResourceDescriptorHeap[outputTexID];
    float outputWidth, outputHeight;
    LUT.GetDimensions(outputWidth, outputHeight);

    // n dot v can be adressed as cos(theta), its just a naming convention
    // our LUT is in the range of [0,1] on both the x and y axis, which means that in order to 
    // to get these values from a texture x, y we divide by the width and height of the texture.
    float nDotV = ThreadID.x / outputWidth;
    float roughness = ThreadID.y / outputHeight;
    nDotV = max(nDotV, Epsilon);

    // The view direction can be reconstructed 
    float3 V = float3(sqrt(1.0 - nDotV * nDotV), 0.0, nDotV);

    float DFG1 = 0;
    float DFG2 = 0;

    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 u = sampleHammersley(i);

		// Sample directly in tangent/shading space since we don't care about reference frame as long as it's consistent.
        // i.e. there is no conversion to world space. it stays in tangent space!
        float3 H = sampleGGX(u.x, u.y, roughness);

		// Compute incident direction (Li) by reflecting viewing direction (V) around half-vector (H).
        float3 Li = 2.0 * dot(V, H) * H - V;

         // Because we assume that N is just the normal of (0,0,1) the only portion that remains after a dot product is the z component.
        float nDotL = Li.z;
        float nDotH = H.z;
        float vDotH = saturate(dot(V, H));

        if (nDotL > 0.0)
        {
            float G = gaSchlickGGX_IBL(nDotL, nDotV, roughness);
            float Gv = G * vDotH / (nDotH * nDotV);
            float Fc = pow(1.0 - vDotH, 5);

            DFG1 += (1 - Fc) * Gv;
            DFG2 += Fc * Gv;
        }
    }

    LUT[ThreadID.xy] = float2(DFG1, DFG2) * InvNumSamples;
}
