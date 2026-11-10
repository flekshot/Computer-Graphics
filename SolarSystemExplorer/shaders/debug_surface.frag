#version 330 core
in vec3 vNormal;
out vec4 FragColor;
uniform bool uNormals;
uniform vec3 uColor;
void main() {
    FragColor=vec4(uNormals ? normalize(vNormal)*0.5+0.5 : uColor,1.0);
}
