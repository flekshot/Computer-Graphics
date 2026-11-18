#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aOrbit;
uniform mat4 uVP;
uniform float uCycle;
out vec3 vWorld;
flat out float vTone;
void main() {
    float turns = round(43.0 * pow(22.0/aOrbit.x, 1.5));
    float angle = aOrbit.y + turns * uCycle;
    vec3 center = aOrbit.x * vec3(cos(angle), sin(aOrbit.z)*sin(angle+aOrbit.y), sin(angle));
    float spin = aOrbit.y*5.0 + uCycle*(900.0+turns);
    mat2 rotation = mat2(cos(spin),sin(spin),-sin(spin),cos(spin));
    vec3 offset = aPosition*aOrbit.w;
    offset.xz = rotation*offset.xz;
    vWorld = center+offset;
    vTone = fract(aOrbit.y*11.0);
    gl_Position = uVP*vec4(vWorld,1);
}
