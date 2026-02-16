#version 330 core

in float vHeight;
in vec3  vDir;

out vec4 FragColor;

// --- Градиент неба ---
uniform vec3 uBottomColor;   // у горизонта/внизу
uniform vec3 uTopColor;      // зенит
uniform vec3 uHorizonColor;  // середина

// --- Солнце ---
uniform vec3  uSunDir;       // нормализованное направление на солнце
uniform vec3  uSunColor;
uniform float uSunSize;      // угол ядра солнца (радианы)
uniform float uSunGlow;      // угол мягкого ореола (радианы)

// --- Подводный режим / туман ---
uniform int  uUnderwater;    // 1 – под водой, 0 – над водой
uniform vec3 uFogColor;      // цвет подводного тумана (fogColorUnder)

void main()
{
    // ---- 1. Базовый градиент неба ----
    float h = clamp(vHeight, 0.0, 1.0);
    h = smoothstep(0.0, 1.0, h);  // немного сгладим

    vec3 col;
    if (h < 0.5)
    {
        float t = h / 0.5;
        col = mix(uBottomColor, uHorizonColor, t);
    }
    else
    {
        float t = (h - 0.5) / 0.5;
        col = mix(uHorizonColor, uTopColor, t);
    }

    // ---- 2. Солнце на сфере ----
    vec3 d = normalize(vDir);
    vec3 s = normalize(uSunDir);

    float sunDot = clamp(dot(d, s), -1.0, 1.0);
    float ang = acos(sunDot);   // угол между направлением пикселя и солнцем

    // ядро солнца — яркое пятно
    float core = smoothstep(uSunSize, uSunSize * 0.5, ang); // ближе к 0 → 1
    // мягкий ореол вокруг
    float halo = smoothstep(uSunGlow, 0.0, ang);

    // смешиваем ядро с базовым небом
    col = mix(col, uSunColor, core);
    // добавляем мягкий ореол
    col += uSunColor * 0.3 * halo;

    // ---- 3. Под водой чуть тоним в цвет тумана ----
    if (uUnderwater == 1)
    {
        col = mix(col, uFogColor, 0.6);
    }

    FragColor = vec4(col, 1.0);
}
