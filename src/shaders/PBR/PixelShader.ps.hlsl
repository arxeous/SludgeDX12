#define SHADER_MODEL_6_6
#include "../Utils/Utils.hlsl"
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
    float3 emissiveLight = (0, 0, 0);
    float3 aoColor = (1, 1, 1);

     // Interpolating normal can unnormalize it, so renormalize it.
    input.normalW = normalize(input.normalW);
    float3 N = input.normalW;
    
    if (normalID != 0)
    {
        float3 T = input.tangent.xyz;
        float3 bitangent;
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[normalID];
        bitangent = normalize(cross(N, T)) * -input.tangent.w;
        float3x3 tbn = float3x3(T, bitangent, N);
        
        float3 mapNormal = normalTexture.Sample(anisotropic, input.texCoord).rgb * 2.0 - 1.0;
        N = normalize(mul(mapNormal, tbn));
    }
    
    if(emissiveID != 0)
    {
        Texture2D emissiveMap = ResourceDescriptorHeap[emissiveID];
        emissiveLight = emissiveMap.Sample(anisotropic, input.texCoord);
    }
    
    if(aoID != 0)
    {
        Texture2D aoMap = ResourceDescriptorHeap[emissiveID];
        aoColor = aoMap.Sample(anisotropic, input.texCoord);
    }
    
    float3 toEyeW = normalize(passConstant.EyePosW.xyz - input.positionW);
    float distToEye = length(toEyeW);
   
    uint textureWidth, textureHeight, mipmapLevels;
    radianceMap.GetDimensions(0u, textureWidth, textureHeight, mipmapLevels);

    float nDotV = saturate(dot(N, toEyeW));
    float3 L = reflect(-toEyeW, N);
    float preceptualRoughnessToLOD = mipmapLevels * roughness;
    // Sufficiently small roughness will actually mess up our IBL calculations that accounts for 
    // furnacing of metals. so we clamp it here. It looks weird but it just looks best when its set like so.
    if (roughness <= 0.015)
    {
        roughness = 0.025;
        preceptualRoughnessToLOD = mipmapLevels * roughness;
    }

    Material mat;
    mat.DiffuseAlbedo = diffuseAlbedo;
    mat.Metallicness = metalness;
    mat.Roughness = roughness;
    mat.AO = aoColor;
    mat.Irradiance = irradianceMap.SampleLevel(anisotropic, N, 0.0f).xyz;
    mat.Radiance = radianceMap.SampleLevel(anisotropic, L, preceptualRoughnessToLOD).xyz;
    mat.LUT = LUT.Sample(anisotropic, float2(nDotV, roughness)).xy;
   
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(passConstant.Lights, mat, input.positionW,
        N, toEyeW, shadowFactor);


    float4 litColor =  directLight ;
    litColor = pow(litColor, GAMMA);
    litColor.a = diffuseAlbedo.a;

    return float4(emissiveLight, 0) + litColor;
}