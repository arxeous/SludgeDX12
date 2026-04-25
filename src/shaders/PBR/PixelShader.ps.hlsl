#define SHADER_MODEL_6_6
#include "../Utils/Utils.hlsl"
float3 ReconstructNormal(float3 worldPos, float3 worldNormal, float2 uv, Texture2D normalMap, SamplerState samp)
{
    // Screen space derivatives of position and uv
    // this formula to get our TBN using these small diff can be found here: https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/
    float3 dp1 = ddx(worldPos);
    float3 dp2 = ddy(worldPos);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);

    // Solve for tangent and bitangent
    float3 N = normalize(worldNormal);
    
    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
    
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // Normalize and construct TBN
    float invMax = rsqrt(max(dot(T, T), dot(B, B)));
    float3x3 TBN = float3x3(T * invMax, B * invMax, N);

    // Sample and transform
    float3 sampledNormal = normalMap.Sample(samp, uv).rgb * 2.0 - 1.0;
    
    return normalize(mul(sampledNormal, TBN));
    //return T * invMax;
}

// TO do our gamma correction, we have to gamma correct two places. The texture(FOR COLOR TEXTURE INFO ONLY)
// where we get our diffuse lighting information and the calculation we do for our lighting. i.e. Cook Torrance.
[RootSignature(RootSig)]
float4 main(VSOutput input) : SV_Target
{
    Texture2D testTexture = ResourceDescriptorHeap[albedoID];
    Texture2D testRoughness = ResourceDescriptorHeap[roughnessID];

    TextureCube<float4> irradianceMap = ResourceDescriptorHeap[irradianceID];
    TextureCube<float4> radianceMap = ResourceDescriptorHeap[prefilteredID];
    Texture2D LUT = ResourceDescriptorHeap[lutID];
    ConstantBuffer<PassConstant> passConstant = ResourceDescriptorHeap[passConstantID];
    ConstantBuffer<ModelConstants> modelConstant = ResourceDescriptorHeap[modelConstantID];
    
    float4 diffuseAlbedo = testTexture.Sample(linearWrap, input.texCoord) * modelConstant.albedo;
    float4 roughMetal = testRoughness.Sample(linearWrap, input.texCoord);
    float roughness = roughMetal.g;
    float metalness = roughMetal.b;
    float4 emissiveLight = (0.0f, 0.0f, 0.0f, 0.0f);

     // Interpolating normal can unnormalize it, so renormalize it.
    input.normalW = normalize(input.normalW);

    //float3 N = ReconstructNormal(input.positionW, input.normalW, input.texCoord, normalMap, anisotropic);
    
    float3 N = input.normalW;
    float3 bitangent;
    if (normalID != 0)
    {

        float3 T = input.tangent.xyz;
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalID];
        bitangent = normalize(cross(input.normalW, T)) * -input.tangent.w;
        //T = normalize(T - dot(T, input.normalW) * input.normalW);
        float3x3 tbn = float3x3(T, bitangent, input.normalW);
        
        N = normalTexture.Sample(anisotropic, input.texCoord).rgb * 2.0 - 1.0;
        //return float4(normalTexture.Sample(anisotropic, input.texCoord).rgb, 1.0);
        N = normalize(mul(N, tbn));
    }
    
    if(emissiveID != 0)
    {
        Texture2D emissiveMap = ResourceDescriptorHeap[emissiveID];
        emissiveLight = emissiveMap.Sample(anisotropic, input.texCoord);
    }
    
    // Vector from point being lit to eye. 
    float3 toEyeW = normalize(passConstant.EyePosW.xyz - input.positionW);
    float distToEye = length(toEyeW);

    // Ambient light is now calculated in the compute lighting calculation using IBl.
    //float4 ambient = passConstant.AmbientLight * diffuseAlbedo;

    // Specular BRDF
    uint textureWidth, textureHeight, mipmapLevels;
    radianceMap.GetDimensions(0u, textureWidth, textureHeight, mipmapLevels);

    float nDotV = saturate(dot(N, toEyeW));
    float3 L = reflect(-toEyeW, N);
    float preceptualRoughnessToLOD = mipmapLevels * roughness;
    // Sufficiently small roughness will actually mess up our IBL calculations that accounts for 
    // furnacing of metals. so we clamp it here. 
    if (roughness <= 0.015)
    {
        roughness = 0.025;
        preceptualRoughnessToLOD = mipmapLevels * roughness;
    }

    Material mat = { diffuseAlbedo, metalness, roughness, 0, 0, 0, 0, 0, 0, 0, 0 };
    mat.Irradiance = irradianceMap.SampleLevel(anisotropic, N, 0.0f).xyz;
    mat.Radiance = radianceMap.SampleLevel(anisotropic, L, preceptualRoughnessToLOD).xyz;
    mat.LUT = LUT.Sample(anisotropic, float2(nDotV, roughness)).xy;
   
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(passConstant.Lights, mat, input.positionW,
        N, toEyeW, shadowFactor);


    float4 litColor =  directLight ;
    litColor = pow(litColor, GAMMA);
    litColor.a = diffuseAlbedo.a;
    
    //float3 test = ReconstructNormal(input.positionW, input.normalW, input.texCoord, normalMap, anisotropic);
    
    return emissiveLight + litColor;
    //return float4(bitangent, 1.0f);

}