#version 330 core

in vec2 vTex;
in vec3 vNormal;
out vec4 FragColor;

uniform sampler2D uTex;
uniform vec3 uLightDir;

void main()
{
	//vec2 uv = vec2(vTex.x, 1.0 - vTex.y);
    vec3 base = texture(uTex, vTex).rgb;
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    float diff = max(dot(N, L), 0.0);

    vec3 color = base * (0.3 + 0.7 * diff);
    FragColor = vec4(color, 1.0);
}
