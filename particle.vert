#version 330 core

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inUV;

out vec3 vWorldPos;
out vec3 vN;

uniform mat4 uV;
uniform mat4 uP;

uniform vec3  uCenter;
uniform float uRadius;

uniform float uTime;
uniform vec3  uVel;
uniform vec3  uAmp;
uniform float uFreq;

void main()
{
    vec3 drift = uVel * uTime;

    float w = 6.2831853 * uFreq;
    vec3 wobble = vec3(
        sin(w * uTime),
        sin(w * uTime + 2.094),
        sin(w * uTime + 4.188)
    ) * uAmp;

    vec3 center = uCenter + drift + wobble;

    vec3 worldPos = center + inPos * uRadius;

    vWorldPos = worldPos;
    vN = normalize(inNormal);

    gl_Position = uP * uV * vec4(worldPos, 1.0);
}
