#include "WorkshopModel.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance = 0.00002f) {
    return std::abs(a - b) < tolerance;
}
template <class Action> void rejects(Action action, const char* message) {
    bool rejected = false;
    try { action(); } catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, message);
}
bool finite(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
void testTerrain() {
    sse::WorkshopSettings settings;
    const glm::vec3 direction = glm::normalize(glm::vec3(1, 2, 3));
    const auto height = sse::sampleWorkshopTerrain(direction, settings);
    check(height >= 0 && height <= 1, "Terrain samples must be normalized");
    check(height == sse::sampleWorkshopTerrain(direction, settings), "Terrain sampling must be deterministic");
    check(near(height, sse::sampleWorkshopTerrain(direction * 9.0f, settings)),
          "Terrain must depend on direction rather than input-vector length");
    settings.seed += 1;
    check(!near(height, sse::sampleWorkshopTerrain(direction, settings), 0.001f),
          "Changing the seed must change the terrain");
    settings.seed -= 1;
    settings.featureScale = 8;
    check(!near(height, sse::sampleWorkshopTerrain(direction, settings), 0.001f),
          "Feature scale must change the terrain");
    for (const glm::vec3 point : {glm::vec3(0, 1, 0), glm::vec3(0, -1, 0),
                                 glm::vec3(1, .1f, 0), glm::vec3(1, .1f, .000001f)}) {
        const float h = sse::sampleWorkshopTerrain(point, settings);
        check(std::isfinite(h) && h >= 0 && h <= 1, "Polar and seam samples must remain finite and normalized");
    }
    check(near(sse::sampleWorkshopTerrain({1, .1f, -.000001f}, settings),
               sse::sampleWorkshopTerrain({1, .1f, .000001f}, settings)),
          "Terrain must remain continuous across the longitude seam");
    rejects([&] { sse::sampleWorkshopTerrain(glm::vec3(0), settings); }, "A zero terrain direction must be rejected");
    rejects([&] { sse::sampleWorkshopTerrain({std::numeric_limits<float>::infinity(), 0, 0}, settings); },
            "A non-finite terrain direction must be rejected");
}
void testGeometry() {
    sse::WorkshopSettings settings;
    settings.seaLevel = 0;
    settings.relief = .25f;
    constexpr unsigned segments = 48, rings = 24;
    const auto geometry = sse::makeWorkshopPlanet(settings, segments, rings);
    const auto repeated = sse::makeWorkshopPlanet(settings, segments, rings);
    check(geometry.vertices.size() == 49u * 25u, "Workshop mesh must retain a complete sphere UV grid");
    check(geometry.indices.size() == 48u * 23u * 6u, "Workshop sphere must omit degenerate polar triangles");
    float smallest = 100, largest = 0;
    bool tiltedNormal = false;
    for (size_t i = 0; i < geometry.vertices.size(); ++i) {
        const auto& vertex = geometry.vertices[i];
        check(vertex.position == repeated.vertices[i].position && vertex.normal == repeated.vertices[i].normal,
              "Identical settings must reproduce identical positions and normals");
        check(finite(vertex.position) && finite(vertex.normal) && near(glm::length(vertex.normal), 1),
              "Workshop vertices must have finite positions and unit normals");
        const float radius = glm::length(vertex.position);
        smallest = std::min(smallest, radius);
        largest = std::max(largest, radius);
        check(glm::dot(vertex.normal, vertex.position) > 0, "Displaced normals must point outward");
        tiltedNormal |= glm::dot(glm::normalize(vertex.position), vertex.normal) < .995f;
        check(vertex.uv.x >= 0 && vertex.uv.x <= 1 && vertex.uv.y >= 0 && vertex.uv.y <= 1,
              "Workshop texture coordinates must remain normalized");
    }
    check(largest - smallest > .05f, "Rocky geometry must physically displace the surface");
    check(tiltedNormal, "Displaced terrain normals must follow the terrain rather than the sphere");
    for (unsigned y = 0; y <= rings; ++y) {
        const auto& first = geometry.vertices[y * (segments + 1)];
        const auto& last = geometry.vertices[y * (segments + 1) + segments];
        check(glm::length(first.position - last.position) < .000001f &&
                  glm::length(first.normal - last.normal) < .000001f,
              "Duplicate longitude vertices must share positions and smooth normals");
    }
    for (unsigned x = 1; x <= segments; ++x) {
        check(geometry.vertices[x].position == geometry.vertices[0].position &&
                  geometry.vertices[x].normal == geometry.vertices[0].normal,
              "The north-pole duplicates must have one smooth normal");
        const size_t south = rings * (segments + 1);
        check(geometry.vertices[south + x].position == geometry.vertices[south].position &&
                  geometry.vertices[south + x].normal == geometry.vertices[south].normal,
              "The south-pole duplicates must have one smooth normal");
    }
    for (size_t i = 0; i < geometry.indices.size(); i += 3) {
        const auto a = geometry.vertices.at(geometry.indices[i]).position;
        const auto b = geometry.vertices.at(geometry.indices[i + 1]).position;
        const auto c = geometry.vertices.at(geometry.indices[i + 2]).position;
        check(glm::dot(glm::cross(b - a, c - a), a + b + c) > 0,
              "All displaced triangles must use outward counterclockwise winding");
    }
    settings.seed += 1;
    const auto alternate = sse::makeWorkshopPlanet(settings, segments, rings);
    check(glm::length(alternate.vertices[311].position - geometry.vertices[311].position) > .0001f,
          "Mesh generation must use the selected seed");
    settings.seaLevel = 1;
    const auto ocean = sse::makeWorkshopPlanet(settings, 16, 8);
    for (const auto& vertex : ocean.vertices) {
        check(near(glm::length(vertex.position), settings.radius * (1 + settings.relief)),
              "Submerged terrain must flatten onto one sea-level sphere");
        check(glm::dot(glm::normalize(vertex.position), vertex.normal) > .99999f,
              "A fully covered ocean must have radial normals");
    }
    settings.kind = sse::WorkshopKind::Gas;
    const auto gas = sse::makeWorkshopPlanet(settings, 16, 8);
    for (const auto& vertex : gas.vertices)
        check(near(glm::length(vertex.position), settings.radius), "Gas giants must remain a round sphere");
    settings.kind = sse::WorkshopKind::Rocky;
    settings.relief = 0;
    const auto flat = sse::makeWorkshopPlanet(settings, 16, 8);
    for (const auto& vertex : flat.vertices)
        check(near(glm::length(vertex.position), settings.radius), "Zero relief must produce the configured radius");
}
void testOceanAndPresets() {
    auto settings = sse::workshopPreset(0);
    check(settings.kind == sse::WorkshopKind::Rocky, "Earthlike preset must be rocky");
    const float coverage = sse::workshopOceanCoverage(settings);
    check(coverage >= .45f && coverage <= .75f, "Earthlike preset must give a useful ocean-world starting point");
    check(coverage == sse::workshopOceanCoverage(settings), "Coverage must be deterministic");
    settings.seaLevel = 0;
    check(sse::workshopOceanCoverage(settings) == 0, "The lowest sea level must have no ocean");
    float previous = 0;
    for (float level : {.25f, .45f, .5f, .55f, .75f, 1.0f}) {
        settings.seaLevel = level;
        const float next = sse::workshopOceanCoverage(settings);
        check(next >= previous && next >= 0 && next <= 1, "Raising sea level must never reduce area coverage");
        previous = next;
    }
    check(previous == 1, "The highest sea level must cover the whole world");
    settings = sse::workshopPreset(3);
    check(settings.kind == sse::WorkshopKind::Gas && sse::workshopOceanCoverage(settings) == 0,
          "A gas world must report no surface ocean");
    const auto volcanic = sse::workshopPreset(1), ice = sse::workshopPreset(2);
    check(volcanic.kind == sse::WorkshopKind::Rocky && volcanic.relief >= .1f && volcanic.seaLevel < .2f,
          "Volcanic preset must expose pronounced rocky terrain");
    check(ice.kind == sse::WorkshopKind::Rocky && ice.iceCap > .3f, "Ice preset must have broad polar ice");
    for (int index = 0; index < 4; ++index)
        check(!sse::makeWorkshopPlanet(sse::workshopPreset(index), 8, 4).vertices.empty(), "Every preset must be valid");
    rejects([] { sse::workshopPreset(-1); }, "Negative preset index must be rejected");
    rejects([] { sse::workshopPreset(4); }, "Unknown preset index must be rejected");
}
void testBounds() {
    const sse::WorkshopSettings valid;
    rejects([&] { sse::makeWorkshopPlanet(valid, 2, 8); }, "Too few segments must be rejected");
    rejects([&] { sse::makeWorkshopPlanet(valid, 8, 1); }, "Too few rings must be rejected");
    rejects([&] { sse::makeWorkshopPlanet(valid, 1025, 8); }, "Excessive segments must be rejected");
    rejects([&] { sse::makeWorkshopPlanet(valid, 8, 513); }, "Excessive rings must be rejected");
    for (auto member : {&sse::WorkshopSettings::radius, &sse::WorkshopSettings::relief,
                        &sse::WorkshopSettings::featureScale, &sse::WorkshopSettings::seaLevel,
                        &sse::WorkshopSettings::iceCap, &sse::WorkshopSettings::clouds,
                        &sse::WorkshopSettings::atmosphere, &sse::WorkshopSettings::rotationDegrees,
                        &sse::WorkshopSettings::rotationSpeed, &sse::WorkshopSettings::gasTurbulence}) {
        auto invalid = valid;
        invalid.*member = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { sse::makeWorkshopPlanet(invalid, 8, 4); }, "Every float setting must reject NaN");
        rejects([&] { sse::workshopOceanCoverage(invalid); }, "Coverage must reject invalid settings");
        rejects([&] { sse::sampleWorkshopTerrain({1, 0, 0}, invalid); }, "Sampling must reject invalid settings");
    }
    const std::array<std::pair<float sse::WorkshopSettings::*, std::pair<float, float>>, 10> bounds{{
        {&sse::WorkshopSettings::radius, {.5f, 3.0f}}, {&sse::WorkshopSettings::relief, {0.0f, .25f}},
        {&sse::WorkshopSettings::featureScale, {1.0f, 12.0f}}, {&sse::WorkshopSettings::seaLevel, {0.0f, 1.0f}},
        {&sse::WorkshopSettings::iceCap, {0.0f, .6f}}, {&sse::WorkshopSettings::clouds, {0.0f, 1.0f}},
        {&sse::WorkshopSettings::atmosphere, {0.0f, 1.0f}}, {&sse::WorkshopSettings::rotationDegrees, {-3600.0f, 3600.0f}},
        {&sse::WorkshopSettings::rotationSpeed, {-45.0f, 45.0f}}, {&sse::WorkshopSettings::gasTurbulence, {0.0f, 1.0f}}}};
    for (const auto& bound : bounds) {
        for (float endpoint : {bound.second.first, bound.second.second}) {
            auto settings = valid; settings.*bound.first = endpoint;
            check(!sse::makeWorkshopPlanet(settings, 3, 2).vertices.empty(), "Inclusive setting endpoints must be valid");
        }
        for (float outside : {bound.second.first - .01f, bound.second.second + .01f}) {
            auto settings = valid; settings.*bound.first = outside;
            rejects([&] { sse::makeWorkshopPlanet(settings, 3, 2); }, "Out-of-range settings must be rejected");
        }
    }
    auto invalid = valid; invalid.seed = 1000000;
    rejects([&] { sse::makeWorkshopPlanet(invalid, 3, 2); }, "Seed outside persisted range must be rejected");
    invalid = valid; invalid.kind = static_cast<sse::WorkshopKind>(2);
    rejects([&] { sse::makeWorkshopPlanet(invalid, 3, 2); }, "Unknown world kinds must be rejected");
    for (int bands : {2, 25}) {
        invalid = valid; invalid.gasBands = bands;
        rejects([&] { sse::makeWorkshopPlanet(invalid, 3, 2); }, "Unsupported gas band counts must be rejected");
    }
    for (auto member : {&sse::WorkshopSettings::landColor, &sse::WorkshopSettings::waterColor,
                        &sse::WorkshopSettings::atmosphereColor, &sse::WorkshopSettings::gasColorA,
                        &sse::WorkshopSettings::gasColorB})
        for (float bad : {-.01f, 1.01f, std::numeric_limits<float>::infinity()}) {
            invalid = valid; (invalid.*member)[1] = bad;
            rejects([&] { sse::makeWorkshopPlanet(invalid, 3, 2); }, "All color channels must remain finite and normalized");
        }
}
}
int main() {
    try {
        testTerrain(); testGeometry(); testOceanAndPresets(); testBounds();
        std::cout << "PASS: " << checks << " workshop assertions (terrain, geometry, oceans, presets, bounds)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
        return 1;
    }
}
