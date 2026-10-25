#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in vec3 fragScale;
layout(location = 4) flat in uvec3 fragTexIndices1;
layout(location = 5) flat in uvec3 fragTexIndices2;
layout(location = 6) flat in uvec3 fragTexTilesPacked;
layout(location = 7) flat in float fragTexTileFactor;
layout(location = 8) in vec3 fragCameraPos;
layout(location = 9) in vec3 fragWorldPos;
layout(location = 10) in vec3 fragWorldNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout(set = 1, binding = 0) uniform sampler2DShadow shadowMap;
layout(set = 1, binding = 1) uniform ShadowUbo {
    mat4 lightSpace;
    vec4 params;
} shadow;

const vec3 LIGHT_DIR = normalize(vec3(0.30, 0.90, 0.40));
const vec3 LIGHT_COLOR = vec3(1.0);
const float BRIGHTNESS = 1.5;
const vec3 SKY_AMBIENT = vec3(0.80);
const vec3 GROUND_AMBIENT = vec3(0.58);
const float DIFFUSE_FACTOR = 0.45;
const float SHADOW_AMBIENT_INFLUENCE = 0.5;

struct FaceUV {
    uint layer;
    vec2 uv;
    bool isStretched;
};

float getFaceTile(uvec3 packedTiles, uint layer) {
    vec2 t01 = unpackHalf2x16(packedTiles.x);
    vec2 t23 = unpackHalf2x16(packedTiles.y);
    vec2 t45 = unpackHalf2x16(packedTiles.z);

    if (layer == 0)
        return t01.x;
    if (layer == 1)
        return t01.y;
    if (layer == 2)
        return t23.x;
    if (layer == 3)
        return t23.y;
    if (layer == 4)
        return t45.x;
    return t45.y;
}

FaceUV resolveUV(vec3 n, vec3 shiftedPos, vec3 scale, uvec3 packedTiles) {
    FaceUV result;
    vec3 na = abs(n);

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

    float tileFactor = getFaceTile(packedTiles, result.layer);

    if (tileFactor <= 0.0) {
        result.isStretched = true;

        float maxSide = max(faceSize.x, faceSize.y);
        result.uv = localUV / maxSide;
        vec2 uvOffset = (faceSize / maxSide - vec2(1.0)) * 0.5;
        result.uv -= uvOffset;
    } else {
        result.isStretched = false;

        result.uv = (localUV * tileFactor) / fragTexTileFactor;
    }

    return result;
}

float calcShadow(vec3 worldPos, vec3 N) {
    float strength = shadow.params.w;
    if (strength <= 0.0)
        return 1.0;

    float NdotL = dot(N, LIGHT_DIR);
    if (NdotL <= 0.0)
        return 1.0;

    vec3 offsetPos = worldPos + N * (shadow.params.y * shadow.params.z * (1.0 - NdotL * 0.5));

    vec4 sc = shadow.lightSpace * vec4(offsetPos, 1.0);
    vec3 p = sc.xyz / sc.w;
    vec2 uv = p.xy * 0.5 + 0.5;

    if (p.z < 0.0 || p.z > 1.0 || any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))))
        return 1.0;

    float bias = max(shadow.params.x * (1.0 - NdotL), shadow.params.x * 0.1);
    float ref = p.z - bias;

    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float sum = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            sum += texture(shadowMap, vec3(uv + vec2(x, y) * texel, ref));
        }
    }
    float visibility = sum / 9.0;

    return mix(1.0 - strength, 1.0, visibility);
}

void main() {
    vec3 N = normalize(fragWorldNormal);
    vec3 L = LIGHT_DIR;

    vec3 shiftedPos = fragPos + (fragScale * 0.5);
    FaceUV fuv = resolveUV(fragNormal, shiftedPos, fragScale, fragTexTilesPacked);

    uint texIndices[6] = uint[](
        fragTexIndices1.x,
        fragTexIndices1.y,
        fragTexIndices1.z,
        fragTexIndices2.x,
        fragTexIndices2.y,
        fragTexIndices2.z
    );

    uint texIndex = texIndices[fuv.layer];
    uint baseTexIndex = texIndices[0];

    vec3 albedo;
    float finalAlpha = fragColor.a;

    if (fuv.isStretched) {
        vec2 backgroundUV = (shiftedPos.xy) * 0.5;
        vec4 baseTexColor = texture(textures[nonuniformEXT(baseTexIndex)], backgroundUV);
        vec3 baseColor = baseTexColor.rgb * fragColor.rgb;

        vec4 decalColor = texture(textures[nonuniformEXT(texIndex)], fuv.uv);

        float inBounds = step(0.0, fuv.uv.x) * step(fuv.uv.x, 1.0) *
                         step(0.0, fuv.uv.y) * step(fuv.uv.y, 1.0);
        float decalAlpha = decalColor.a * inBounds;

        albedo = mix(baseColor, decalColor.rgb, decalAlpha);
        finalAlpha = mix(baseTexColor.a * fragColor.a, decalColor.a, decalAlpha);
    } else {
        vec4 sampled = texture(textures[nonuniformEXT(texIndex)], fuv.uv);
        albedo = sampled.rgb * fragColor.rgb;
        finalAlpha = sampled.a * fragColor.a;
    }

    float up = N.y * 0.5 + 0.5;
    vec3 ambient = mix(GROUND_AMBIENT, SKY_AMBIENT, up);

    float shadowTerm = calcShadow(fragWorldPos, N);
    ambient *= mix(1.0, shadowTerm, SHADOW_AMBIENT_INFLUENCE);

    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = DIFFUSE_FACTOR * NdotL * LIGHT_COLOR * shadowTerm;

    vec3 litColor = albedo * (ambient + diffuse) * BRIGHTNESS;

    outColor = vec4(litColor, finalAlpha);
}