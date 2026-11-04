#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    float r=length(gl_PointCoord-vec2(0.5))*2.0;
    if(r>1.0) discard;
    float glow=exp(-4.0*r*r);
    FragColor=vec4(vColor*(0.8+1.8*glow),glow);
}
