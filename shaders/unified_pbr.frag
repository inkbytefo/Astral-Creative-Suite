#version 450

// Configuration defines (will be added by MaterialShaderManager)
// Texture defines
// #define HAS_BASECOLOR_MAP
// #define HAS_METALLIC_ROUGHNESS_MAP 
// #define HAS_NORMAL_MAP
// #define HAS_OCCLUSION_MAP
// #define HAS_EMISSIVE_MAP
// #define HAS_ANISOTROPY_MAP
// #define HAS_CLEARCOAT_MAP
// #define HAS_CLEARCOAT_NORMAL_MAP
// #define HAS_CLEARCOAT_ROUGHNESS_MAP
// #define HAS_SHEEN_MAP
// #define HAS_SHEEN_COLOR_MAP
// #define HAS_SHEEN_ROUGHNESS_MAP
// #define HAS_TRANSMISSION_MAP
// #define HAS_VOLUME_MAP
// #define HAS_SPECULAR_MAP
// #define HAS_SPECULAR_COLOR_MAP

// Feature defines
// #define USE_ANISOTROPY
// #define USE_CLEARCOAT
// #define USE_SHEEN
// #define USE_TRANSMISSION
// #define USE_VOLUME
// #define USE_SPECULAR
// #define USE_VERTEX_COLORS
// #define USE_SECONDARY_UV
// #define UNLIT
// #define DOUBLE_SIDED
// #define RECEIVE_SHADOWS

// Alpha mode defines
// #define ALPHA_TEST
// #define ALPHA_BLEND

// Inputs from vertex shader
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragTangent;
layout(location = 4) in vec3 fragBitangent;
layout(location = 5) in vec3 fragViewPos;
layout(location = 6) in vec4 fragShadowCoords[4];

#ifdef USE_VERTEX_COLORS
layout(location = 7) in vec4 fragVertexColor;
#endif

#ifdef USE_SECONDARY_UV
layout(location = 8) in vec2 fragTexCoord2;
#endif

// Uniform buffers
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
} scene;

// Unified Material UBO - matches UnifiedMaterialUBO structure exactly
layout(set = 1, binding = 0) uniform UnifiedMaterialUBO {
    // Base PBR Properties (64 bytes)
    vec4 baseColor;                    // RGBA base color + opacity
    vec3 emissiveColor; float metallic; // RGB emissive + metallic factor
    vec3 specularColor; float roughness; // RGB specular + roughness factor
    vec4 normalOcclusionParams;         // normalScale, occlusionStrength, alphaCutoff, specularFactor

    // Advanced PBR Properties (80 bytes)
    vec4 anisotropyParams;              // anisotropy, anisotropyRotation, clearcoat, clearcoatRoughness
    vec4 sheenParams;                   // sheen, sheenRoughness, padding, padding
    vec3 sheenColor; float clearcoatNormalScale;
    vec4 transmissionParams;            // transmission, ior, thickness, volume
    vec4 volumeParams;                  // volumeThickness, volumeAttenuationDistance, padding, padding

    // Attenuation Colors (32 bytes)
    vec3 attenuationColor; float attenuationDistance;
    vec3 volumeAttenuationColor; float volumeAttenuationDistance;

    // Texture Coordinate Transforms (64 bytes)
    vec4 baseColorTilingOffset;         // tiling.xy, offset.xy
    vec4 normalTilingOffset;            // tiling.xy, offset.xy  
    vec4 metallicRoughnessTilingOffset; // tiling.xy, offset.xy
    vec4 emissiveTilingOffset;          // tiling.xy, offset.xy

    // Texture Indices (64 bytes) - for bindless textures (not used in this implementation)
    uvec4 textureIndices1;             // baseColor, metallicRoughness, normal, occlusion
    uvec4 textureIndices2;             // emissive, anisotropy, clearcoat, clearcoatNormal
    uvec4 textureIndices3;             // clearcoatRoughness, sheen, sheenColor, sheenRoughness
    uvec4 textureIndices4;             // transmission, volume, specular, specularColor

    // Material Flags and Settings (16 bytes)
    uvec4 materialFlags;               // textureFlags, featureFlags, alphaMode, renderingFlags
} material;

layout(set = 2, binding = 0) uniform ShadowUBO {
    mat4 lightSpaceMatrices[4];
    vec4 cascadeDistances;
    vec4 shadowParams; // bias, normalOffset, pcfRadius, strength
    uvec4 shadowConfig; // cascadeCount, filterMode, mapSize, debug
    vec3 lightDirection;
} shadow;

// Texture samplers - bound to specific slots
#ifdef HAS_BASECOLOR_MAP
layout(set = 1, binding = 1) uniform sampler2D baseColorTexture;
#endif

