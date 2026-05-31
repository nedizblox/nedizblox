#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 3) in mat4 model;
layout(location = 7) in vec4 color;
layout(location = 8) in uint texIndex;
layout(location = 9) in vec2 texTile;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec3 fragScale;
layout(location = 4) flat out uint fragTexIndex;
layout(location = 5) out vec2 fragTexTile;

layout(push_constant) uniform PushConstantObject {
    mat4 viewProj;
} push;

void main() {
    vec4 worldPos = model * vec4(position, 1.0);
    vec3 worldNormal = normalize(mat3(model) * normal);

    vec3 scale = vec3(
        length(model[0].xyz),
        length(model[1].xyz),
        length(model[2].xyz)
    );

    vec3 lightDirection = normalize(vec3(1.0, 1.5, 2.0));
    float ambientStren = 1.5;

    float lightIntensity = ambientStren + max(dot(worldNormal, lightDirection), 0);

    fragPos = position * scale;
    fragNormal = normal;
    fragColor = vec4(color.rgb * lightIntensity, color.a);
    fragScale = scale;
    fragTexIndex = texIndex;
    fragTexTile = texTile;

    gl_Position = push.viewProj * worldPos;
}