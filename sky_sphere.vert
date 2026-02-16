#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 uProjection;
uniform mat4 uView;

out float vHeight;   // 0..1 — высота точки на сфере
out vec3  vDir;      // направление от центра сферы

void main()
{
    // Направление от центра сферы
    vec3 dir = normalize(aPos);
    vDir = dir;

    // высота для градиента: -1..1 → 0..1
    vHeight = dir.y * 0.5 + 0.5;

    // Сфера всегда вокруг камеры: убираем смещение из view
    mat4 viewNoTrans = uView;
    viewNoTrans[3] = vec4(0.0, 0.0, 0.0, viewNoTrans[3].w);

    gl_Position = uProjection * viewNoTrans * vec4(aPos, 1.0);
}
