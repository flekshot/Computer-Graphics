#include "Planet.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <utility>
namespace sse {
namespace {
double eccentricAnomaly(const OrbitParameters& orbit, double days) {
    const double mean = std::remainder(glm::radians(orbit.phaseDegrees) +
        std::remainder(days / orbit.periodDays, 1.0) * glm::two_pi<double>(), glm::two_pi<double>());
    double eccentric = mean;
    // e <= 0.85 keeps Newton's derivative strictly positive. All calculations stay double precision.
    for (int iteration = 0; iteration < 24; ++iteration) {
        const double step = (eccentric - orbit.eccentricity * std::sin(eccentric) - mean) /
                            (1.0 - orbit.eccentricity * std::cos(eccentric));
        eccentric -= step;
        if (std::abs(step) < 1e-14)
            break;
    }
    return eccentric;
}
glm::vec3 inclined(double x, double z, double inclinationDegrees) {
    const double inclination = glm::radians(inclinationDegrees);
    return glm::vec3(x * std::cos(inclination), x * std::sin(inclination), z);
}
} // namespace
Planet::Planet(PlanetData data) : data_(std::move(data)),
    orbit_{data_.orbitRadius, 0, data_.inclinationDegrees, data_.orbitalDays, data_.phaseDegrees} {}
void Planet::setOrbit(const OrbitParameters& parameters) {
    orbit_ = parameters;
    // Retain the legacy read API for info panels and older callers.
    data_.orbitRadius = static_cast<float>(parameters.semiMajorAxis);
    data_.orbitalDays = parameters.periodDays;
    data_.inclinationDegrees = static_cast<float>(parameters.inclinationDegrees);
    data_.phaseDegrees = static_cast<float>(parameters.phaseDegrees);
}
glm::vec3 Planet::velocity(double days) const {
    const double anomaly = eccentricAnomaly(orbit_, days);
    const double rate = glm::two_pi<double>() / orbit_.periodDays /
                        (1.0 - orbit_.eccentricity * std::cos(anomaly));
    return inclined(-orbit_.semiMajorAxis * std::sin(anomaly) * rate,
                    -orbit_.semiMajorAxis * std::sqrt(1.0 - orbit_.eccentricity * orbit_.eccentricity) *
                        std::cos(anomaly) * rate,
                    orbit_.inclinationDegrees);
}
glm::mat4 Planet::orbitMatrix(double days) const {
    const double eccentric = eccentricAnomaly(orbit_, days);
    const float phase = static_cast<float>(std::atan2(
        std::sqrt(1.0 - orbit_.eccentricity * orbit_.eccentricity) * std::sin(eccentric),
        std::cos(eccentric) - orbit_.eccentricity));
    const auto plane = glm::rotate(glm::mat4(1), static_cast<float>(glm::radians(orbit_.inclinationDegrees)),
                                   glm::vec3(0, 0, 1));
    auto frame = plane * glm::rotate(glm::mat4(1), phase, glm::vec3(0, 1, 0));
    frame[3] = glm::vec4(position(days), 1);
    return frame;
}
glm::vec3 Planet::position(double days) const {
    const double anomaly = eccentricAnomaly(orbit_, days);
    return inclined(orbit_.semiMajorAxis * (std::cos(anomaly) - orbit_.eccentricity),
                    -orbit_.semiMajorAxis * std::sqrt(1.0 - orbit_.eccentricity * orbit_.eccentricity) *
                        std::sin(anomaly),
                    orbit_.inclinationDegrees);
}
glm::mat4 Planet::equatorialFrame(double days) const {
    return glm::translate(glm::mat4(1), position(days)) *
           glm::rotate(glm::mat4(1), glm::radians(data_.tiltDegrees), glm::vec3(0, 0, 1));
}
glm::mat4 Planet::modelMatrix(double days) const {
    // Axial time is slowed 60x for legible surface motion; negative periods are retrograde.
    const auto spin =
        static_cast<float>(std::remainder(days / (data_.rotationDays * 60.0), 1.0) * glm::two_pi<double>());
    return equatorialFrame(days) * glm::rotate(glm::mat4(1), spin, glm::vec3(0, 1, 0)) *
           glm::scale(glm::mat4(1), glm::vec3(data_.radius));
}
} // namespace sse
