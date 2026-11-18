#version 330 core
in vec3 vWorld;
flat in float vTone;
out vec4 FragColor;
uniform vec3 uLighting;
uniform bool uNormals;
uniform bool uWireframe;
void main() {
    vec3 rock = mix(vec3(.23,.18,.15),vec3(.53,.48,.42),vTone);
    if (uWireframe) {
        FragColor = vec4(rock,1);
        return;
    }
    vec3 normal = normalize(cross(dFdx(vWorld), dFdy(vWorld)));
    float diffuse = max(dot(normal,normalize(-vWorld)),0.0);
    FragColor = vec4(uNormals ? normal*.5+.5 : rock*(.22*uLighting.x+uLighting.y*diffuse),1);
}
