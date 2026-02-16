#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uSceneTex;
uniform int  uUnderwater;
uniform float uTime;

vec2 wobble(vec2 uv, float strength)
{
    float w1 = sin(uv.y * 15.0 + uTime * 2.0);
    float w2 = cos(uv.x * 12.0 + uTime * 1.7);
    return uv + strength * vec2(w1, w2) * 0.003;
}

void main()
{
    // == НАД ВОДОЙ ==
    if (uUnderwater == 0)
    {
        FragColor = texture(uSceneTex, vUV);
        return;
    }

    // == ПОД ВОДОЙ ==
    float strength = 1.0;
    vec2 uv0 = wobble(vUV, strength);

    vec4 col = texture(uSceneTex, uv0) * 0.4;
    col += texture(uSceneTex, wobble(vUV + vec2( 0.003, 0.0), strength)) * 0.15;
    col += texture(uSceneTex, wobble(vUV + vec2(-0.003, 0.0), strength)) * 0.15;
    col += texture(uSceneTex, wobble(vUV + vec2(0.0,  0.003), strength)) * 0.15;
    col += texture(uSceneTex, wobble(vUV + vec2(0.0, -0.003), strength)) * 0.15;

    vec3 tint = vec3(0.0, 0.3, 0.35);
    col.rgb = mix(col.rgb, tint, 0.15);

    FragColor = col;
}
