#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex;
layout(location = 3) in ivec4 aBoneIds;
layout(location = 4) in vec4 aWeights;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;
uniform mat4 uNode;              // <-- ВАЖНО: матрица узла (rigid animation)

const int MAX_BONES = 128;
uniform mat4 uBones[MAX_BONES];

out vec3 vNormal;
out vec2 vTex;

void main()
{
    float wsum = aWeights.x + aWeights.y + aWeights.z + aWeights.w;

    mat4 skin = mat4(1.0);
    if (wsum > 0.0001)
    {
        skin =
            aWeights.x * uBones[aBoneIds.x] +
            aWeights.y * uBones[aBoneIds.y] +
            aWeights.z * uBones[aBoneIds.z] +
            aWeights.w * uBones[aBoneIds.w];
    }

    vec4 localPos = skin * vec4(aPos, 1.0);

    // uNode применяется ВСЕГДА, но в C++ мы для skinned мешей дадим identity
    vec4 worldPos = uModel * uNode * localPos;

    // нормаль по uModel*uNode (skin на нормаль тут не трогаем — ок для теста)
    mat3 N = mat3(transpose(inverse(uModel * uNode)));
    vNormal = normalize(N * aNormal);
    vTex = aTex;

    gl_Position = uProjection * uView * worldPos;
}
