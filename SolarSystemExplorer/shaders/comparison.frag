#version 330 core
in vec3 vNormal;
in vec2 vUV;
uniform sampler2D uSurface;
out vec4 fragColor;
void main() {
    vec3 base = texture(uSurface, vUV).rgb; // sRGB texture is decoded by OpenGL.
    vec3 light = normalize(vec3(-0.4, 0.65, 1.0));
    float diffuse = max(dot(normalize(vNormal), light), 0.0);
    vec3 linear = base * (0.28 + 0.72 * diffuse);
    // The offscreen RGBA8 image is displayed directly by ImGui.
    vec3 srgb = mix(12.92 * linear, 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055,
                    step(vec3(0.0031308), linear));
    fragColor = vec4(srgb, 1.0);
}
