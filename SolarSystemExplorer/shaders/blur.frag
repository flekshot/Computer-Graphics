#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uInput;
uniform bool uHorizontal;
uniform bool uExtract;
vec3 sampleColor(vec2 uv) {
    vec3 c=texture(uInput,uv).rgb;
    return uExtract ? max(c-vec3(1.0),vec3(0.0)) : c;
}
void main() {
    vec2 texel=1.0/vec2(textureSize(uInput,0));
    vec2 axis=uHorizontal ? vec2(texel.x,0) : vec2(0,texel.y);
    vec3 color=sampleColor(vUV)*0.227027;
    color+=(sampleColor(vUV+axis*1.384615)+sampleColor(vUV-axis*1.384615))*0.316216;
    color+=(sampleColor(vUV+axis*3.230769)+sampleColor(vUV-axis*3.230769))*0.070270;
    FragColor=vec4(color,1.0);
}
