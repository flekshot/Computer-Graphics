#version 330 core
layout(location=0) in vec3 aPosition;
uniform mat4 uModel;
uniform mat4 uVP;
uniform int uCone;
uniform float uRadius0;
uniform float uRadius1;
void main() {
    vec3 p = aPosition;
    if (uCone != 0)
        p.yz *= mix(uRadius0, uRadius1, p.x);
    gl_Position = uVP * uModel * vec4(p, 1.0);
}
