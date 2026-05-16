#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragScale;
layout(location = 4) flat in uint fragTexIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

void main() {
    uint layer;
    vec2 uv;
    vec3 n = abs(fragNormal);
    vec3 sPos = fragPos + (fragScale * 0.5);

    if (n.x > n.y && n.x > n.z) {
        layer = (fragNormal.x > 0.0) ? 0 : 1;
        uv = sPos.zy;
    } else if (n.y > n.x && n.y > n.z) {
        layer = (fragNormal.y > 0.0) ? 2 : 3;
        uv = sPos.xz;
    } else {
        layer = (fragNormal.z > 0.0) ? 4 : 5;
        uv = sPos.xy;
    }

    uv /= 2.0;

    uint texIndex = fragTexIndex + layer;
    outColor = texture(textures[nonuniformEXT(texIndex)], uv) * fragColor;
}