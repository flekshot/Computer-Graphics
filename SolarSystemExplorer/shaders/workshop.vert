#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uVP;
uniform mat4 uModel;
uniform int uLayer;
out vec3 vWorld;
out vec3 vNormal;
out vec3 vDirection;
out float vHeight;
void main() {
    if (uLayer == 3) {
        vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
        gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
        vWorld = vec3(0.0);
        vNormal = vec3(0.0, 1.0, 0.0);
        vDirection = vec3(0.0, 1.0, 0.0);
        vHeight = 0.0;
        return;
    }
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorld = world.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vDirection = normalize(aPosition);
    vHeight = aUV.x;
    gl_Position = uVP * world;
}