#ifdef HAS_METALLIC_ROUGHNESS_MAP
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
#endif

#ifdef HAS_NORMAL_MAP
layout(set = 1, binding = 3) uniform sampler2D normalTexture;
#endif

#ifdef HAS_OCCLUSION_MAP
layout(set = 1, binding = 4) uniform sampler2D occlusionTexture;
#endif

#ifdef HAS_EMISSIVE_MAP
layout(set = 1, binding = 5) uniform sampler2D emissiveTexture;
#endif

#ifdef HAS_ANISOTROPY_MAP
layout(set = 1, binding = 6) uniform sampler2D anisotropyTexture;
#endif

#ifdef HAS_CLEARCOAT_MAP
layout(set = 1, binding = 7) uniform sampler2D clearcoatTexture;
#endif

#ifdef HAS_CLEARCOAT_NORMAL_MAP
layout(set = 1, binding = 8) uniform sampler2D clearcoatNormalTexture;
#endif

#ifdef HAS_CLEARCOAT_ROUGHNESS_MAP
layout(set = 1, binding = 9) uniform sampler2D clearcoatRoughnessTexture;
#endif

#ifdef HAS_SHEEN_MAP
layout(set = 1, binding = 10) uniform sampler2D sheenTexture;
#endif

#ifdef HAS_SHEEN_COLOR_MAP
layout(set = 1, binding = 11) uniform sampler2D sheenColorTexture;
#endif

#ifdef HAS_SHEEN_ROUGHNESS_MAP
layout(set = 1, binding = 12) uniform sampler2D sheenRoughnessTexture;
#endif

#ifdef HAS_TRANSMISSION_MAP
layout(set = 1, binding = 13) uniform sampler2D transmissionTexture;
#endif

#ifdef HAS_VOLUME_MAP
layout(set = 1, binding = 14) uniform sampler2D volumeTexture;
#endif

#ifdef HAS_SPECULAR_MAP
layout(set = 1, binding = 15) uniform sampler2D specularTexture;
#endif

#ifdef HAS_SPECULAR_COLOR_MAP
layout(set = 1, binding = 16) uniform sampler2D specularColorTexture;
#endif

#ifdef RECEIVE_SHADOWS
layout(set = 2, binding = 1) uniform sampler2DArrayShadow shadowMap;
#endif

// Output
layout(location = 0) out vec4 outColor;

// Constants
const float PI = 3.14159265359;
const float EPSILON = 0.0001;
const float MIN_ROUGHNESS = 0.04;

// Material parameter accessors (inline functions for readability)
float getNormalScale() { return material.normalOcclusionParams.x; }
float getOcclusionStrength() { return material.normalOcclusionParams.y; }
float getAlphaCutoff() { return material.normalOcclusionParams.z; }
float getSpecularFactor() { return material.normalOcclusionParams.w; }

float getAnisotropy() { return material.anisotropyParams.x; }
float getAnisotropyRotation() { return material.anisotropyParams.y; }
float getClearcoat() { return material.anisotropyParams.z; }
float getClearcoatRoughness() { return material.anisotropyParams.w; }

float getSheen() { return material.sheenParams.x; }
float getSheenRoughness() { return material.sheenParams.y; }

float getTransmission() { return material.transmissionParams.x; }
float getIor() { return material.transmissionParams.y; }
float getThickness() { return material.transmissionParams.z; }
float getVolume() { return material.transmissionParams.w; }

// Texture coordinate computation
vec2 getTextureCoords(vec4 tilingOffset, vec2 baseUV) {
    return baseUV * tilingOffset.xy + tilingOffset.zw;
}

// Utility functions
vec3 getNormalFromMap() {
    vec3 normal = normalize(fragNormal);
    
#ifdef HAS_NORMAL_MAP
    vec2 normalUV = getTextureCoords(material.normalTilingOffset, fragTexCoord);
    vec3 tangentNormal = texture(normalTexture, normalUV).xyz * 2.0 - 1.0;
    tangentNormal.xy *= getNormalScale();
    
    vec3 N = normal;
    vec3 T = normalize(fragTangent);
    vec3 B = normalize(fragBitangent);
    
    // Handle degenerate cases
    if (dot(T, T) < EPSILON || dot(B, B) < EPSILON) {
        return normal;
    }
    
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
#else
    return normal;
#endif
}

