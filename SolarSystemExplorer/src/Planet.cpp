#include "Planet.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <utility>
namespace sse {
Planet::Planet(PlanetData data) : data_(std::move(data)) {}
glm::mat4 Planet::orbitMatrix(double days) const {
    const float phase =
        glm::radians(data_.phaseDegrees) +
        static_cast<float>(std::remainder(days / data_.orbitalDays, 1.0) * glm::two_pi<double>());
    const auto plane = glm::rotate(glm::mat4(1), glm::radians(data_.inclinationDegrees), glm::vec3(0, 0, 1));
    return plane * glm::rotate(glm::mat4(1), phase, glm::vec3(0, 1, 0)) *
           glm::translate(glm::mat4(1), glm::vec3(data_.orbitRadius, 0, 0));
}
glm::vec3 Planet::position(double days) const {
    return glm::vec3(orbitMatrix(days)[3]);
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
