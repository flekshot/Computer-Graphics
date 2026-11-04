#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
uniform mat4 uModel;
uniform mat4 uVP;
out vec3 vWorld;
out vec3 vNormal;
out vec2 vUV;
void main() {
    vec4 world=uModel*vec4(aPosition,1.0);
    vWorld=world.xyz;
    vNormal=transpose(inverse(mat3(uModel)))*aNormal;
    vUV=aUV;
    gl_Position=uVP*world;
}
