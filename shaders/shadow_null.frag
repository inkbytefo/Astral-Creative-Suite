#version 460

// Empty fragment shader for depth-only shadow pass
// This is only needed if the pipeline creation requires a fragment stage

void main() {
    // No output needed for depth-only rendering
    // Depth is written automatically by the hardware
}
