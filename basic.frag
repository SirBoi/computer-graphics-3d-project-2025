#version 330 core
in vec3 vWorldPos;
in vec3 vN;
in vec3 vBase;

out vec4 FragColor;

uniform vec3 uViewPos;

#define MAX_LIGHTS 8
uniform int uLightCount;
uniform vec3 uLightPos[MAX_LIGHTS];
uniform vec3 uLightColor[MAX_LIGHTS];
uniform float uLightIntensity[MAX_LIGHTS];

void main() {
    vec3 N = normalize(vN);
    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 sum = 0.12 * vBase;

    for(int i=0;i<uLightCount;i++){
        vec3 Ldir = uLightPos[i] - vWorldPos;
        float dist = length(Ldir);
        vec3 L = Ldir / max(dist, 1e-4);

        float att = 1.0 / (1.0 + 0.09*dist + 0.032*dist*dist);

        float diff = max(dot(N, L), 0.0);

        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);

        vec3 light = uLightColor[i] * uLightIntensity[i] * att;
        sum += (vBase * diff + vec3(1.0)*spec*0.25) * light;
    }

    FragColor = vec4(sum, 1.0);
}
