#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTex;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat4 uNode;   // <<< ДОБАВИЛИ

void main()
{
    mat4 M = uModel * uNode;                     // <<< ВАЖНО
    vec4 worldPos = M * vec4(aPos, 1.0);
    vNormal = mat3(transpose(inverse(M))) * aNormal;
    vTex = aTex;
    gl_Position = uProjection * uView * worldPos;
}
