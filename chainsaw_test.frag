#version 330 core

in vec3 vNormal;
in vec2 vTex;

out vec4 FragColor;

uniform vec3 uLightDir;

uniform sampler2D uTex;
uniform int uHasTex;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    float diff = max(dot(N, L), 0.0);

    vec3 base = vec3(0.7);
    if (uHasTex == 1)
        base = texture(uTex, vTex).rgb;

    vec3 color = base * (0.25 + 0.75 * diff);
    FragColor = vec4(color, 1.0);
}
