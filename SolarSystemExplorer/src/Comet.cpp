#include "Comet.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
namespace sse {
namespace {
constexpr float kappa = 0.5522847f; // Cubic Bezier handle ratio that best approximates a quarter ellipse.
constexpr float perihelionDistance = 5.0f, aphelionDistance = 44.0f;
constexpr std::size_t samplesPerSegment = 512;

BezierLoop makePath() {
    const float a = 0.5f * (perihelionDistance + aphelionDistance);
    const float c = a - perihelionDistance;
    const float b = std::sqrt(a * a - c * c);
    // Four anchors at the ends of the ellipse axes. The perihelion handle is
    // shortened so the curve bends more tightly around the Sun, and the side
    // anchors leave the ecliptic in opposite directions, producing a tilted,
    // slightly twisted loop instead of a flat textbook ellipse.
    std::vector<BezierLoop::Anchor> anchors{
        {{perihelionDistance, 0, 0}, {0, 0, kappa * b * 0.8f}},
        {{-c, 4.0f, b}, {-kappa * a, 0, 0}},
        {{-aphelionDistance, 0, 0}, {0, 0, -kappa * b}},
        {{-c, -4.0f, -b}, {kappa * a, 0, 0}},
    };
    const auto orientation = glm::rotate(glm::mat4(1), glm::radians(40.0f), glm::vec3(0, 1, 0)) *
                             glm::rotate(glm::mat4(1), glm::radians(12.0f), glm::vec3(0, 0, 1));
    for (auto& anchor : anchors) {
        anchor.position = glm::vec3(orientation * glm::vec4(anchor.position, 1));
        anchor.handle = glm::vec3(orientation * glm::vec4(anchor.handle, 0));
    }
    return BezierLoop(std::move(anchors));
}
} // namespace

Comet::Comet() : path_(makePath()), samplesPerSegment_(static_cast<double>(samplesPerSegment)) {
    const std::size_t total = samplesPerSegment * path_.segmentCount();
    perihelion_ = aphelion_ = glm::length(path_.point(0));
    for (std::size_t i = 0; i < total; ++i) {
        const float r = glm::length(path_.point(static_cast<double>(i) / samplesPerSegment_));
        perihelion_ = std::min(perihelion_, r);
        aphelion_ = std::max(aphelion_, r);
    }
    semiMajor_ = 0.5f * (perihelion_ + aphelion_);
    // Integrate dt = |dP/du| du / v(r) with the midpoint rule, then rescale so
    // one full loop lasts exactly periodDays.
    cumulative_.assign(total + 1, 0.0);
    const double du = 1.0 / samplesPerSegment_;
    for (std::size_t i = 0; i < total; ++i) {
        const double u = (static_cast<double>(i) + 0.5) * du;
        const double ds = glm::length(path_.derivative(u)) * du;
        cumulative_[i + 1] = cumulative_[i] + ds / speedWeight(path_.point(u));
    }
    const double scale = periodDays / cumulative_.back();
    for (auto& value : cumulative_)
        value *= scale;
}

float Comet::speedWeight(const glm::vec3& point) const {
    const float r = std::max(glm::length(point), 1.0f);
    return std::sqrt(std::max(2.0f / r - 1.0f / semiMajor_, 0.02f / semiMajor_));
}

double Comet::parameterAt(double days) const {
    if (!std::isfinite(days))
        days = 0;
    double phase = std::fmod(days, periodDays);
    if (phase < 0)
        phase += periodDays;
    const auto upper = std::upper_bound(cumulative_.begin(), cumulative_.end(), phase);
    const auto index = static_cast<std::size_t>(
        std::clamp<std::ptrdiff_t>(upper - cumulative_.begin() - 1, 0,
                                   static_cast<std::ptrdiff_t>(cumulative_.size()) - 2));
    const double span = cumulative_[index + 1] - cumulative_[index];
    const double fraction = span > 0 ? std::clamp((phase - cumulative_[index]) / span, 0.0, 1.0) : 0.0;
    return (static_cast<double>(index) + fraction) / samplesPerSegment_;
}

glm::vec3 Comet::position(double days) const {
    return path_.point(parameterAt(days));
}

glm::vec3 Comet::velocity(double days) const {
    constexpr double h = 0.01;
    return (position(days + h) - position(days - h)) / static_cast<float>(2 * h);
}
} // namespace sse
