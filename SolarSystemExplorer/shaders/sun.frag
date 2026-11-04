#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uSurface;
uniform vec3 uEye;
uniform float uTime;
void main() {
    vec3 surface=texture(uSurface,vUV+vec2(sin(vUV.y*30.0+uTime*0.05)*0.001,0)).rgb;
    float limb=0.65+0.35*max(dot(normalize(vNormal),normalize(uEye-vWorld)),0.0);
    FragColor=vec4((surface*4.2+vec3(1.35,0.44,0.05))*limb,1.0);
}
