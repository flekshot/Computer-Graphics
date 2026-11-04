#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec2 vUV;
out vec4 fragColor;
uniform sampler2D uSurface;
uniform vec3 uSunCenter;
uniform float uSunRadius;
uniform vec3 uOccluderCenter;
uniform float uOccluderRadius;
uniform vec3 uEye;
uniform int uSource;
uniform int uShadowed;
uniform int uSurfaceMode;
uniform float uAmbient;
uniform float uDiffuse;
uniform float uSpecular;
uniform float uExposure;
const float PI = 3.14159265358979323846;

// Exact circular lens area in angular coordinates. EclipseTests compares the
// same CPU model against independent integration over the spherical Sun disk.
float sunlightVisibility(vec3 point) {
    vec3 toLight = uSunCenter - point;
    vec3 toBlocker = uOccluderCenter - point;
    float lightDistance = length(toLight), blockerDistance = length(toBlocker);
    float along = dot(toBlocker, toLight) / lightDistance;
    if (along <= 0.0 || along >= lightDistance || uOccluderRadius <= 0.0)
        return 1.0;
    if (blockerDistance <= uOccluderRadius)
        return 0.0;
    float r = asin(clamp(uSunRadius / lightDistance, 0.0, 1.0));
    float R = asin(clamp(uOccluderRadius / blockerDistance, 0.0, 1.0));
    float d = acos(clamp(dot(toLight, toBlocker) / (lightDistance * blockerDistance), -1.0, 1.0));
    if (d >= r + R)
        return 1.0;
    if (d <= abs(r - R))
        return R >= r ? 0.0 : 1.0 - R * R / (r * r);
    float r2 = r * r, R2 = R * R, d2 = d * d;
    float a = acos(clamp((d2 + r2 - R2) / (2.0 * d * r), -1.0, 1.0));
    float b = acos(clamp((d2 + R2 - r2) / (2.0 * d * R), -1.0, 1.0));
    float lens = r2 * a + R2 * b - 0.5 * sqrt(max(0.0, (-d+r+R)*(d+r-R)*(d-r+R)*(d+r+R)));
    return clamp(1.0 - lens / (PI * r2), 0.0, 1.0);
}
void main() {
    vec3 normal = normalize(vNormal);
    if (uSurfaceMode == 2) {
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    }
    vec3 albedo = texture(uSurface, vUV).rgb;
    vec3 color;
    if (uSource != 0) {
        color = albedo * 3.2 + vec3(0.38, 0.18, 0.015);
    } else {
        vec3 light = normalize(uSunCenter - vWorld);
        float visibility = uShadowed != 0 ? sunlightVisibility(vWorld) : 1.0;
        float diffuse = max(dot(normal, light), 0.0);
        vec3 halfDirection = normalize(light + normalize(uEye - vWorld));
        float specular = pow(max(dot(normal, halfDirection), 0.0), 32.0) * step(0.001, diffuse);
        color = albedo * (0.027 * uAmbient + 1.65 * uDiffuse * diffuse * visibility);
        color += vec3(0.1) * uSpecular * specular * visibility;
    }
    color = vec3(1.0) - exp(-max(color, vec3(0.0)) * uExposure);
    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
