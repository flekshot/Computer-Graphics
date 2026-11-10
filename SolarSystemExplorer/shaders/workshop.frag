#version 330 core
in vec3 vWorld;
in vec3 vNormal;
in vec3 vDirection;
in float vHeight;
out vec4 fragColor;
uniform int uLayer;
uniform int uKind;
uniform int uSurfaceMode;
uniform vec3 uEye;
uniform vec3 uViewport;
uniform vec3 uLandColor;
uniform vec3 uWaterColor;
uniform vec3 uAtmosphereColor;
uniform vec3 uGasColorA;
uniform vec3 uGasColorB;
uniform float uSeed;
uniform float uScale;
uniform float uSeaLevel;
uniform float uIceCap;
uniform float uClouds;
uniform float uAtmosphere;
uniform float uGasBands;
uniform float uTurbulence;
uniform float uAmbient;
uniform float uDiffuse;
uniform float uSpecular;
uniform float uExposure;
const float PI = 3.14159265358979323846;

float hash(vec3 p) {
    p = fract(p * .1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}
float noise(vec3 p) {
    vec3 cell = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash(cell), hash(cell + vec3(1, 0, 0)), f.x),
                   mix(hash(cell + vec3(0, 1, 0)), hash(cell + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(hash(cell + vec3(0, 0, 1)), hash(cell + vec3(1, 0, 1)), f.x),
                   mix(hash(cell + vec3(0, 1, 1)), hash(cell + vec3(1, 1, 1)), f.x), f.y), f.z);
}
float fbm(vec3 p) {
    float value = 0.0, amplitude = .52;
    for (int i = 0; i < 5; ++i) {
        value += amplitude * noise(p);
        p = p * 2.03 + vec3(7.1, 3.8, 2.4);
        amplitude *= .48;
    }
    return value;
}
vec3 displayColor(vec3 color) {
    return pow(vec3(1.0) - exp(-max(color, vec3(0.0)) * uExposure), vec3(1.0 / 2.2));
}
void background() {
    vec2 uv = gl_FragCoord.xy / uViewport.xy;
    vec2 centered = (uv - .5) * vec2(uViewport.x / uViewport.y, 1.0);
    float halo = exp(-dot(centered - vec2(.08, .05), centered - vec2(.08, .05)) * 2.0);
    vec3 sky = mix(vec3(.012, .020, .039), vec3(.024, .043, .071), halo);
    vec2 grid = gl_FragCoord.xy / 32.0;
    vec2 cell = floor(grid);
    float seed = hash(vec3(cell, 8.7));
    vec2 center = vec2(hash(vec3(cell, 4.1)), hash(vec3(cell, 9.3))) * .72 + .14;
    float star = (1.0 - smoothstep(.012, .044, length(fract(grid) - center))) * step(.964, seed);
    sky += mix(vec3(.37, .57, .75), vec3(.82, .67, .46), hash(vec3(cell, 2.1))) * star * .72;
    fragColor = vec4(sky, 1.0);
}
void main() {
    if (uLayer == 3) { background(); return; }
    vec3 normal = normalize(vNormal);
    vec3 eye = normalize(uEye - vWorld);
    vec3 direction = normalize(vDirection);
    vec3 light = normalize(vec3(-3.5, 4.3, 6.8));
    float facing = dot(normal, eye);
    float diffuse = max(dot(normal, light), 0.0);
    vec3 offset = vec3(mod(uSeed, 997.0) * .071, mod(uSeed, 313.0) * .113, mod(uSeed, 719.0) * .037);

    if (uLayer == 2) {
        if (facing <= 0.0) discard;
        float rim = pow(1.0 - facing, 3.4);
        float alpha = rim * smoothstep(0.0, .14, facing) * uAtmosphere *
                      (.19 + .47 * smoothstep(-.3, .8, dot(normal, light)));
        fragColor = vec4(displayColor(uAtmosphereColor * (.55 + diffuse * 1.5)), alpha);
        return;
    }
    if (uLayer == 1) {
        if (facing <= 0.0) discard;
        vec3 p = direction * (uScale * 1.6 + 2.0) + offset;
        p += vec3(fbm(p + 11.3), fbm(p - 5.7), fbm(p + 3.8)) * 1.8;
        float cloud = fbm(p);
        float threshold = mix(.82, .22, uClouds);
        float alpha = smoothstep(threshold, threshold + .12, cloud) * .68 * smoothstep(.04, .32, facing);
        vec3 color = vec3(.84, .92, 1.0) * (.11 * uAmbient + 1.65 * diffuse * uDiffuse);
        fragColor = vec4(displayColor(color), alpha);
        return;
    }
    if (uSurfaceMode == 2) {
        fragColor = vec4(normal * .5 + .5, 1.0);
        return;
    }
    if (uLayer == 0 && uKind == 0 && uSurfaceMode == 0) {
        normal = normalize(mix(normal, normalize(vWorld), .45));
        facing = dot(normal, eye);
        diffuse = max(dot(normal, light), 0.0);
    }
    float ocean = 0.0;
    vec3 albedo;
    float detail = fbm(direction * (uScale * 7.0) + offset);
    if (uKind == 0) {
        // Height comes from the CPU terrain used to create these triangles.
        ocean = 1.0 - smoothstep(uSeaLevel - .006, uSeaLevel + .009, vHeight);
        float altitude = clamp((vHeight - uSeaLevel) / max(.05, 1.0 - uSeaLevel), 0.0, 1.0);
        vec3 land = uLandColor * (.52 + detail * .68);
        land = mix(land, mix(uLandColor * .6, vec3(.38, .33, .28), .45), smoothstep(.25, .65, altitude));
        float shore = 1.0 - smoothstep(.0, .037, max(0.0, vHeight - uSeaLevel));
        land = mix(land, mix(uLandColor, vec3(.68, .58, .34), .48), shore * .68);
        vec3 water = uWaterColor * (.55 + .26 * detail);
        water = mix(water, uWaterColor * 1.35, smoothstep(uSeaLevel - .06, uSeaLevel, vHeight) * .35);
        water *= 1.30;
        albedo = mix(land, water, ocean);
        float ice = smoothstep(1.0 - uIceCap - .025, 1.0 - uIceCap + .025,
                              abs(direction.y) + (detail - .5) * .07);
        ice *= step(.001, uIceCap);
        albedo = mix(albedo, vec3(.78, .88, .94) * (.85 + .15 * detail), ice);
        ocean *= 1.0 - ice;
    } else {
        float latitude = asin(clamp(direction.y, -1.0, 1.0));
        vec3 turbulencePoint = direction * (uScale * 1.2 + 2.0) + offset;
        float warp = (fbm(turbulencePoint) - .5) * uTurbulence * 3.8;
        float band = .5 + .5 * sin((latitude + warp * .22) * uGasBands +
                                  uTurbulence * 1.4 * sin(direction.x * 15.0 + direction.z * 9.0));
        band = smoothstep(.13, .88, band);
        albedo = mix(uGasColorA, uGasColorB, band) * (.76 + detail * .38);
        float thinBands = .5 + .5 * sin(latitude * uGasBands * 8.0 + warp * 6.0);
        albedo *= .91 + thinBands * .12;
    }
    vec3 halfDirection = normalize(light + eye);
    float specular = pow(max(dot(normal, halfDirection), 0.0), mix(24.0, 100.0, ocean));
    specular *= step(.001, diffuse) * mix(.045, .85, ocean);
    float rim = pow(1.0 - max(0.0, facing), 4.0);
    vec3 color = albedo * (.095 * uAmbient + 1.7 * diffuse * uDiffuse);
    color += vec3(.82, .91, 1.0) * specular * uSpecular;
    color += uAtmosphereColor * rim * .13 * uAtmosphere * (.2 + diffuse);
    if (uSurfaceMode == 1)
        color = mix(color, vec3(.19, .56, .55), .48) + vec3(.025);
    fragColor = vec4(displayColor(color), 1.0);
}
