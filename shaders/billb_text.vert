#version 450

layout(location = 0) in vec2 position;

layout(location = 1) in vec3 worldOrigin;
layout(location = 2) in vec2 offset;
layout(location = 3) in vec2 size;
layout(location = 4) in vec2 uvTopLeft;
layout(location = 5) in vec2 uvBottomRight;

layout(location = 0) out vec2 fragUv;
layout(location = 1) flat out uint fragTexIndex;

layout(push_constant) uniform PushConstantObject {
    mat4 view;
    mat4 proj;
    uint texIndex;
} push;

void main() {
    float worldScale = 0.005;
    vec2 localSignPos = (offset + position * size) * worldScale;

    vec3 cameraRight = vec3(push.view[0][0], push.view[1][0], push.view[2][0]);
    vec3 cameraUp = vec3(push.view[0][1], push.view[1][1], push.view[2][1]);

    vec3 worldPos = worldOrigin + cameraRight * localSignPos.x + cameraUp * localSignPos.y;

    fragUv = mix(uvTopLeft, uvBottomRight, position);
    fragTexIndex = push.texIndex;

    gl_Position = push.proj * push.view * vec4(worldPos, 1.0);
}