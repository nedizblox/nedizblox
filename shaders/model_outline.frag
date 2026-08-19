#version 450

layout(location = 0) in vec3 fragLocalPos;
layout(location = 1) in vec3 fragLocalNormal;
layout(location = 2) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

const float THICKNESS = 0.05; 

void main() {
    vec3 n = abs(fragLocalNormal);
    vec2 faceUV;

    if (n.x > n.y && n.x > n.z) {
        faceUV = fragLocalPos.zy;
    } else if (n.y > n.x && n.y > n.z) {
        faceUV = fragLocalPos.xz;
    } else {
        faceUV = fragLocalPos.xy;
    }

    vec2 uv = faceUV + vec2(0.5);

    bool isEdge = (uv.x < THICKNESS || uv.x > (1.0 - THICKNESS) ||
                   uv.y < THICKNESS || uv.y > (1.0 - THICKNESS));

    if (!isEdge) {
        discard;
    }

    outColor = fragColor;
}