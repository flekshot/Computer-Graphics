#pragma once
#include <glm/glm.hpp>
#include <string>
namespace sse {
struct OrbitParameters {
    // Scene units, dimensionless eccentricity, degrees, days, and mean anomaly at day zero.
    double semiMajorAxis, eccentricity, inclinationDegrees, periodDays, phaseDegrees;
};
struct PlanetData {
    std::string name, category, texture, description;
    float radius, orbitRadius;
    double orbitalDays, rotationDays;
    float tiltDegrees, inclinationDegrees, phaseDegrees;
    double diameterKm, distanceMillionKm;
    glm::vec3 color;
};
class Planet {
  public:
    explicit Planet(PlanetData data);
    const PlanetData& data() const { return data_; }
    const OrbitParameters& orbitParameters() const { return orbit_; }
    glm::vec3 position(double days) const;
    glm::vec3 velocity(double days) const; // Scene units per simulated day.
    double periapsis() const { return orbit_.semiMajorAxis * (1.0 - orbit_.eccentricity); }
    double apoapsis() const { return orbit_.semiMajorAxis * (1.0 + orbit_.eccentricity); }
    glm::mat4 orbitMatrix(double days) const;
    glm::mat4 modelMatrix(double days) const;
    glm::mat4 equatorialFrame(double days) const;

  private:
    friend class SolarSystem;
    void setOrbit(const OrbitParameters& parameters);
    PlanetData data_;
    OrbitParameters orbit_;
};
} // namespace sse
