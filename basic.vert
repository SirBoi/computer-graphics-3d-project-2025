#version 330 core
layout(location=0) in vec3 inPos;
layout(location=1) in vec4 inCol;
layout(location=2) in vec3 inN;

out vec3 vWorldPos;
out vec3 vN;
out vec3 vBase;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

void main() {
    vec4 world = uM * vec4(inPos, 1.0);
    vWorldPos = world.xyz;

    mat3 normalMat = mat3(transpose(inverse(uM)));
    vN = normalize(normalMat * inN);

    vBase = inCol.rgb;

    gl_Position = uP * uV * world;
}
