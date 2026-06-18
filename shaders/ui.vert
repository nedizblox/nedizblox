#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 fragUv;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out uint fragTexIndex;

layout(push_constant) uniform PushConstantObject {
    mat4 proj;
    uint texIndex;
} push;

void main() {
    fragUv = uv;
    fragColor = color;
    fragTexIndex = push.texIndex;

    gl_Position = push.proj * vec4(position, 0.0, 1.0);
}