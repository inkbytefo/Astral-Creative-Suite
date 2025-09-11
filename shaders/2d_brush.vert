#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

// Instance input for brush properties
layout(location = 3) in vec2 inBrushPosition;
layout(location = 4) in float inBrushSize;
layout(location = 5) in float inBrushHardness;

// Output to fragment shader
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 brushCenter;
layout(location = 3) out float brushRadius;
layout(location = 4) out float brushHardness;

void main() {
    // Apply brush position and size
    vec2 transformedPos = inPosition * inBrushSize + inBrushPosition;
    
    // Convert to clip space (-1 to 1)
    gl_Position = vec4(transformedPos, 0.0, 1.0);
    
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    brushCenter = inBrushPosition;
    brushRadius = inBrushSize * 0.5;
    brushHardness = inBrushHardness;
}