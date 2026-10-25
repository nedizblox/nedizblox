#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in mat4 model;

layout(push_constant) uniform PushConstantObject {
    mat4 lightSpace;
} push;

void main() {
    gl_Position = push.lightSpace * model * vec4(position, 1.0);
}