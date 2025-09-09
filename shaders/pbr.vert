#version 450

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

// Uniform buffers
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
} scene;

layout(set = 2, binding = 0) uniform ShadowUBO {
    mat4 lightSpaceMatrices[4];
    vec4 cascadeDistances;
    vec4 shadowParams;
    uvec4 shadowConfig;
    vec3 lightDirection;
} shadow;

// Push constants for per-object data
layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

// Outputs
layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragTangent;
layout(location = 4) out vec3 fragBitangent;
layout(location = 5) out vec3 fragViewPos;
layout(location = 6) out vec4 fragShadowCoords[4]; // Shadow coordinates for each cascade

void main() {
    // Calculate world position
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    
    // Transform to clip space
    gl_Position = scene.proj * scene.view * worldPos;
    
    // Transform normal and tangent to world space
    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    fragTangent = normalize(normalMatrix * inTangent);
    fragBitangent = cross(fragNormal, fragTangent);
    
    // Pass through texture coordinates
    fragTexCoord = inTexCoord;
    
    // Calculate view position
    fragViewPos = (scene.view * worldPos).xyz;
    
    // Calculate shadow coordinates for each cascade
    for (int i = 0; i < 4; ++i) {
        fragShadowCoords[i] = shadow.lightSpaceMatrices[i] * worldPos;
    }
}
