#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uRing;
uniform vec3 uCenter;
uniform float uRadius;
void main() {
    vec4 texel=texture(uRing,vec2(vUV.x,0.5));
    if(texel.a<0.015) discard;
    vec3 L=normalize(-vWorld),q=vWorld-uCenter;
    float b=dot(q,L),c=dot(q,q)-uRadius*uRadius;
    float shadow=(b<0.0 && b*b>c) ? 0.10 : 1.0;
    float diffuse=0.28+0.95*abs(dot(normalize(vNormal),L));
    FragColor=vec4(texel.rgb*diffuse*shadow,texel.a*0.94);
}
