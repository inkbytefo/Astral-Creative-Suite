#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

// UBO'yu değiştirip sadece view ve proj matrislerini içerecek şekilde güncelleyin
layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
} cameraData;

// Push constant bloğunu ekleyin
layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
} pushConstants;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = cameraData.proj * cameraData.view * pushConstants.modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
}