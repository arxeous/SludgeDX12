#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif


static const float GAMMA = 0.454545455f;
// Include structures and functions for lighting.
#include "LightingUtils.hlsl"
#include "BindlessRS.hlsl"

struct VertexData
{
    float3 position;
    float3 normal;
    float2 texCoord;
    float4 tangent;
};

struct VSOutput
{
    float4 positionH : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float2 texCoord : TEXCOORD;
    float4 tangent : TANGENT;
    //float3x3 TBN : TBN_MATRIX;
    float3x3 world : MODEL_MATRIX;
};

struct PassConstant
{
    float4 EyePosW;
    float4 AmbientLight;
    Light Lights[MaxLights];
};

struct ModelConstants
{
    matrix World;
    matrix ViewProj;
    matrix InvWorld;
    float4 albedo;
};

cbuffer Resources : register(b0)
{
    uint albedoID;
    uint roughnessID;
    uint geometryID;
    uint passConstantID;
    uint modelConstantID;
    uint irradianceID;
    uint prefilteredID;
    uint lutID;
    uint normalID;
    uint emissiveID;
    uint aoID;
};





