#define RootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED), " \
    "RootConstants(b0, num32BitConstants=64, visibility = SHADER_VISIBILITY_ALL), " \
    "StaticSampler(s0, " \
                  "filter=FILTER_MIN_MAG_MIP_POINT, " \
                  "addressU=TEXTURE_ADDRESS_CLAMP, " \
                  "addressV=TEXTURE_ADDRESS_CLAMP, " \
                  "addressW=TEXTURE_ADDRESS_CLAMP, " \
                  "visibility=SHADER_VISIBILITY_ALL), " \
    "StaticSampler(s1, " \
                  "filter=FILTER_MIN_MAG_MIP_POINT, " \
                  "addressU=TEXTURE_ADDRESS_WRAP, " \
                  "addressV=TEXTURE_ADDRESS_WRAP, " \
                  "addressW=TEXTURE_ADDRESS_WRAP, " \
                  "mipLODBias=0.0f, " \
                  "maxAnisotropy=0, " \
                  "comparisonFunc=COMPARISON_NEVER, " \
                  "borderColor=STATIC_BORDER_COLOR_OPAQUE_WHITE, " \
                  "minLOD=0.0f, " \
                  "maxLOD=3.402823466e+38f, " \
                  "visibility=SHADER_VISIBILITY_ALL)," \
    "StaticSampler(s2, " \
                  "filter=FILTER_ANISOTROPIC, " \
                  "maxAnisotropy = 16), " \
    "StaticSampler(s3, " \
                  "filter=FILTER_MIN_MAG_MIP_LINEAR, " \
                  "addressU=TEXTURE_ADDRESS_WRAP, " \
                  "addressV=TEXTURE_ADDRESS_WRAP, " \
                  "addressW=TEXTURE_ADDRESS_WRAP), " \
    "StaticSampler(s4, " \
                  "filter=FILTER_MIN_MAG_MIP_LINEAR, " \
                  "addressU=TEXTURE_ADDRESS_CLAMP, " \
                  "addressV=TEXTURE_ADDRESS_CLAMP, " \
                  "addressW=TEXTURE_ADDRESS_CLAMP), " 


SamplerState pointClamp : register(s0);
SamplerState pointWrap : register(s1);
SamplerState anisotropic : register(s2);
SamplerState linearWrap: register(s3);
SamplerState linearClamp : register(s4);