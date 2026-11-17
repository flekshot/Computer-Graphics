#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
uniform mat4 uModel;
uniform mat4 uVP;
uniform vec3 uEye;
uniform bool uEarth;
// 0 = Blinn-Phong per pixel, 1 = Phong per pixel, 2 = Gouraud (per vertex).
uniform int uShadingModel;
out vec3 vWorld;
out vec3 vNormal;
out vec2 vUV;
// Gouraud terms: lighting is evaluated here once per vertex and the
// rasterizer interpolates the results across each triangle.
out float vNdl;
out float vSpec;
void main() {
    vec4 world=uModel*vec4(aPosition,1.0);
    vWorld=world.xyz;
    vNormal=transpose(inverse(mat3(uModel)))*aNormal;
    vUV=aUV;
    vNdl=0.0;
    vSpec=0.0;
    if(uShadingModel==2) {
        vec3 N=normalize(vNormal),L=normalize(-vWorld),V=normalize(uEye-vWorld);
        vNdl=dot(N,L);
        vSpec=pow(max(dot(N,normalize(L+V)),0.0),uEarth ? 56.0 : 28.0);
    }
    gl_Position=uVP*world;
}
