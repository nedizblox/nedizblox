#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragUv;
layout(location = 1) flat in uint fragTexIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

const vec4 TEXT_COLOR = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 OUTLINE_COLOR = vec4(0.0, 0.0, 0.0, 0.8);
const float OUTLINE_WIDTH = 0.2;
const float EDGE = 0.5;

void main() {
    float dist = texture(textures[nonuniformEXT(fragTexIndex)], fragUv).r;
    float smoothing = fwidth(dist);

    float textFactor = smoothstep(EDGE - smoothing, EDGE + smoothing, dist);
    float outlineFactor = smoothstep(EDGE - OUTLINE_WIDTH - smoothing, EDGE - OUTLINE_WIDTH + smoothing, dist);

    float textAlpha = textFactor * TEXT_COLOR.a;
    float outlineAlpha = outlineFactor * OUTLINE_COLOR.a;

    float finalAlpha = textAlpha + outlineAlpha * (1.0 - textAlpha);

    vec3 finalColor = mix(OUTLINE_COLOR.rgb, TEXT_COLOR.rgb, textFactor);

    outColor = vec4(finalColor, clamp(finalAlpha, 0.0, 1.0));
}