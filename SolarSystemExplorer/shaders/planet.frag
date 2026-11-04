#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uSurface;
uniform sampler2D uNight;
uniform sampler2D uRing;
uniform vec3 uEye;
uniform bool uEarth;
uniform bool uClouds;
uniform bool uRingShadow;
uniform vec3 uBodyCenter;
uniform vec3 uRingNormal;
uniform float uRadius;
void main() {
    vec3 N=normalize(vNormal),L=normalize(-vWorld),V=normalize(uEye-vWorld);
    vec3 H=normalize(L+V);
    float ndl=dot(N,L),shadow=1.0;
    if(uRingShadow) {
        float den=dot(L,uRingNormal);
        if(abs(den)>0.0001) {
            float t=dot(uBodyCenter-vWorld,uRingNormal)/den;
            float r=length(vWorld+t*L-uBodyCenter)/uRadius;
            if(t>0.01 && r>1.25 && r<2.3) shadow=1.0-0.8*texture(uRing,vec2((r-1.25)/1.05,0.5)).a;
        }
    }
    vec3 albedo=texture(uSurface,vUV).rgb;
    if(uClouds) {
        float density=dot(albedo,vec3(0.3333));
        FragColor=vec4(vec3(0.06+1.4*max(ndl,0.0)),density*0.76);
        return;
    }
    float ocean=uEarth ? smoothstep(0.02,0.13,albedo.b-albedo.r) : 0.0;
    float spec=pow(max(dot(N,H),0.0),uEarth ? 56.0 : 28.0)*(uEarth ? 0.12+ocean*0.65 : 0.055);
    vec3 color=albedo*(0.035+max(ndl,0.0)*1.8*shadow)*vec3(1.0,0.96,0.9);
    color+=spec*max(ndl,0.0)*shadow;
    if(uEarth) color+=texture(uNight,vUV).rgb*(1.0-smoothstep(-0.12,0.20,ndl))*2.0;
    FragColor=vec4(color,1.0);
}
