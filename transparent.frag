#version 330 core

in vec3 vWorldPos;
in vec3 vN;
in vec4 vCol;

out vec4 FragColor;

uniform vec3 uViewPos;

uniform float uAlphaMul;
uniform float uFresnelPow;
uniform float uRimBoost;

#define MAX_LIGHTS 8
uniform int uLightCount;
uniform vec3 uLightPos[MAX_LIGHTS];
uniform vec3 uLightColor[MAX_LIGHTS];
uniform float uLightIntensity[MAX_LIGHTS];

void main()
{
    vec3 N = normalize(vN);
    vec3 V = normalize(uViewPos - vWorldPos);

    float fresnel = pow(1.0 - max(dot(N, V), 0.0), uFresnelPow);

    vec3 baseColor = vCol.rgb;

    vec3 color = 0.05 * baseColor;

    for (int i = 0; i < uLightCount; i++)
    {
        vec3 Ldir = uLightPos[i] - vWorldPos;
        float dist = length(Ldir);
        vec3 L = Ldir / max(dist, 1e-4);

        float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        vec3 light = uLightColor[i] * uLightIntensity[i] * att;

        float diff = max(dot(N, L), 0.0);

        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 96.0);

        color += (baseColor * diff * 0.35 + vec3(1.0) * spec) * light;
    }

    float alpha = clamp(
        vCol.a * uAlphaMul + fresnel * uRimBoost,
        0.0, 1.0
    );

    color += fresnel * vec3(0.25);

    FragColor = vec4(color, alpha);
}
