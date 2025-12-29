#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

// Push constants for screen dimensions
layout(push_constant) uniform PushConstants {
    vec2 screenSize;
} push;

void main() {
    // Convert from pixel coordinates to normalized device coordinates
    vec2 ndc;
    ndc.x = (inPosition.x / push.screenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (inPosition.y / push.screenSize.y) * 2.0; // Flip Y axis
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}

