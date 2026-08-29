#version 450

layout(location = 0) in vec2 position;

layout(location = 1) in vec3 origin;
layout(location = 2) in vec2 scale;

layout(location = 0) out vec2 fragUv;
layout(location = 1) flat out uint fragTexIndex;

layout(push_constant) uniform PushConstantObject {
    mat4 view;
    mat4 proj;
    uint texIndex;
} push;

void main() {
    vec2 localPos = position * scale;

    vec3 cameraRight = vec3(push.view[0][0], push.view[1][0], push.view[2][0]);
    vec3 cameraUp = vec3(push.view[0][1], push.view[1][1], push.view[2][1]);

    vec3 worldPos = origin + cameraRight * localPos.x - cameraUp * localPos.y;

    fragUv = position;
    fragTexIndex = push.texIndex;

    gl_Position = push.proj * push.view * vec4(worldPos, 1.0);
}