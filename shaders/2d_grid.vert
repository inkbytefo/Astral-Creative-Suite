#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

// Instance input for grid properties
layout(location = 2) in vec2 inOffset;
layout(location = 3) in float inZoom;
layout(location = 4) in float inGridSize;

// Output to fragment shader
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 worldPos;

void main() {
    // Apply canvas transform (offset and zoom)
    vec2 transformedPos = (inPosition * inZoom) + inOffset;
    
    // Convert to clip space (-1 to 1)
    gl_Position = vec4(transformedPos, 0.0, 1.0);
    
    fragColor = inColor;
    worldPos = inPosition * inGridSize;
}