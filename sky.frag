// sky.frag
#version 330 core

out vec4 FragColor;

uniform vec3 uTopColor;      // цвет наверху
uniform vec3 uBottomColor;   // цвет у горизонта / внизу
uniform float uScreenHeight; // высота экрана в пикселях

void main()
{
    // 0 внизу экрана, 1 наверху
    float t = gl_FragCoord.y / uScreenHeight;
    t = clamp(t, 0.0, 1.0);

    vec3 col = mix(uBottomColor, uTopColor, t);
    FragColor = vec4(col, 1.0);
}