#ifdef USE_CLEARCOAT
vec3 getClearcoatNormalFromMap() {
    vec3 normal = normalize(fragNormal);
    
#ifdef HAS_CLEARCOAT_NORMAL_MAP
    vec2 normalUV = getTextureCoords(material.normalTilingOffset, fragTexCoord);
    vec3 tangentNormal = texture(clearcoatNormalTexture, normalUV).xyz * 2.0 - 1.0;
    tangentNormal.xy *= material.clearcoatNormalScale;
    
    vec3 N = normal;
    vec3 T = normalize(fragTangent);
    vec3 B = normalize(fragBitangent);
    
    if (dot(T, T) < EPSILON || dot(B, B) < EPSILON) {
        return normal;
    }
    
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
#else
    return normal;
#endif
}
#endif

// PBR lighting functions
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Shadow calculation
float calculateShadow() {
#ifdef RECEIVE_SHADOWS
    // Determine which cascade to use based on view depth
    float viewDepth = -fragViewPos.z;
    int cascadeIndex = 0;
    
    for (int i = 0; i < int(shadow.shadowConfig.x) - 1; ++i) {
        if (viewDepth > shadow.cascadeDistances[i]) {
            cascadeIndex = i + 1;
        }
    }
    
    // Get shadow coordinates for the selected cascade
    vec4 shadowCoord = fragShadowCoords[cascadeIndex];
    shadowCoord.xyz /= shadowCoord.w;
    
    // Transform to [0,1] range
    shadowCoord.xyz = shadowCoord.xyz * 0.5 + 0.5;
    
    // Check if we're outside the shadow map
    if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 ||
        shadowCoord.y < 0.0 || shadowCoord.y > 1.0 ||
        shadowCoord.z < 0.0 || shadowCoord.z > 1.0) {
        return 1.0; // No shadow
    }
    
    // Sample shadow map with PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0).xy;
    float pcfRadius = shadow.shadowParams.z;
    
    for (float x = -pcfRadius; x <= pcfRadius; x += 1.0) {
        for (float y = -pcfRadius; y <= pcfRadius; y += 1.0) {
            vec2 offset = vec2(x, y) * texelSize;
            shadow += texture(shadowMap, vec4(shadowCoord.xy + offset, cascadeIndex, shadowCoord.z));
        }
    }
    
    float sampleCount = (pcfRadius * 2.0 + 1.0);
    sampleCount *= sampleCount;
    shadow /= sampleCount;
    
    return shadow;
#else
    return 1.0;
#endif
}

#ifdef USE_SHEEN
// Sheen BRDF
float distributionCharlie(float sheenRoughness, float NdotH) {
    float alphaG = sheenRoughness * sheenRoughness;
    float invR = 1.0 / alphaG;
    float cos2h = NdotH * NdotH;
    float sin2h = 1.0 - cos2h;
    return (2.0 + invR) * pow(sin2h, invR * 0.5) / (2.0 * PI);
}

float visibilityAshikhmin(float NdotL, float NdotV) {
    return clamp(1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV)), 0.0, 1.0);
}
#endif

