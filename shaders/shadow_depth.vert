#version 460

// Shadow mapping depth-only vertex shader
// Only needs position input and world matrix transformation

layout(push_constant) uniform PushConstants {
    mat4 lightViewProj;  // Combined light view-projection matrix
    mat4 model;          // Object model matrix
} pc;

// Only position input is needed for depth-only rendering
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;    // Required for vertex format compatibility but unused
layout(location = 2) in vec3 inNormal;   // Required for vertex format compatibility but unused
layout(location = 3) in vec2 inTexCoord; // Required for vertex format compatibility but unused

void main() {
    // Transform vertex to light clip space
    gl_Position = pc.lightViewProj * pc.model * vec4(inPosition, 1.0);
}
