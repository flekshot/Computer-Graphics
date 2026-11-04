#pragma once
#include "LabState.h"
#include <glm/glm.hpp>

namespace sse {
struct EclipseBody {
    glm::dvec3 center{0.0};
    double radius = 0.0;
};

// One coherent illustrative scale, not ephemerides or a calendar prediction.
struct EclipseScene {
    EclipseBody sun, earth, moon;
    bool lunar = false;
    const EclipseBody& target() const { return lunar ? moon : earth; }
    const EclipseBody& occluder() const { return lunar ? earth : moon; }
};

EclipseScene makeEclipseScene(const EclipseSettings& settings);

// Exact overlap of circular disks in angular coordinates (radians).
// This small-angle approximation is validated against spherical-cap integration.
double angularDiskVisibleFraction(double sourceRadius, double occluderRadius, double separation);
double visibleLightFraction(const glm::dvec3& point, const EclipseBody& source,
                            const EclipseBody& occluder);
// The measurement is at the target's sun-facing surface, not a global eclipse classification.
double eclipseCenterVisibility(const EclipseScene& scene);
const char* eclipseStatus(const EclipseScene& scene);
} // namespace sse
