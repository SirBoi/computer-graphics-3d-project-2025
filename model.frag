#version 330 core
out vec4 FragColor;

in vec3 chFragPos;
in vec3 chNormal;
in vec2 chUV;

uniform vec3 uViewPos;

uniform sampler2D texture_diffuse1;

#define MAX_LIGHTS 8
uniform int uLightCount;
uniform vec3 uLightPos[MAX_LIGHTS];
uniform vec3 uLightColor[MAX_LIGHTS];
uniform float uLightIntensity[MAX_LIGHTS];

void main()
{
    vec3 N = normalize(chNormal);
    vec3 V = normalize(uViewPos - chFragPos);

    vec4 tex = texture(texture_diffuse1, chUV);

    if (tex.rgb == vec3(0.0)) tex = vec4(1.0);

    vec3 lightSum = vec3(0.0);

    float ambientStrength = 0.25;
    lightSum += ambientStrength * vec3(1.0);

    float specularStrength = 0.35;
    float shininess = 32.0;

    for (int i = 0; i < uLightCount && i < MAX_LIGHTS; i++)
    {
        vec3 Ldir = uLightPos[i] - chFragPos;
        float dist = length(Ldir);
        vec3 L = Ldir / max(dist, 1e-4);

        float attenuation = 1.0 / (1.0 + 0.35 * dist + 0.10 * dist * dist);

        vec3 Li = uLightColor[i] * uLightIntensity[i] * attenuation;

        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * Li;

        vec3 R = reflect(-L, N);
        float spec = pow(max(dot(V, R), 0.0), shininess);
        vec3 specular = specularStrength * spec * Li;

        lightSum += diffuse + specular;
    }

    vec3 lit = tex.rgb * lightSum;
    FragColor = vec4(lit, tex.a);
}
