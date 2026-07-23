#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 3) in mat4 model;
layout(location = 7) in vec4 color;
layout(location = 8) in uvec3 texIndices1;
layout(location = 9) in uvec3 texIndices2;
layout(location = 10) in vec2 texTile;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec3 fragScale;
layout(location = 4) flat out uvec3 fragTexIndices1;
layout(location = 5) flat out uvec3 fragTexIndices2;
layout(location = 6) out vec2 fragTexTile;
layout(location = 7) out vec3 fragCameraPos;
layout(location = 8) out vec3 fragWorldPos;
layout(location = 9) out vec3 fragWorldNormal;

layout(push_constant) uniform PushConstantObject {
    mat4 viewProj;
    vec3 cameraPos;
} push;

void main() {
    vec4 worldPos = model * vec4(position, 1.0);

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 worldNormal = normalize(normalMatrix * normal);

    vec3 scale = vec3(length(model[0].xyz), length(model[1].xyz), length(model[2].xyz));

    fragPos = position * scale;
    fragNormal = normal;
    fragColor = color;
    fragScale = scale;
    fragTexIndices1 = texIndices1;
    fragTexIndices2 = texIndices2;
    fragTexTile = texTile;
    fragCameraPos = push.cameraPos;
    fragWorldPos = worldPos.xyz;
    fragWorldNormal = worldNormal;

    gl_Position = push.viewProj * worldPos;
}