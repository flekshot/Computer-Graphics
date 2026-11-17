#include "WorkshopModel.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sse {
namespace {
constexpr double pi = 3.14159265358979323846;
bool inRange(float value, float minimum, float maximum) {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}
void validate(const WorkshopSettings& settings) {
    if ((settings.kind != WorkshopKind::Rocky && settings.kind != WorkshopKind::Gas) ||
        settings.seed > 999999 || !inRange(settings.radius, .5f, 3.0f) ||
        !inRange(settings.relief, 0, .25f) || !inRange(settings.featureScale, 1, 12) ||
        !inRange(settings.seaLevel, 0, 1) || !inRange(settings.iceCap, 0, .6f) ||
        !inRange(settings.clouds, 0, 1) || !inRange(settings.atmosphere, 0, 1) ||
        !inRange(settings.rotationDegrees, -3600, 3600) || !inRange(settings.rotationSpeed, -45, 45) ||
        settings.gasBands < 3 || settings.gasBands > 24 || !inRange(settings.gasTurbulence, 0, 1))
        throw std::invalid_argument("Workshop settings outside supported limits");
    for (const auto* color : {&settings.landColor, &settings.waterColor, &settings.atmosphereColor,
                              &settings.gasColorA, &settings.gasColorB})
        for (float channel : *color)
            if (!inRange(channel, 0, 1))
                throw std::invalid_argument("Workshop colors must be finite values in [0, 1]");
}
uint32_t mixBits(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}
float lattice(int x, int y, int z, uint32_t seed) {
    const uint32_t bits = mixBits(uint32_t(x) * 0x8da6b343u ^ uint32_t(y) * 0xd8163841u ^
                                 uint32_t(z) * 0xcb1ab31fu ^ mixBits(seed + 0x9e3779b9u));
    return float(bits >> 8) / 16777215.0f;
}
float fade(float value) {
    // Quintic interpolation gives continuous first and second derivatives at lattice edges.
    return value * value * value * (value * (value * 6 - 15) + 10);
}
float noise(const glm::vec3& point, uint32_t seed) {
    const auto cell = glm::ivec3(glm::floor(point));
    const auto fraction = point - glm::vec3(cell);
    const glm::vec3 weight(fade(fraction.x), fade(fraction.y), fade(fraction.z));
    const float x00 = glm::mix(lattice(cell.x, cell.y, cell.z, seed),
                              lattice(cell.x + 1, cell.y, cell.z, seed), weight.x);
    const float x10 = glm::mix(lattice(cell.x, cell.y + 1, cell.z, seed),
                              lattice(cell.x + 1, cell.y + 1, cell.z, seed), weight.x);
    const float x01 = glm::mix(lattice(cell.x, cell.y, cell.z + 1, seed),
                              lattice(cell.x + 1, cell.y, cell.z + 1, seed), weight.x);
    const float x11 = glm::mix(lattice(cell.x, cell.y + 1, cell.z + 1, seed),
                              lattice(cell.x + 1, cell.y + 1, cell.z + 1, seed), weight.x);
    return glm::mix(glm::mix(x00, x10, weight.y), glm::mix(x01, x11, weight.y), weight.z);
}
float terrain(const glm::vec3& direction, const WorkshopSettings& settings) {
    glm::vec3 point = direction * settings.featureScale;
    float height = 0, amplitude = 1, total = 0;
    for (uint32_t octave = 0; octave < 5; ++octave) {
        height += noise(point, settings.seed + octave * 1013u) * amplitude;
        total += amplitude;
        point *= 2.0f;
        amplitude *= .5f;
    }
    return std::clamp((height / total - .5f) * 1.65f + .5f, 0.0f, 1.0f);
}
glm::vec3 surfacePoint(const glm::vec3& direction, const WorkshopSettings& settings) {
    const float level = std::max(terrain(direction, settings), settings.seaLevel);
    return direction * (settings.radius * (1.0f + settings.relief * (level * 2.0f - 1.0f)));
}
glm::vec3 surfaceNormal(const glm::vec3& direction, const WorkshopSettings& settings) {
    // Sample an orthonormal tangent frame, independent of latitude/longitude and mesh density.
    // This also supplies the exact same normal for all duplicated seam/pole vertices.
    const glm::vec3 axis = std::abs(direction.y) < .9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    const auto tangent = glm::normalize(glm::cross(axis, direction));
    const auto bitangent = glm::cross(direction, tangent);
    constexpr float step = .001f;
    const auto alongTangent = surfacePoint(glm::normalize(direction + tangent * step), settings) -
                              surfacePoint(glm::normalize(direction - tangent * step), settings);
    const auto alongBitangent = surfacePoint(glm::normalize(direction + bitangent * step), settings) -
                                surfacePoint(glm::normalize(direction - bitangent * step), settings);
    const auto normal = glm::normalize(glm::cross(alongTangent, alongBitangent));
    return glm::dot(normal, direction) < 0 ? -normal : normal;
}
} // namespace

