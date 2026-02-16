#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;
layout (location = 3) in float aMat;

out vec3 vNormal;
out vec2 vTex;
out float vMat;
out vec3 vWorldPos;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // если uModel всегда = identity, это то же самое, но так корректнее
    vNormal = mat3(uModel) * aNormal;

    vTex = aTex;
    vMat = aMat;

    gl_Position = uProjection * uView * worldPos;
}
