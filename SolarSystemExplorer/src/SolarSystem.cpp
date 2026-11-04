#include "SolarSystem.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
namespace sse {
SolarSystem::SolarSystem()
    : planets_{
          Planet({"Mercury",
                  "TERRESTRIAL",
                  "mercury.jpg",
                  "A small, cratered world with almost no atmosphere. Its sunlit surface becomes extremely "
                  "hot, while the long nights are bitterly cold.",
                  0.48f,
                  7.0f,
                  87.969,
                  58.646,
                  0.034f,
                  7.0f,
                  35,
                  4879,
                  57.9,
                  {0.65f, 0.61f, 0.55f}}),
          Planet(
              {"Venus",
               "TERRESTRIAL",
               "venus_atmosphere.jpg",
               "A brilliant world wrapped in thick clouds. Its dense carbon dioxide atmosphere traps heat, "
               "making Venus the hottest planet. It rotates in the opposite direction to most planets.",
               0.88f,
               10.2f,
               224.701,
               -243.025,
               2.64f,
               3.39f,
               130,
               12104,
               108.2,
               {0.92f, 0.74f, 0.41f}}),
          Planet({"Earth",
                  "TERRESTRIAL",
                  "earth_daymap.jpg",
                  "Our ocean world and the only planet known to host life. Liquid water, a protective "
                  "atmosphere and a magnetic field make Earth a remarkable home. Its Moon helps stabilize "
                  "the axial tilt.",
                  1.0f,
                  14.0f,
                  365.256,
                  0.99727,
                  23.44f,
                  0.0f,
                  215,
                  12742,
                  149.6,
                  {0.32f, 0.65f, 0.98f}}),
          Planet({"Mars",
                  "TERRESTRIAL",
                  "mars.jpg",
                  "The red planet owes its color to iron minerals. Ancient riverbeds and polar ice preserve "
                  "evidence of a wetter past. Olympus Mons is the largest volcano in the Solar System.",
                  0.64f,
                  18.5f,
                  686.980,
                  1.02596,
                  25.19f,
                  1.85f,
                  315,
                  6779,
                  227.9,
                  {0.92f, 0.39f, 0.23f}}),
          Planet({"Jupiter",
                  "GAS GIANT",
                  "jupiter.jpg",
                  "The largest planet is a gas giant with fast winds and colorful cloud bands. Its Great Red "
                  "Spot is a vast, long-lived storm. Jupiter has no solid surface to stand on.",
                  2.65f,
                  27.0f,
                  4332.59,
                  0.41354,
                  3.13f,
                  1.30f,
                  80,
                  139820,
                  778.6,
                  {0.83f, 0.64f, 0.46f}}),
          Planet({"Saturn",
                  "GAS GIANT",
                  "saturn.jpg",
                  "Saturn's spectacular rings are made of countless pieces of ice and rock. This low-density "
                  "gas giant has a rich family of moons, including Titan with its thick atmosphere.",
                  2.20f,
                  38.0f,
                  10759.22,
                  0.44401,
                  26.73f,
                  2.49f,
                  245,
                  116460,
                  1433.5,
                  {0.88f, 0.77f, 0.53f}}),
          Planet({"Uranus",
                  "ICE GIANT",
                  "uranus.jpg",
                  "An ice giant tipped almost onto its side. Methane in its atmosphere absorbs red light, "
                  "giving it a pale blue-green appearance. The extreme tilt produces unusually long seasons.",
                  1.52f,
                  49.0f,
                  30688.5,
                  0.71833,
                  97.77f,
                  0.77f,
                  175,
                  50724,
                  2872.5,
                  {0.46f, 0.85f, 0.87f}}),
          Planet({"Neptune",
                  "ICE GIANT",
                  "neptune.jpg",
                  "Dark, cold and swept by powerful winds, Neptune is the most distant planet. It was first "
                  "located through mathematical predictions of its gravitational effect on Uranus.",
                  1.48f,
                  60.0f,
                  60182.0,
                  0.67125,
                  28.32f,
                  1.77f,
                  15,
                  49244,
                  4495.1,
                  {0.27f, 0.43f, 0.97f}})} {
    for (const auto& planet : planets_)
        defaultOrbits_.push_back(planet.orbitParameters());
}
void SolarSystem::setSpeed(float multiplier) {
    if (std::isfinite(multiplier))
        speed_ = std::clamp(multiplier, 0.05f, 100.0f);
}
bool SolarSystem::setOrbit(size_t index, const OrbitParameters& orbit, std::string& error) {
    if (index >= planets_.size())
        error = "Choose one of the eight planets.";
    else if (!std::isfinite(orbit.semiMajorAxis) || !std::isfinite(orbit.eccentricity) ||
             !std::isfinite(orbit.inclinationDegrees) || !std::isfinite(orbit.periodDays) ||
             !std::isfinite(orbit.phaseDegrees))
        error = "All orbit parameters must be finite numbers.";
    else if (orbit.semiMajorAxis < 6 || orbit.semiMajorAxis > 180)
        error = "Semi-major axis must be between 6 and 180 scene units.";
    else if (orbit.eccentricity < 0 || orbit.eccentricity > 0.85)
        error = "Eccentricity must be between 0 and 0.85.";
    else if (orbit.inclinationDegrees < -60 || orbit.inclinationDegrees > 60)
        error = "Inclination must be between -60 and 60 degrees.";
    else if (orbit.periodDays < 5 || orbit.periodDays > 200000)
        error = "Period must be between 5 and 200000 days.";
    else if (orbit.phaseDegrees < -3600 || orbit.phaseDegrees > 3600)
        error = "Phase must be between -3600 and 3600 degrees.";
    else if (orbit.semiMajorAxis * (1.0 - orbit.eccentricity) <=
             static_cast<double>(sunRadius) + planets_[index].data().radius + 0.4)
        error = "Periapsis must leave the planet outside the Sun with 0.4 units of clearance.";
    else {
        planets_[index].setOrbit(orbit);
        error.clear();
        return true;
    }
    return false;
}
void SolarSystem::resetOrbits() {
    for (size_t i = 0; i < planets_.size(); ++i)
        planets_[i].setOrbit(defaultOrbits_[i]);
}
bool SolarSystem::isModified() const {
    for (size_t i = 0; i < planets_.size(); ++i) {
        const auto& a = planets_[i].orbitParameters();
        const auto& b = defaultOrbits_[i];
        if (a.semiMajorAxis != b.semiMajorAxis || a.eccentricity != b.eccentricity ||
            a.inclinationDegrees != b.inclinationDegrees || a.periodDays != b.periodDays ||
            a.phaseDegrees != b.phaseDegrees)
            return true;
    }
    return false;
}
void SolarSystem::setDays(double days) {
    if (std::isfinite(days))
        days_ = std::clamp(days, 0.0, 1.0e10);
}
void SolarSystem::update(double seconds) {
    if (!paused_ && std::isfinite(seconds) && seconds > 0)
        setDays(days_ + seconds * 6.0 * speed_);
}
glm::mat4 SolarSystem::moonFrame(double days) const {
    // Inherit Earth's translation, never Earth's surface spin or visual radius.
    return glm::translate(glm::mat4(1), planets_[2].position(days)) *
           glm::rotate(glm::mat4(1), glm::radians(5.145f), glm::vec3(0, 0, 1));
}
glm::mat4 SolarSystem::moonModel(double days) const {
    const float angle = static_cast<float>(std::remainder(days / 27.3217, 1.0) * glm::two_pi<double>());
    return moonFrame(days) * glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0)) *
           glm::translate(glm::mat4(1), glm::vec3(moonOrbitRadius, 0, 0)) *
           glm::scale(glm::mat4(1), glm::vec3(moonRadius));
}
glm::vec3 SolarSystem::moonPosition(double days) const {
    return glm::vec3(moonModel(days)[3]);
}
} // namespace sse
