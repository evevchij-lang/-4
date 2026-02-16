#version 330 core

layout (location = 0) in vec2 aQuadPos;   // -0.5..0.5, 0..1
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec4 aInstance;  // xyz (центр), w = scale

uniform mat4 uProjection;
uniform mat4 uView;
uniform float uTime;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;

out vec2 vTex;
out vec3 vWorldPos;

void main()
{
    vec3 center = aInstance.xyz;
    float scale = aInstance.w;

    float width  = 0.7 * scale;
    float height = 1.4 * scale;

    float x = aQuadPos.x;  // -0.5..0.5
    float y = aQuadPos.y;  // 0..1

    // ветер: сильнее к верху, немного рандома по позиции
    float phase = uTime * 2.3
                + center.x * 0.17
                + center.z * 0.23;
    float sway = sin(phase) * (0.15 + 0.35 * y);

    // смещение вершины в мировом пространстве:
    vec3 offset =
          uCameraRight * (x * width + sway)   // горизонт + покачивание
        + uCameraUp    * (y * height);        // вверх

    vec3 worldPos = center + offset;
    vWorldPos = worldPos;

    vTex = aTex;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
