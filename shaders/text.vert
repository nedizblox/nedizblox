#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 fragUv;
layout(location = 1) flat out uint fragTexIndex;

layout(push_constant) uniform PushConstantObject {
    mat4 proj;
    vec2 scale;
    uint texIndex;
} push;

void main() {
    fragUv = uv;
    fragTexIndex = push.texIndex;

    gl_Position = push.proj * vec4(position * push.scale, 0.0, 1.0);
}