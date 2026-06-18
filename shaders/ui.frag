#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragUv;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint fragTexIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

void main() {
    vec4 texColor = texture(textures[nonuniformEXT(fragTexIndex)], fragUv);
    outColor = fragColor * texColor;
}