#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "include/common.glsl"

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vNormalWS;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vColor;

void main() {
    mat4 vp = ubo.proj * ubo.view;
    gl_Position = vp * pc.model * vec4(inPosition, 1.0);
    vNormalWS = mat3(pc.model) * inNormal;
    vUV = inTexCoord;
    vColor = inColor;
}

