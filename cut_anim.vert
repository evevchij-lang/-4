#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vTex;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // нормали с учётом масштаба/поворота
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    vTex = aUV;
    gl_Position = uProjection * uView * worldPos;
}