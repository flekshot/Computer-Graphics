#pragma once
#include <array>
#include <cstdint>
namespace sse {
enum class SurfaceMode { Shaded = 0, Wireframe = 1, Normals = 2 };
enum class LabView { Explore = 0, Orbit = 1, Eclipse = 2, Workshop = 3 };
enum class WorkshopKind { Rocky = 0, Gas = 1 };

struct WorkshopSettings {
    WorkshopKind kind = WorkshopKind::Rocky;
    uint32_t seed = 42;
    float radius = 1.25f, relief = 0.12f, featureScale = 3.5f;
    float seaLevel = 0.50f, iceCap = 0.15f, clouds = 0.40f, atmosphere = 0.55f;
    float rotationDegrees = 0.0f, rotationSpeed = 6.0f;
    bool animate = true;
    int gasBands = 12;
    float gasTurbulence = 0.45f;
    std::array<float, 3> landColor{0.18f, 0.42f, 0.22f};
    std::array<float, 3> waterColor{0.02f, 0.18f, 0.40f};
    std::array<float, 3> atmosphereColor{0.18f, 0.48f, 0.95f};
    std::array<float, 3> gasColorA{0.75f, 0.46f, 0.25f};
    std::array<float, 3> gasColorB{0.96f, 0.84f, 0.62f};
};

struct MissionProgress {
    std::array<bool, 5> completed{};
    int active = -1;
};

struct RenderSettings {
    bool orbits = true, labels = true, bloom = true;
    float exposure = 1.05f;
    SurfaceMode surfaceMode = SurfaceMode::Shaded;
    float ambient = 1.0f, diffuse = 1.0f, specular = 1.0f;
    bool trails = false, velocityVectors = false, apsides = false;
};

struct EclipseSettings {
    bool lunar = false, animate = false, showCones = true, observerView = false;
    float phaseDegrees = 0.0f, inclinationDegrees = 0.0f, angularSpeed = 5.0f;
};

struct LabState {
    LabView view = LabView::Explore;
    EclipseSettings eclipse;
    WorkshopSettings workshop;
    MissionProgress missions;
};
} // namespace sse
