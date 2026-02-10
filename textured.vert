#version 330 core
layout(location=0) in vec3 inPos;
layout(location=1) in vec2 inUV;

out vec2 vUV;
out vec3 vWorldPos;
out vec3 vN;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

float hash(vec2 p){
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    float h = hash(inPos.xz);
    float offset = (h - 0.5) * 0.015;

    vec3 pos = inPos;
    pos.y += offset;

    vec4 world = uM * vec4(pos, 1.0);
    vWorldPos = world.xyz;

    vN = normalize(mat3(transpose(inverse(uM))) * vec3(0,1,0));
    vUV = inUV;

    gl_Position = uP * uV * world;
}
