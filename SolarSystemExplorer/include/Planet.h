#pragma once
#include <glm/glm.hpp>
#include <string>
namespace sse {
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
    glm::vec3 position(double days) const;
    glm::mat4 orbitMatrix(double days) const;
    glm::mat4 modelMatrix(double days) const;
    glm::mat4 equatorialFrame(double days) const;

  private:
    PlanetData data_;
};
} // namespace sse
