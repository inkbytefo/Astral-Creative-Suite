#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

// Instance input
layout(location = 3) in vec2 inOffset;
layout(location = 4) in float inZoom;
layout(location = 5) in vec4 inCanvasColor;

// Output to fragment shader
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec4 canvasColor;

void main() {
    // Apply canvas transform (offset and zoom)
    vec2 transformedPos = (inPosition * inZoom) + inOffset;
    
    // Convert to clip space (-1 to 1)
    gl_Position = vec4(transformedPos, 0.0, 1.0);
    
    fragTexCoord = inTexCoord;
    fragColor = inColor;
    canvasColor = inCanvasColor;
}