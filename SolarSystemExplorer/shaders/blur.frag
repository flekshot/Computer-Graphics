#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uInput;
uniform bool uHorizontal;
uniform bool uExtract;
void main() {
    if (uExtract) {
        FragColor=vec4(max(texture(uInput,vUV).rgb-vec3(1.0),vec3(0.0)),1.0);
        return;
    }
    vec2 texel=1.0/vec2(textureSize(uInput,0));
    vec2 axis=uHorizontal ? vec2(texel.x,0) : vec2(0,texel.y);
    vec3 color=texture(uInput,vUV).rgb*0.227027;
    color+=(texture(uInput,vUV+axis*1.384615).rgb+texture(uInput,vUV-axis*1.384615).rgb)*0.316216;
    color+=(texture(uInput,vUV+axis*3.230769).rgb+texture(uInput,vUV-axis*3.230769).rgb)*0.070270;
    FragColor=vec4(color,1.0);
}
