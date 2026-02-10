#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

out vec3 chFragPos;
out vec3 chNormal;
out vec2 chUV;

uniform mat4 uM;
uniform mat4 uV;
uniform mat4 uP;

uniform vec3 uOffset;
uniform vec3 uVel;
uniform float uTime;

uniform vec3 uAmp;
uniform float uFreq;

void main()
{
    chUV = inUV;

    vec3 p = inPos;

    vec3 move = uOffset + uVel * uTime;

    float w = sin(uTime * uFreq);
    move += uAmp * w;

    vec4 world = uM * vec4(p, 1.0);
    world.xyz += move;

    chFragPos = world.xyz;

    chNormal = mat3(transpose(inverse(uM))) * inNormal;

    gl_Position = uP * uV * world;
}
