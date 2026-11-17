#pragma once
#include "Bezier.h"
#include <vector>
namespace sse {
// A periodic comet whose path is a closed cubic Bezier loop. The loop shape is
// designed by hand (perihelion inside Mercury's orbit, aphelion between Saturn
// and Uranus); its timing follows the vis-viva relation v ~ sqrt(2/r - 1/a), so
// the comet races past the Sun and drifts slowly through the outer system.
class Comet {
  public:
    static constexpr float radius = 0.22f;
    static constexpr double periodDays = 1200.0;
    Comet();
    const BezierLoop& path() const { return path_; }
    // Bezier parameter u in [0, segmentCount()) reached after the given days.
    double parameterAt(double days) const;
    glm::vec3 position(double days) const;
    glm::vec3 velocity(double days) const; // Scene units per simulated day.
    float perihelion() const { return perihelion_; }
    float aphelion() const { return aphelion_; }

  private:
    float speedWeight(const glm::vec3& point) const;
    BezierLoop path_;
    // cumulative_[i] is the time (days) needed to reach u = i / samplesPerSegment.
    std::vector<double> cumulative_;
    double samplesPerSegment_ = 0;
    float perihelion_ = 0, aphelion_ = 0, semiMajor_ = 0;
};
} // namespace sse