float sampleWorkshopTerrain(const glm::vec3& unitDirection, const WorkshopSettings& settings) {
    validate(settings);
    // Double precision normalization also accepts tiny/large finite float vectors safely.
    const glm::dvec3 direction(unitDirection);
    const double length = glm::length(direction);
    if (!std::isfinite(length) || length <= 0)
        throw std::invalid_argument("Terrain direction must be finite and nonzero");
    return terrain(glm::vec3(direction / length), settings);
}

Geometry makeWorkshopPlanet(const WorkshopSettings& settings, unsigned segments, unsigned rings) {
    validate(settings);
    auto geometry = makeSphere(segments, rings);
    const bool spherical = settings.kind == WorkshopKind::Gas || settings.relief == 0;
    for (unsigned y = 0; y <= rings; ++y) {
        const double theta = double(y) * pi / rings;
        for (unsigned x = 0; x <= segments; ++x) {
            // Use exact endpoints so floating-point sin(2*pi) cannot create a shading seam.
            const double phi = x == segments ? 0.0 : double(x) * 2.0 * pi / segments;
            const auto direction = y == 0 ? glm::vec3(0, 1, 0) :
                                   y == rings ? glm::vec3(0, -1, 0) :
                                   glm::normalize(glm::vec3(std::sin(theta) * std::cos(phi), std::cos(theta),
                                                            std::sin(theta) * std::sin(phi)));
            auto& vertex = geometry.vertices[y * (segments + 1) + x];
            vertex.position = spherical ? direction * settings.radius : surfacePoint(direction, settings);
            vertex.normal = spherical ? direction : surfaceNormal(direction, settings);
        }
    }
    return geometry;
}

float workshopOceanCoverage(const WorkshopSettings& settings) {
    validate(settings);
    if (settings.kind == WorkshopKind::Gas || settings.seaLevel == 0)
        return 0;
    if (settings.seaLevel == 1)
        return 1;
    // A Fibonacci sphere gives equal area per sample, unlike an unweighted UV grid.
    constexpr unsigned samples = 4096;
    constexpr double goldenAngle = 2.39996322972865332;
    unsigned ocean = 0;
    for (unsigned i = 0; i < samples; ++i) {
        const double y = 1.0 - 2.0 * (double(i) + .5) / samples;
        const double radial = std::sqrt(1.0 - y * y), phi = i * goldenAngle;
        const glm::vec3 direction(radial * std::cos(phi), y, radial * std::sin(phi));
        if (terrain(direction, settings) <= settings.seaLevel)
            ++ocean;
    }
    return float(ocean) / samples;
}

WorkshopSettings workshopPreset(int index) {
    WorkshopSettings settings;
    switch (index) {
    case 0: // Earthlike, with both exposed land and a substantial ocean.
        settings.seaLevel = .58f;
        break;
    case 1:
        settings.seed = 173;
        settings.relief = .22f;
        settings.featureScale = 5.0f;
        settings.seaLevel = .08f;
        settings.iceCap = 0;
        settings.clouds = .12f;
        settings.atmosphere = .32f;
        settings.landColor = {.26f, .12f, .08f};
        settings.waterColor = {.95f, .15f, .015f};
        settings.atmosphereColor = {.95f, .28f, .08f};
        break;
    case 2:
        settings.seed = 804;
        settings.relief = .07f;
        settings.featureScale = 4.0f;
        settings.seaLevel = .34f;
        settings.iceCap = .55f;
        settings.clouds = .22f;
        settings.atmosphere = .42f;
        settings.landColor = {.65f, .78f, .88f};
        settings.waterColor = {.10f, .30f, .47f};
        settings.atmosphereColor = {.45f, .72f, .96f};
        break;
    case 3:
        settings.kind = WorkshopKind::Gas;
        settings.seed = 312;
        settings.radius = 1.65f;
        settings.gasBands = 14;
        settings.gasTurbulence = .65f;
        settings.clouds = 0;
        settings.atmosphere = .6f;
        settings.rotationSpeed = 9.0f;
        break;
    default:
        throw std::invalid_argument("Workshop preset index must be between 0 and 3");
    }
    return settings;
}
} // namespace sse
