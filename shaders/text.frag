#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 fragUv;
layout(location = 1) flat in uint fragTexIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

void main() {
    float width = 0.001;
    vec4 textColor = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 outlineColor = vec4(0.0, 0.0, 0.0, 1.0);
    
    float alpha = texture(textures[nonuniformEXT(fragTexIndex)], fragUv).r;

    float a2 = texture(textures[nonuniformEXT(fragTexIndex)], fragUv + vec2(width, 0.0)).r;
    float a3 = texture(textures[nonuniformEXT(fragTexIndex)], fragUv + vec2(-width, 0.0)).r;
    float a4 = texture(textures[nonuniformEXT(fragTexIndex)], fragUv + vec2(0.0, width)).r;
    float a5 = texture(textures[nonuniformEXT(fragTexIndex)], fragUv + vec2(0.0, -width)).r;
    
    float outlineAlpha = max(max(a2, a3), max(a4, a5));

    float finalAlpha = max(alpha, outlineAlpha);
    vec3 finalColor = mix(outlineColor.rgb, textColor.rgb, alpha);

    outColor = vec4(finalColor, finalAlpha);
}