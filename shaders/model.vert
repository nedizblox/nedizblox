#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 3) in mat4 instanceModel;
layout(location = 7) in vec4 instanceColor;
layout(location = 8) in uint texIndex;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec3 fragScale;
layout(location = 4) flat out uint fragTexIndex;

layout(push_constant) uniform PushConstantObject {
    mat4 viewProj;
} push;

void main() {
    vec4 worldPos = instanceModel * vec4(position, 1.0);
    vec3 worldNormal = normalize(mat3(instanceModel) * normal);

    vec3 scale = vec3(
        length(instanceModel[0].xyz),
        length(instanceModel[1].xyz),
        length(instanceModel[2].xyz)
    );

    vec3 lightDirection = normalize(vec3(1.0, 1.5, 2.0));
    float ambientStren = 1.5;

    float lightIntensity = ambientStren + max(dot(worldNormal, lightDirection), 0);

    fragPos = position * scale;
    fragNormal = normal;
    fragColor = vec4(instanceColor.rgb * lightIntensity, instanceColor.a);
    fragScale = scale;
    fragTexIndex = texIndex;

    gl_Position = push.viewProj * worldPos;
}