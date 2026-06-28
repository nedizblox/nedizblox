#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragScale;
layout(location = 4) flat in uint fragTexIndex;
layout(location = 5) in vec2 fragTexTile;
layout(location = 6) in vec3 fragCameraPos;
layout(location = 7) in vec3 fragWorldPos;
layout(location = 8) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

const vec3 LIGHT_DIR = normalize(vec3(1.0, 1.5, 2.0));
const vec3 LIGHT_COLOR = vec3(1.0, 0.97, 0.9);
const float AMBIENT_FACTOR = 1.0;
const float DIFFUSE_FACTOR = 0.75;
const float SPECULAR_FACTOR = 0.2;
const float SHININESS = 48.0;

float specular(vec3 N, vec3 L, vec3 V) {
    vec3 H = normalize(L + V);
    return pow(max(dot(N, H), 0.0), SHININESS);
}

float rimLight(vec3 N, vec3 V) {
    float rim = 1.0 - max(dot(N, V), 0.0);
    return pow(rim, 3.0);
}

struct FaceUV {
    uint layer;
    vec2 uv;
};

FaceUV resolveUV(vec3 n, vec3 shiftedPos, vec3 scale, vec2 texTile) {
    FaceUV result;

    vec3 na = abs(n);

    if (texTile.y < 0.5) {
        vec2 faceSize;
        vec2 localUV;

        if (na.x > na.y && na.x > na.z) {
            result.layer = (n.x > 0.0) ? 0 : 1;
            faceSize = scale.zy;
            localUV = shiftedPos.zy;
        } else if (na.y > na.x && na.y > na.z) {
            result.layer = (n.y > 0.0) ? 2 : 3;
            faceSize = scale.xz;
            localUV = shiftedPos.xz;
        } else {
            result.layer = (n.z > 0.0) ? 4 : 5;
            faceSize = scale.xy;
            localUV = shiftedPos.xy;
        }

        float maxSide = max(faceSize.x, faceSize.y);
        result.uv = localUV / maxSide;
        vec2 uvOffset = (faceSize / maxSide - vec2(1.0)) * 0.5;
        result.uv -= uvOffset;
    } else {
        vec2 localUV;

        if (na.x > na.y && na.x > na.z) {
            result.layer = (n.x > 0.0) ? 0 : 1;
            localUV = shiftedPos.zy;
        } else if (na.y > na.x && na.y > na.z) {
            result.layer = (n.y > 0.0) ? 2 : 3;
            localUV = shiftedPos.xz;
        } else {
            result.layer = (n.z > 0.0) ? 4 : 5;
            localUV = shiftedPos.xy;
        }

        result.uv = (localUV * texTile.x) / 2.0;
    }

    return result;
}

void main() {
    vec3 N = normalize(fragWorldNormal);
    vec3 V = normalize(fragCameraPos - fragWorldPos);
    vec3 L = LIGHT_DIR;

    vec3 shiftedPos = fragPos + (fragScale * 0.5);
    FaceUV fuv = resolveUV(fragNormal, shiftedPos, fragScale, fragTexTile);

    uint texIndex = fragTexIndex + fuv.layer;
    vec4 texColor = texture(textures[nonuniformEXT(texIndex)], fuv.uv);
    vec4 baseTexColor = texture(textures[nonuniformEXT(fragTexIndex)], fuv.uv);
    vec4 sampledColor = mix(baseTexColor, texColor, texColor.a);

    vec3 albedo = sampledColor.rgb * fragColor.rgb;

    vec3 ambient = AMBIENT_FACTOR * albedo;

    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = DIFFUSE_FACTOR * NdotL * albedo * LIGHT_COLOR;

    float spec = specular(N, L, V) * NdotL;
    float lum = dot(albedo, vec3(0.299, 0.587, 0.114));
    vec3 specColor = SPECULAR_FACTOR * spec * LIGHT_COLOR * mix(vec3(1.0), albedo, lum * 0.5);

    vec3 litColor = ambient + diffuse + specColor;

    outColor = vec4(litColor, sampledColor.a * fragColor.a);
}