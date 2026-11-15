#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    // Soft round sprite: Gaussian falloff from the centre of the point.
    float r=length(gl_PointCoord-vec2(0.5))*2.0;
    if(r>1.0) discard;
    float glow=exp(-3.5*r*r);
    FragColor=vec4(vColor.rgb,vColor.a*glow);
}
