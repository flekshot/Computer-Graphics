#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;
uniform vec3 uEye;
uniform vec3 uColor;
uniform float uStrength;
uniform bool uSun;
void main() {
    vec3 N=normalize(vNormal),V=normalize(uEye-vWorld);
    float rim=pow(1.0-max(dot(N,V),0.0),3.5);
    float lit=uSun ? 1.0 : 0.12+0.88*max(dot(N,normalize(-vWorld)),0.0);
    FragColor=vec4(uColor,rim*lit*uStrength);
}
