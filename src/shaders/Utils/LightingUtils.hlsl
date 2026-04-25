//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 16

#ifndef DIELECTRIC_SPECULAR
#define DIELECTRIC_SPECULAR 0.04
#endif

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // directional/spot light only
    float FalloffEnd; // point/spot light only
    float3 Position; // point light only
    float SpotPower; // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float Metallicness;
    float Roughness;
    float3 Irradiance;
    float3 Radiance;
    float2 LUT;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

static const float MIN_FLOAT_VALUE = 0.0000001f;
static const float PI = 3.14159265f;

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
// compared to normal formulations of the equation, this one uses n dot v rather than using the half vector.
// This is because it is the GLOBAL illumination version of the equation, and often times youll find both being used
// depending on the context of the lighting.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float3 FresnelSchlickLocal(float3 F0, float3 viewDir, float3 halfWayDir)
{
    float cosIncidentAngle = saturate(dot(viewDir, halfWayDir));
    return F0 + (1 - F0) * pow(saturate(1.0f - cosIncidentAngle), 5);
}

float3 FresnelSchlickRoughness(float3 F0, float3 viewDir, float3 halfWayDir, float roughnessFactor)
{
    return F0 + (max((float3(1.0f - roughnessFactor, 1.0f - roughnessFactor, 1.0f - roughnessFactor) - F0), F0) - F0) * pow(saturate(1 - dot(viewDir, halfWayDir)), 5.0f);

}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = (1 - mat.Roughness)  * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(F0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// A lot of the current model for PBR we get from this wonderful paper. 
// Like roughness being remapped to (roughness + 1 / 2) before squaring.
// It should be noted however this change is for analytical lighting only. i.e. for lights in the scene. IBl does not use this change.
// Both of these needed to be changed to account for division by 0, otherwise wed get wonky values when we sample from the roughness map.
// Trowbridge - Reitz or GGX is our normal disctribution function for cook torrance ! i.e. D
float GGX_Distribution(float3 normal, float3 halfVec, float roughnessFactor)
{
    float alpha = pow(roughnessFactor, 2);
    float alphaSquare = pow(alpha, 2);

    float nDotH = saturate(dot(normal, halfVec));
    return alphaSquare / max((PI * pow(pow(nDotH, 2) * (alphaSquare - 1) + 1, 2)), MIN_FLOAT_VALUE);
}

// For our shadowing function we are using shlick beckman,, but at some point I want to swtich over to smith
float SchlickBeckmannGS(float roughnessFactor, float3 normal, float3 X)
{
    roughnessFactor = pow((roughnessFactor + 1), 2);
    float k = roughnessFactor / 8;
    
    float nDotX = saturate(dot(normal, X));
    return nDotX / max((nDotX * (1 - k) + k), MIN_FLOAT_VALUE);
}

// When we say cook torrance what we really mean is that we are using the cook torrance BRDF as a function that is to be plugged
// into the entire rendering equation as a whole. WE do that below, but just remember its really just one portion of a bigger whole.
float3 CookTorrance(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    
    float3 halfVec = normalize(toEye + lightVec);
    // F0 in the context of the Fresnel Schlick approx can be best summed up as, the F0 is what youd see if you were viewing the material
    // direct on with the light !
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    float3 albedo = mat.DiffuseAlbedo.xyz;
    F0 = lerp(F0, albedo, mat.Metallicness);
    
    float3 D = GGX_Distribution(normal, halfVec, mat.Roughness);
    float3 G = SchlickBeckmannGS(mat.Roughness, normal, toEye) * SchlickBeckmannGS(mat.Roughness, normal, lightVec);
    float3 F = FresnelSchlickLocal(F0, toEye, halfVec);
    
    float3 kS = F;
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), mat.Metallicness);
    kD *= (1.0 - mat.Metallicness);
    
    // WE are essentially following the old approx that we do with blinn phong. 
    // The full BRDF is the diffuse brdf plus the specular brdf, and thats why we have the addition between the two.
    // kD in this context is still apart of that diffuse term, we just go through some extra steps to calculate it.
    // so all in all, its not as confusing as it seems. the big BRDF combines the specular and diffuse brdfs, 
    // with the specular BRDF being the cook torrance model, and the diffuse being the normal lambertian model /PI that weve seen before.
    // Then the light strength just being n dot l * light(Li) means that weve taken care of two birds one stone, the only thing left is to add the ambient
    // term to get the full color.
    float3 lambertianDiffuse = albedo * kD / PI;
    float3 specular = (D * G * F) / max(4.0f * max(dot(normal, toEye), 0.0f) * max(dot(normal, lightVec), 0.0f), MIN_FLOAT_VALUE);
    float3 BRDF = lambertianDiffuse * kD + specular;
    
    float3 outgoinglight = BRDF * lightStrength;
    
    return outgoinglight;
    
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // The light vector aims opposite the direction the light rays travel.
    float3 lightVec = normalize(-L.Direction);

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return CookTorrance(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for point lights.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return CookTorrance(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for spot lights.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > L.FalloffEnd)
        return 0.0f;

    // Normalize the light vector.
    lightVec /= d;

    // Scale light down by Lambert's cosine law.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // Attenuate light by distance.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // Scale by spotlight
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return CookTorrance(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    // Cook-Torrance BRDF
    float3 result = 0.0f;

    int i = 0;

#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif

#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 
    
    // In order to avoid adding the ambient lighting over and over again, we have to move alot of the calculations out of the light calculation functions
    // which means we do some double calculations( f0 and lambertian diffuse). worth the cost though as if we kept it in wed get some very bad looking results the more lights in the scene there are.
    float3 F0 = float3(DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR);
    float3 albedo = mat.DiffuseAlbedo.xyz;
    // gltf specs
    F0 = lerp(F0, albedo, mat.Metallicness);
    float3 LR = reflect(-toEye, normal);
    float3 halfVec = normalize(toEye + LR);
    //float VoH = saturate(dot(toEye, halfVec));
    float NoV = clamp(dot(normal, toEye), 1e-5, 1.0);
    float3 diffuseColor = albedo * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - mat.Metallicness);

    // IBL lighting
    // https://www.jcgt.org/published/0008/01/03/paper.pdf 
    // We use the above to more accurately model metals by accounting for the multiple scattering that occurs when materials become rougher
    // i.e. fixing metals getting darker the rougher they get.
    float3 ks = F0;
    float3 Fr = max(float3(1.0 - mat.Roughness, 1.0 - mat.Roughness, 1.0 - mat.Roughness), F0) - F0;
    ks += Fr * pow(1.0 - NoV, 5.0);
    
    float3 FssEss = ks * mat.LUT.x + mat.LUT.y;
    float Ess = mat.LUT.x + mat.LUT.y;
    float Ems = 1.0 - Ess;
    float3 Favg = F0 + (1.0 - F0) / 21.0;
    float3 Fms = FssEss * Favg / (1.0 - (1.0 - Ess) * Favg);
    
    // Energy of diffuse single scatter
    float3 Edss = 1 - (FssEss + Fms * Ems);
    float3 kd = diffuseColor * Edss;
    
    float3 IBL = FssEss * mat.Radiance + (Fms * Ems + kd) * mat.Irradiance;
    float3 lambertianDiffuse = albedo / PI;
    
    // ambient lighting
    float3 kS = FresnelSchlickRoughness(F0, toEye, normal, mat.Roughness);
    float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), mat.Metallicness);
    float3 ambient = kD * mat.Irradiance * lambertianDiffuse;

    return float4(IBL, 1.0);
    
    //// TEST
    //float3 F0 = float3(DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR);
    //float3 albedo = mat.DiffuseAlbedo.xyz;
    //// gltf specs
    //F0 = lerp(F0, albedo, mat.Metallicness);
    //float cosLO = clamp(dot(normal, toEye), 1e-5, 1.0);
    //float3 LR = reflect(-toEye, normal);

    //float3 kS = FresnelSchlickRoughness(F0, toEye, normal, mat.Roughness);
    //float3 kD = lerp(float3(1.0f, 1.0f, 1.0f) - kS, float3(0.0f, 0.0f, 0.0f), mat.Metallicness);

    //float3 F = kS;
    
    //float3 diffuseIBL = mat.Irradiance * albedo;

    //float3 specularIBL = mat.Radiance * (F0 * mat.LUT.x + mat.LUT.y);
    
    //float3 outgoingLight = (specularIBL + diffuseIBL * kD);

    
    //return float4(outgoingLight, 1.0f);
}


