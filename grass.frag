#version 330 core

in vec2 vTex;
in vec3 vWorldPos;      // ДОБАВИЛИ
out vec4 FragColor;

uniform sampler2D uGrassTex;

// туман
uniform vec3 uCamPos;
uniform int  uUnderwater;
uniform vec3 uFogColor;
uniform float uFogDensity;

void main()
{
    vec4 tex = texture(uGrassTex, vTex);

    if (tex.a < 0.3)
        discard;

    float shade = mix(0.6, 1.0, vTex.y);
    vec3 color = tex.rgb * shade;

    // === ТУМАН ===
    float dist = length(vWorldPos - uCamPos);
    float fogFactor = 1.0 - exp(-uFogDensity * dist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    color = mix(color, uFogColor, fogFactor);

    if (uUnderwater == 1)
        color *= 0.85;

    FragColor = vec4(color, tex.a);
}
