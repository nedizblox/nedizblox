#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 3) in mat4 model;
layout(location = 7) in vec4 outlineColor;

layout(push_constant) uniform PushConstantObject {
    mat4 viewProj;
    vec3 cameraPos;
} push;

layout(location = 0) out vec3 fragLocalPos;
layout(location = 1) out vec3 fragLocalNormal;
layout(location = 2) out vec4 fragColor;

void main() {
    fragLocalPos = position;
    fragLocalNormal = normal;
    fragColor = outlineColor;

    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = push.viewProj * worldPos;
}