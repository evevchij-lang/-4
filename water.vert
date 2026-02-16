// water.vert
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

uniform mat4 uProjection;
uniform mat4 uView;
uniform float uTime;

out vec3 vWorldPos;

void main()
{
    float wave =
        sin(aPos.x * 0.05 + uTime * 1.5) * 0.05 +
        cos(aPos.z * 0.07 + uTime * 1.2) * 0.05;

    vec3 pos = aPos + vec3(0.0, wave, 0.0);

    vWorldPos = pos;
    gl_Position = uProjection * uView * vec4(pos, 1.0);
}
