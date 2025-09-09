#version 450

// Inputs - FIXED: Removed inBitangent (location 5) since vertex data no longer provides it
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;     // Color at location 1 (from vertex data)
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inTangent;

// Outputs to fragment shader
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out vec3 fragViewPos;
layout(location = 6) out vec4 fragShadowCoords[4];

// Scene UBO (set = 0)
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
} scene;

// Shadow UBO (set = 2)
layout(set = 2, binding = 0) uniform ShadowUBO {
    mat4 lightSpaceMatrices[4];
    vec4 cascadeDistances;
    vec4 shadowParams; // bias, normalOffset, pcfRadius, strength
    uvec4 shadowConfig; // cascadeCount, filterMode, mapSize, debug
    vec3 lightDirection;
} shadow;

// Push constants for per-object transform
layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    // World space position
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;

    // Normal/Tangent/Bitangent transformation
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    fragTangent = normalize(normalMatrix * inTangent);
    // Calculate bitangent from normal and tangent (since it's not provided in vertex data anymore)
    fragBitangent = normalize(cross(fragNormal, fragTangent));

    // Pass UVs
    fragTexCoord = inTexCoord;

    // View space position (z for cascade selection in fragment)
    vec4 viewPos = scene.view * worldPos;
    fragViewPos = viewPos.xyz;

    // Shadow coordinates for cascades
    int cascadeCount = int(shadow.shadowConfig.x);
    cascadeCount = clamp(cascadeCount, 1, 4);
    for (int i = 0; i < cascadeCount; ++i) {
        fragShadowCoords[i] = shadow.lightSpaceMatrices[i] * worldPos;
    }
    // Zero any unused cascades
    for (int i = cascadeCount; i < 4; ++i) {
        fragShadowCoords[i] = vec4(0.0);
    }

    // Final clip space position
    gl_Position = scene.proj * viewPos;
}

