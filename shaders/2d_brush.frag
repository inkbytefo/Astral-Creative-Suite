#version 450

// Input from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 brushCenter;
layout(location = 3) in float brushRadius;
layout(location = 4) in float brushHardness;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Calculate distance from brush center
    float distance = length(fragTexCoord - brushCenter);
    
    // Create brush falloff based on hardness
    float alpha = 1.0 - smoothstep(brushRadius * brushHardness, brushRadius, distance);
    
    // Apply color and alpha
    outColor = fragColor * alpha;
    
    // Discard transparent fragments
    if (outColor.a < 0.01) {
        discard;
    }
}