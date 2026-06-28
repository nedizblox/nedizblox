#version 450

layout(location = 0) in vec2 position;

layout(location = 1) in vec2 origin;
layout(location = 2) in vec2 offset;
layout(location = 3) in vec2 size;
layout(location = 4) in vec2 uvTopLeft;
layout(location = 5) in vec2 uvBottomRight;

layout(location = 0) out vec2 fragUv;
layout(location = 1) flat out uint fragTexIndex;

layout(push_constant) uniform PushConstantObject {
    mat4 proj;
    uint texIndex;
} push;

void main() {
    fragUv = mix(uvTopLeft, uvBottomRight, position);
    fragTexIndex = push.texIndex;

    vec2 pos = origin + offset + (position * size);
    gl_Position = push.proj * vec4(pos, 0.0, 1.0);
}