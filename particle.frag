#version 330 core

in vec3 vWorldPos;
in vec3 vN;

out vec4 FragColor;

uniform vec3 uViewPos;
uniform vec3 uColor;
uniform float uAlpha;

void main()
{
    vec3 N = normalize(vN);
    vec3 V = normalize(uViewPos - vWorldPos);

    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.5);

    vec3 col = uColor + rim * vec3(0.6);
    float a = clamp(uAlpha + rim * 0.25, 0.0, 1.0);

    FragColor = vec4(col, a);
}
