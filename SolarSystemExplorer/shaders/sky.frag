#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uSky;
uniform mat4 uInverseVP;
void main() {
    vec4 p=uInverseVP*vec4(vUV*2.0-1.0,1.0,1.0);
    vec3 d=normalize(p.xyz/p.w);
    vec2 uv=vec2(atan(d.z,d.x)/6.2831853+0.5,acos(clamp(d.y,-1.0,1.0))/3.1415927);
    vec3 sky=texture(uSky,uv).rgb;
    FragColor=vec4(vec3(0.0008,0.0014,0.0035)+sky*0.18,1.0);
}
