// water.frag
#version 330 core

in vec3 vWorldPos;
out vec4 FragColor;

// маска: где вообще есть вода
uniform sampler2D uWaterMask;
uniform float     uTerrainSize;

// базовые параметры воды
uniform vec3 uWaterColor;   // цвет воды в глубине
uniform float uAlpha;       // базовая прозрачность

// туман / подводный режим
uniform vec3  uCamPos;
uniform int   uUnderwater;
uniform vec3  uFogColor;
uniform float uFogDensity;

// новое:
uniform vec3 uSkyColor;     // цвет неба (для отражений)
uniform vec3 uLightDir;     // направление света (как в террейне)

void main()
{
    // --- 1. Сначала решаем: вообще есть тут вода или нет? ---

    float halfSize = uTerrainSize * 0.5;
    float xn = (vWorldPos.x + halfSize) / uTerrainSize;
    float zn = (vWorldPos.z + halfSize) / uTerrainSize;

    // По твоим последним фиксам маска уже корректная: 0 – суша, 1 – вода
    float mask = texture(uWaterMask, vec2(xn, zn)).r;
    if (mask < 0.5)
        discard;

    // --- 2. Освещение воды ---

    // Плоская базовая нормаль, вода как большая плоскость
    vec3 N = vec3(0.0, 1.0, 0.0);

    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uCamPos - vWorldPos);

    // diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 base = uWaterColor * (0.4 + 0.6 * diff);

    // простое зеркальное (specular)
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(R, V), 0.0), 64.0);   // 64 – “глянцевость”
    vec3 specColor = vec3(1.0) * spec * 0.6;      // белый блик

    // --- 3. Френель – отражение неба сильнее под острым углом ---

    float cosTheta = max(dot(N, V), 0.0);
    float fresnel = pow(1.0 - cosTheta, 3.0);     // 0..1
    vec3 reflection = uSkyColor;

    vec3 waterCol = mix(base, reflection, fresnel) + specColor;

    // --- 4. Туман / подводное "молоко" ---

    float dist = length(uCamPos - vWorldPos);
    float fogFactor = 1.0 - exp(-uFogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 col;
    float alpha = uAlpha;

    if (uUnderwater == 1)
    {
        // под водой сильнее туман
        col   = mix(waterCol, uFogColor, fogFactor * 0.7);
        alpha = 0.0;
    }
    else
    {
        col = mix(waterCol, uFogColor, fogFactor * 0.4);
    }

    FragColor = vec4(col, alpha);
}
