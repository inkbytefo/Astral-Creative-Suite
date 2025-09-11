#version 450

// Input from vertex shader
layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 worldPos;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Create grid pattern
    vec2 grid = abs(fract(worldPos - 0.5) - 0.5) / fwidth(worldPos);
    float line = min(grid.x, grid.y);
    float alpha = 1.0 - min(line, 1.0);
    
    // Apply color and alpha
    outColor = fragColor * alpha;
    
    // Discard transparent fragments
    if (outColor.a < 0.01) {
        discard;
    }
}