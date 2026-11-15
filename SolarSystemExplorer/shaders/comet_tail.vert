#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=1) in vec4 aColor;
layout(location=2) in float aSize;
uniform mat4 uVP;
// Projection[1][1] * viewport height / 2: converts a world-space size into
// pixels for both perspective (divide by w) and orthographic (w == 1) views.
uniform float uPixelScale;
out vec4 vColor;
void main() {
    gl_Position=uVP*vec4(aPosition,1.0);
    gl_PointSize=clamp(aSize*uPixelScale/max(gl_Position.w,0.001),1.5,96.0);
    vColor=aColor;
}
