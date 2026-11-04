#pragma once
namespace sse {
enum class SurfaceMode { Shaded = 0, Wireframe = 1, Normals = 2 };
enum class LabView { Explore = 0, Orbit = 1, Eclipse = 2 };

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
};
} // namespace sse
