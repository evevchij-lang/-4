#version 330 core

in vec3 vNormal;
in vec2 vTex;
in float vMat;
in vec3 vWorldPos;      // ДОБАВИЛИ

out vec4 FragColor;

uniform sampler2D uTexGrass;
uniform sampler2D uTexSand;
uniform vec3 uLightDir;

// для тумана / подводного режима
uniform vec3 uCamPos;
uniform int  uUnderwater;   // 0 = над водой, 1 = под водой
uniform vec3 uFogColor;
uniform float uFogDensity;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    float diff = max(dot(N, L), 0.0);

    vec3 grass = texture(uTexGrass, vTex).rgb;
    vec3 sand  = texture(uTexSand, vTex * 8.0).rgb;

    float m = clamp(vMat, 0.0, 1.0);
    vec3 base = mix(grass, sand, m);

    vec3 color = base * (0.3 + 0.7 * diff);

    // === ТУМАН / ПОДВОДНЫЙ ЭФФЕКТ ===
    float dist = length(vWorldPos - uCamPos);

    // density уже пришёл с нужным значением (над/под водой)
    float fogFactor = 1.0 - exp(-uFogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    color = mix(color, uFogColor, fogFactor);

    // чуть приглушим свет под водой
    if (uUnderwater == 1)
        color *= 0.85;

    FragColor = vec4(color, 1.0);
}
