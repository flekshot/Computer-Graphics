#pragma once
#include "Planet.h"
#include <vector>
namespace sse {
class SolarSystem {
  public:
    static constexpr float sunRadius = 3.4f;
    static constexpr float moonRadius = 0.28f;
    static constexpr float moonOrbitRadius = 2.3f;
    SolarSystem();
    const std::vector<Planet>& planets() const { return planets_; }
    void update(double seconds);
    double days() const { return days_; }
    void setDays(double days);
    float speed() const { return speed_; }
    void setSpeed(float multiplier);
    bool paused() const { return paused_; }
    void setPaused(bool paused) { paused_ = paused; }
    glm::vec3 moonPosition(double days) const;
    glm::mat4 moonModel(double days) const;
    glm::mat4 moonFrame(double days) const;

  private:
    std::vector<Planet> planets_;
    double days_ = 0;
    float speed_ = 1;
    bool paused_ = false;
};
} // namespace sse
