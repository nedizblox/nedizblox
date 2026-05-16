#version 450

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 fragUv;
layout(location = 1) flat out uint fragCubIndex;

layout(push_constant) uniform PushConstantObject {
    mat4 viewProj;
    uint cubIndex;
} push;

void main() {
    fragUv = position;
    fragCubIndex = push.cubIndex;

    vec4 pos = push.viewProj * vec4(position, 1.0);
    gl_Position = pos.xyww;
}