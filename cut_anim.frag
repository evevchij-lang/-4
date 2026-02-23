#version 330 core

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTex;

out vec4 FragColor;

uniform sampler2D uTex;

uniform vec3 uCamPos;
uniform vec3 uLightDir;

// туман (как в tree_mesh.frag)
uniform int  uUnderwater;
uniform vec3 uFogColor;
uniform float uFogDensity;

void main()
{
    // ВАЖНО: тот самый фикс, который ты вчера нашёл
    vec2 uv = vec2(vTex.x, 1.0 - vTex.y);

    vec4 tex = texture(uTex, uv);

    // как у деревьев: вырезаем прозрачное
    if (tex.a < 0.2)
        discard;

    float dist = length(uCamPos - vWorldPos);

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);

    float diff = max(dot(N, L), 0.0);
    float ambient = 0.25;
    float lighting = ambient + diff * 0.75;

    vec3 color = tex.rgb * lighting;

    // === ТУМАН ===
    float fogFactor = 1.0 - exp(-uFogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    color = mix(color, uFogColor, fogFactor);

    if (uUnderwater == 1)
        color *= 0.85;

    FragColor = vec4(color, tex.a);
}