#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragUv;
layout(location = 1) flat in uint fragCubIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform samplerCube cubemaps[];

void main() {
    outColor = texture(cubemaps[nonuniformEXT(fragCubIndex)], fragUv);
}