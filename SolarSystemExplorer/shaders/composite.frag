#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform bool uBloomEnabled;
uniform bool uNormalColors;
uniform float uExposure;
vec3 aces(vec3 x) { return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0.0,1.0); }
void main() {
    vec3 color=texture(uScene,vUV).rgb;
    // Diagnostic RGB encodes directions, not light intensity. Preserve that
    // encoding consistently with Workshop and Eclipse rather than tone-map it.
    if(uNormalColors) {
        FragColor=vec4(color,1.0);
        return;
    }
    if(uBloomEnabled) color+=texture(uBloom,vUV).rgb*0.75;
    color=pow(aces(color*uExposure),vec3(1.0/2.2));
    vec2 d=vUV-0.5;
    color*=1.0-0.22*dot(d,d);
    FragColor=vec4(color,1.0);
}
