#version 330 core
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uScene;
uniform float uExposure;
uniform bool uNormalColors;
void main() {
    vec3 color = texture(uScene, vUV).rgb;
    if (!uNormalColors) {
        color = vec3(1.0) - exp(-max(color, vec3(0.0)) * uExposure);
        color = pow(color, vec3(1.0 / 2.2));
    }
    fragColor = vec4(color, 1.0);
}
