#version 450

// Input from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 canvasColor;

// Texture sampler
layout(set = 0, binding = 0) uniform sampler2D texSampler;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Blend with fragment color
    vec4 finalColor = texColor * fragColor;
    
    // Blend with canvas color
    outColor = finalColor * canvasColor;
    
    // Handle alpha
    if (outColor.a < 0.01) {
        discard;
    }
}