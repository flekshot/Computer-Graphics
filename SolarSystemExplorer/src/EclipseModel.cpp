#include "EclipseModel.h"
#include <algorithm>
#include <cmath>
namespace sse {
namespace {
constexpr double pi = 3.14159265358979323846;
}
EclipseScene makeEclipseScene(const EclipseSettings& settings) {
    EclipseScene scene{{{-15, 0, 0}, 2.5}, {{4, 0, 0}, 1.45}, {{0, 0, 0}, .7}, settings.lunar};
    const double phase = std::isfinite(settings.phaseDegrees) ? std::remainder(double(settings.phaseDegrees), 360.0) : 0.0;
    const double inclination = std::isfinite(settings.inclinationDegrees) ? double(settings.inclinationDegrees) : 0.0;
    const double angle = (phase + (settings.lunar ? 0.0 : 180.0)) * pi / 180.0;
    const double tilt = inclination * pi / 180.0;
    // Tilt about Z puts the line of nodes perpendicular to syzygy: changing
    // inclination at phase zero demonstrates why most new/full moons miss.
    scene.moon.center = scene.earth.center + glm::dvec3(std::cos(angle) * std::cos(tilt),
                                                       std::cos(angle) * std::sin(tilt),
                                                       std::sin(angle)) * 5.0;
    return scene;
}
double angularDiskVisibleFraction(double sourceRadius, double occluderRadius, double separation) {
    if (!std::isfinite(sourceRadius) || !std::isfinite(occluderRadius) || !std::isfinite(separation) ||
        sourceRadius <= 0 || occluderRadius <= 0)
        return 1.0;
    separation = std::abs(separation);
    if (separation >= sourceRadius + occluderRadius)
        return 1.0;
    if (separation <= std::abs(sourceRadius - occluderRadius))
        return occluderRadius >= sourceRadius ? 0.0 : 1.0 - occluderRadius * occluderRadius / (sourceRadius * sourceRadius);
    const double r2 = sourceRadius * sourceRadius, R2 = occluderRadius * occluderRadius;
    const double d2 = separation * separation;
    const double a = std::acos(std::clamp((d2 + r2 - R2) / (2.0 * separation * sourceRadius), -1.0, 1.0));
    const double b = std::acos(std::clamp((d2 + R2 - r2) / (2.0 * separation * occluderRadius), -1.0, 1.0));
    const double radicand = (-separation + sourceRadius + occluderRadius) *
                           (separation + sourceRadius - occluderRadius) *
                           (separation - sourceRadius + occluderRadius) *
                           (separation + sourceRadius + occluderRadius);
    const double covered = r2 * a + R2 * b - 0.5 * std::sqrt(std::max(0.0, radicand));
    return std::clamp(1.0 - covered / (pi * r2), 0.0, 1.0);
}
double visibleLightFraction(const glm::dvec3& point, const EclipseBody& source,
                            const EclipseBody& occluder) {
    const auto toLight = source.center - point, toBlocker = occluder.center - point;
    const double lightDistance = glm::length(toLight), blockerDistance = glm::length(toBlocker);
    if (source.radius <= 0 || occluder.radius <= 0 || lightDistance <= source.radius ||
        !std::isfinite(lightDistance) || !std::isfinite(blockerDistance))
        return 1.0;
    const double along = glm::dot(toBlocker, toLight) / lightDistance;
    if (along <= 0 || along >= lightDistance)
        return 1.0;
    if (blockerDistance <= occluder.radius)
        return 0.0;
    const double sourceAngle = std::asin(std::clamp(source.radius / lightDistance, 0.0, 1.0));
    const double blockerAngle = std::asin(std::clamp(occluder.radius / blockerDistance, 0.0, 1.0));
    const double separation = std::acos(std::clamp(glm::dot(toLight, toBlocker) / (lightDistance * blockerDistance), -1.0, 1.0));
    return angularDiskVisibleFraction(sourceAngle, blockerAngle, separation);
}
double eclipseCenterVisibility(const EclipseScene& scene) {
    const auto& target = scene.target();
    const auto point = target.center + glm::normalize(scene.sun.center - target.center) * target.radius;
    return visibleLightFraction(point, scene.sun, scene.occluder());
}
const char* eclipseStatus(const EclipseScene& scene) {
    const double visible = eclipseCenterVisibility(scene);
    if (visible <= 0.001)
        return "Target center: umbra (total shadow)";
    if (visible < 0.999)
        return "Target center: penumbra (partial shadow)";
    return "Target center: full sunlight";
}
}
