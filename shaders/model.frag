#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragScale;
layout(location = 4) flat in uint fragTexIndex;
layout(location = 5) in vec2 fragTexTile;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

void main() {
    uint layer;
    vec2 uv;
    vec3 n = abs(fragNormal);

    vec3 shiftedPos = fragPos + (fragScale * 0.5);

    if (fragTexTile.y < 0.5) {
        vec2 faceSize;
        vec2 localUV;

        if (n.x > n.y && n.x > n.z) {
            layer = (fragNormal.x > 0.0) ? 0 : 1;
            faceSize = fragScale.zy;
            localUV = shiftedPos.zy;
        } else if (n.y > n.x && n.y > n.z) {
            layer = (fragNormal.y > 0.0) ? 2 : 3;
            faceSize = fragScale.xz;
            localUV = shiftedPos.xz;
        } else {
            layer = (fragNormal.z > 0.0) ? 4 : 5;
            faceSize = fragScale.xy;
            localUV = shiftedPos.xy;
        }

        float maxSide = max(faceSize.x, faceSize.y);
        uv = localUV / maxSide;

        vec2 uvOffset = (faceSize / maxSide - vec2(1.0)) * 0.5;
        uv -= uvOffset;
    } else {
        if (n.x > n.y && n.x > n.z) {
            layer = (fragNormal.x > 0.0) ? 0 : 1;
            uv = shiftedPos.zy;
        } else if (n.y > n.x && n.y > n.z) {
            layer = (fragNormal.y > 0.0) ? 2 : 3;
            uv = shiftedPos.xz;
        } else {
            layer = (fragNormal.z > 0.0) ? 4 : 5;
            uv = shiftedPos.xy;
        }

        uv = (uv * fragTexTile.x) / 2.0;
    }

    uint texIndex = fragTexIndex + layer;
    vec4 texColor = texture(textures[nonuniformEXT(texIndex)], uv);

    vec4 baseTexColor = texture(textures[nonuniformEXT(fragTexIndex)], uv);

    vec4 color = mix(baseTexColor, texColor, texColor.a);

    outColor = color * fragColor;
}