void main() {
    // Sample base color
    vec4 baseColor = material.baseColor;
    
#ifdef HAS_BASECOLOR_MAP
    vec2 baseColorUV = getTextureCoords(material.baseColorTilingOffset, fragTexCoord);
    baseColor *= texture(baseColorTexture, baseColorUV);
#endif

#ifdef USE_VERTEX_COLORS
    baseColor *= fragVertexColor;
#endif

    // Alpha test
#ifdef ALPHA_TEST
    if (baseColor.a < getAlphaCutoff()) {
        discard;
    }
#endif

    // Early exit for unlit materials
#ifdef UNLIT
    vec3 emissive = material.emissiveColor;
#ifdef HAS_EMISSIVE_MAP
    vec2 emissiveUV = getTextureCoords(material.emissiveTilingOffset, fragTexCoord);
    emissive *= texture(emissiveTexture, emissiveUV).rgb;
#endif
    outColor = vec4(baseColor.rgb + emissive, baseColor.a);
    return;
#endif

    // Sample metallic-roughness
    float metallic = material.metallic;
    float roughness = max(material.roughness, MIN_ROUGHNESS);
    
#ifdef HAS_METALLIC_ROUGHNESS_MAP
    vec2 metallicRoughnessUV = getTextureCoords(material.metallicRoughnessTilingOffset, fragTexCoord);
    vec3 metallicRoughnessValues = texture(metallicRoughnessTexture, metallicRoughnessUV).rgb;
    metallic *= metallicRoughnessValues.b;  // Blue channel
    roughness *= metallicRoughnessValues.g; // Green channel
#endif
    
    roughness = max(roughness, MIN_ROUGHNESS);

    // Sample normal
    vec3 N = getNormalFromMap();
    vec3 V = normalize(scene.cameraPos - fragWorldPos);
    
    // Calculate F0 (base reflectance)
    vec3 F0 = mix(vec3(0.04), baseColor.rgb, metallic);
    
#ifdef USE_SPECULAR
    vec3 specularColor = material.specularColor;
    float specularFactor = getSpecularFactor();
    
#ifdef HAS_SPECULAR_MAP
    specularFactor *= texture(specularTexture, fragTexCoord).r;
#endif

#ifdef HAS_SPECULAR_COLOR_MAP
    specularColor *= texture(specularColorTexture, fragTexCoord).rgb;
#endif

    F0 = mix(F0, specularColor, specularFactor);
#endif

    // Sample occlusion
    float occlusion = 1.0;
#ifdef HAS_OCCLUSION_MAP
    occlusion = mix(1.0, texture(occlusionTexture, fragTexCoord).r, getOcclusionStrength());
#endif

    // Simple directional light (should be expanded for multiple lights)
    vec3 L = normalize(-shadow.lightDirection);
    vec3 H = normalize(V + L);
    
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    // Calculate BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(VdotH, F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * NdotV * NdotL + EPSILON;
    vec3 specular = numerator / denominator;
    
    // Calculate diffuse (Lambertian)
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic; // Metallic surfaces have no diffuse
    
    vec3 diffuse = kD * baseColor.rgb / PI;
    
    // Apply shadow
    float shadowFactor = calculateShadow();
    
    // Combine lighting
    vec3 radiance = vec3(1.0); // Light color/intensity (should come from uniform)
    vec3 Lo = (diffuse + specular) * radiance * NdotL * shadowFactor;
    
    // Add sheen
#ifdef USE_SHEEN
    float sheenAmount = getSheen();
    if (sheenAmount > EPSILON) {
        vec3 sheenColorValue = material.sheenColor;
        float sheenRoughness = getSheenRoughness();
        
#ifdef HAS_SHEEN_MAP
        sheenAmount *= texture(sheenTexture, fragTexCoord).r;
#endif

#ifdef HAS_SHEEN_COLOR_MAP
        sheenColorValue *= texture(sheenColorTexture, fragTexCoord).rgb;
#endif

#ifdef HAS_SHEEN_ROUGHNESS_MAP
        sheenRoughness *= texture(sheenRoughnessTexture, fragTexCoord).r;
#endif
        
        float sheenD = distributionCharlie(sheenRoughness, NdotH);
        float sheenV = visibilityAshikhmin(NdotL, NdotV);
        vec3 sheenBRDF = sheenColorValue * sheenD * sheenV;
        
        Lo += sheenBRDF * radiance * NdotL * shadowFactor * sheenAmount;
    }
#endif

    // Add clearcoat
#ifdef USE_CLEARCOAT
    float clearcoatAmount = getClearcoat();
    if (clearcoatAmount > EPSILON) {
        float clearcoatRoughness = max(getClearcoatRoughness(), MIN_ROUGHNESS);
        
#ifdef HAS_CLEARCOAT_MAP
        clearcoatAmount *= texture(clearcoatTexture, fragTexCoord).r;
#endif

#ifdef HAS_CLEARCOAT_ROUGHNESS_MAP
        clearcoatRoughness *= texture(clearcoatRoughnessTexture, fragTexCoord).r;
#endif
        
        vec3 clearcoatN = getClearcoatNormalFromMap();
        float clearcoatNdotV = max(dot(clearcoatN, V), 0.0);
        float clearcoatNdotL = max(dot(clearcoatN, L), 0.0);
        float clearcoatNdotH = max(dot(clearcoatN, H), 0.0);
        
        float clearcoatD = distributionGGX(clearcoatN, H, clearcoatRoughness);
        float clearcoatG = geometrySmith(clearcoatN, V, L, clearcoatRoughness);
        vec3 clearcoatF = fresnelSchlick(VdotH, vec3(0.04)); // Fixed F0 for clearcoat
        
        vec3 clearcoatSpecular = clearcoatD * clearcoatG * clearcoatF / 
                                (4.0 * clearcoatNdotV * clearcoatNdotL + EPSILON);
        
        Lo = mix(Lo, Lo * (1.0 - clearcoatF.x) + clearcoatSpecular * radiance * clearcoatNdotL * shadowFactor, 
                clearcoatAmount);
    }
#endif

    // Ambient lighting (should be IBL)
    vec3 ambient = vec3(0.03) * baseColor.rgb * occlusion;
    vec3 color = ambient + Lo;
    
    // Add emissive
    vec3 emissive = material.emissiveColor;
#ifdef HAS_EMISSIVE_MAP
    vec2 emissiveUV = getTextureCoords(material.emissiveTilingOffset, fragTexCoord);
    emissive *= texture(emissiveTexture, emissiveUV).rgb;
#endif
    color += emissive;
    
    // Tone mapping and gamma correction (simple Reinhard)
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, baseColor.a);
}
