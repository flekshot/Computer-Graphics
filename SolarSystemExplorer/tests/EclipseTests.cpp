#include "EclipseModel.h"
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
bool near(double actual, double expected, double tolerance = 1e-9) {
    return std::abs(actual - expected) < tolerance;
}
// Independent solid-angle integration of rays over the visible Sun disk.
// It does not use the production overlap formula or its angular radii helper.
double integratedVisibility(const glm::dvec3& point, const sse::EclipseBody& sun,
                            const sse::EclipseBody& blocker) {
    const glm::dvec3 direction = glm::normalize(sun.center - point);
    const glm::dvec3 right = glm::normalize(glm::cross(direction, glm::dvec3(0, 1, 0)));
    const glm::dvec3 up = glm::cross(right, direction);
    const double distance = glm::length(sun.center - point);
    const double cosineEdge = std::sqrt(1.0 - sun.radius * sun.radius / (distance * distance));
    int visible = 0;
    constexpr int rings = 200, sectors = 720;
    for (int r = 0; r < rings; ++r) {
        const double cosine = 1.0 - (1.0 - cosineEdge) * (r + 0.5) / rings;
        const double sine = std::sqrt(1.0 - cosine * cosine);
        for (int s = 0; s < sectors; ++s) {
            const double angle = (s + 0.5) * 6.283185307179586 / sectors;
            const auto ray = direction * cosine + (right * std::cos(angle) + up * std::sin(angle)) * sine;
            const auto relative = blocker.center - point;
            const double along = glm::dot(relative, ray);
            const double discriminant = blocker.radius * blocker.radius - glm::dot(relative, relative) + along * along;
            if (along <= 0 || discriminant <= 0 || along - std::sqrt(discriminant) >= distance)
                ++visible;
        }
    }
    return double(visible) / (rings * sectors);
}
} // namespace

int main() {
    try {
        using namespace sse;
        check(near(angularDiskVisibleFraction(1, 2, 0), 0), "A larger aligned disk must fully cover the source");
        check(near(angularDiskVisibleFraction(1, .5, 0), .75), "An annular eclipse must preserve the uncovered source area");
        check(near(angularDiskVisibleFraction(1, 1, 1), .6089977810442293), "Equal disks at one radius need the analytic lens area");
        check(near(angularDiskVisibleFraction(1, 1, 2), 1), "Externally tangent disks must leave full light");
        check(near(angularDiskVisibleFraction(1, 2, 1), 0), "Internal tangency of a larger disk must remain total");
        check(near(angularDiskVisibleFraction(1, 0, 0), 1), "Zero-radius occluders must not shadow");
        const EclipseBody source{{10, 0, 0}, 1};
        check(near(visibleLightFraction({0, 0, 0}, source, {{5, 0, 0}, 1}), 0), "A foreground aligned body must shadow");
        check(near(visibleLightFraction({0, 0, 0}, source, {{-5, 0, 0}, 1}), 1), "A blocker behind the receiver must not shadow");
        check(near(visibleLightFraction({0, 0, 0}, source, {{15, 0, 0}, 2}), 1), "A blocker behind the Sun must not shadow");
        check(near(visibleLightFraction({0, 0, 0}, source, {{5, 4, 0}, 1}), 1), "A separated angular disk must not shadow");

        for (bool lunar : {false, true}) {
            EclipseSettings settings;
            settings.lunar = lunar;
            auto scene = makeEclipseScene(settings);
            check(eclipseCenterVisibility(scene) < 1e-9, "Syzygy must create a total eclipse at the target center");
            check(glm::length(scene.moon.center - scene.earth.center) > scene.moon.radius + scene.earth.radius,
                  "The Moon and Earth must never intersect");
            settings.phaseDegrees = lunar ? 20.0f : 8.0f;
            scene = makeEclipseScene(settings);
            const double partial = eclipseCenterVisibility(scene);
            check(partial > .05 && partial < .95, "Partial preset must produce measurable partial shadow");
            settings.phaseDegrees = lunar ? 35.0f : 30.0f;
            check(near(eclipseCenterVisibility(makeEclipseScene(settings)), 1), "Clear preset must leave the target center illuminated");
            settings.phaseDegrees = 0;
            settings.inclinationDegrees = 30;
            check(near(eclipseCenterVisibility(makeEclipseScene(settings)), 1), "Inclination must be able to prevent an eclipse at phase zero");
            settings.inclinationDegrees = 0;
            settings.phaseDegrees = 360;
            check(eclipseCenterVisibility(makeEclipseScene(settings)) < 1e-9, "A full phase revolution must return to totality");

            // Sweep the partial transition and compare with independent 3D ray integration.
            for (float phase : {3.0f, 6.0f, 9.0f, 12.0f, 15.0f, 18.0f, 20.0f, 22.0f, 25.0f, 30.0f}) {
                settings.phaseDegrees = phase;
                scene = makeEclipseScene(settings);
                const auto point = scene.target().center + glm::normalize(scene.sun.center - scene.target().center) * scene.target().radius;
                const double actual = visibleLightFraction(point, scene.sun, scene.occluder());
                check(near(actual, integratedVisibility(point, scene.sun, scene.occluder()), .012),
                      "Angular disk approximation must agree with 3D solid-angle integration within 1.2 percentage points");
                check(actual >= 0 && actual <= 1 && std::isfinite(actual), "Shadow visibility must stay finite and normalized");
            }
        }
        std::cout << "PASS: " << checks << " eclipse assertions (disk overlap, depth ordering, solar/lunar presets, ray integration)\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << " assertions: " << e.what() << '\n';
        return 1;
    }
}
