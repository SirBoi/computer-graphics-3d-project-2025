#version 330 core
in vec2 vUV;
in vec3 vWorldPos;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uViewPos;

#define MAX_LIGHTS 8
uniform int uLightCount;
uniform vec3 uLightPos[MAX_LIGHTS];
uniform vec3 uLightColor[MAX_LIGHTS];
uniform float uLightIntensity[MAX_LIGHTS];

void main() {
    vec3 base = texture(uTexture, vUV).rgb;

    vec3 dpdx = dFdx(vWorldPos);
    vec3 dpdy = dFdy(vWorldPos);
    vec3 N = normalize(cross(dpdx, dpdy));

    vec3 V = normalize(uViewPos - vWorldPos);
    N = faceforward(N, -V, N);

    vec3 ambient = 0.12 * base;
    vec3 sum = ambient;

    for(int i=0;i<uLightCount;i++){
        vec3 Ldir = uLightPos[i] - vWorldPos;
        float dist = length(Ldir);
        vec3 L = Ldir / max(dist, 1e-4);

        float att = 1.0 / (1.0 + 0.09*dist + 0.032*dist*dist);

        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 16.0);

        vec3 light = uLightColor[i] * uLightIntensity[i] * att;
        sum += (base * diff + vec3(1.0)*spec*0.15) * light;
    }

    FragColor = vec4(sum, 1.0);
}
