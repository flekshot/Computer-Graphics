#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aColor;
layout(location=2) in vec2 aSizePhase;
uniform mat4 uVP;
uniform float uTime;
out vec3 vColor;
void main() {
    vec4 p=uVP*vec4(aPosition,1.0);
    gl_Position=p.xyww;
    gl_PointSize=aSizePhase.x;
    vColor=aColor*(0.7+0.3*sin(uTime*0.5+aSizePhase.y));
}
