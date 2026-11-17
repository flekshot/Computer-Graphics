#include "Bezier.h"
#include <cmath>
#include <stdexcept>
#include <utility>
namespace sse {
glm::vec3 deCasteljau(const glm::vec3* points, std::size_t count, float t) {
    if (!points || count == 0)
        throw std::invalid_argument("De Casteljau needs at least one control point");
    // Small curves (all of ours) stay on the stack; higher degrees use the heap.
    std::array<glm::vec3, 16> local{};
    std::vector<glm::vec3> heap;
    glm::vec3* work = local.data();
    if (count > local.size()) {
        heap.resize(count);
        work = heap.data();
    }
    for (std::size_t i = 0; i < count; ++i)
        work[i] = points[i];
    // Level k replaces n-k points with n-k-1 interpolated points.
    for (std::size_t level = count - 1; level > 0; --level)
        for (std::size_t i = 0; i < level; ++i)
            work[i] = glm::mix(work[i], work[i + 1], t);
    return work[0];
}

glm::vec3 cubicBernstein(const std::array<glm::vec3, 4>& p, float t) {
    const float s = 1.0f - t;
    return s * s * s * p[0] + 3.0f * s * s * t * p[1] + 3.0f * s * t * t * p[2] + t * t * t * p[3];
}

BezierLoop::BezierLoop(std::vector<Anchor> anchors) : anchors_(std::move(anchors)) {
    if (anchors_.size() < 2)
        throw std::invalid_argument("A closed Bezier loop needs at least two anchors");
}

std::array<glm::vec3, 4> BezierLoop::segment(std::size_t index) const {
    const auto& a = anchors_[index % anchors_.size()];
    const auto& b = anchors_[(index + 1) % anchors_.size()];
    return {a.position, a.position + a.handle, b.position - b.handle, b.position};
}

std::size_t BezierLoop::locate(double u, float& local) const {
    const double n = static_cast<double>(anchors_.size());
    if (!std::isfinite(u))
        u = 0;
    u = std::fmod(u, n);
    if (u < 0)
        u += n;
    auto index = static_cast<std::size_t>(u);
    if (index >= anchors_.size())
        index = anchors_.size() - 1;
    local = static_cast<float>(u - static_cast<double>(index));
    return index;
}

glm::vec3 BezierLoop::point(double u) const {
    float t = 0;
    const auto p = segment(locate(u, t));
    return deCasteljau(p.data(), p.size(), t);
}

glm::vec3 BezierLoop::derivative(double u) const {
    // The derivative of a cubic Bezier is a quadratic Bezier on the scaled
    // differences of consecutive control points (the hodograph).
    float t = 0;
    const auto p = segment(locate(u, t));
    const std::array<glm::vec3, 3> hodograph{3.0f * (p[1] - p[0]), 3.0f * (p[2] - p[1]), 3.0f * (p[3] - p[2])};
    return deCasteljau(hodograph.data(), hodograph.size(), t);
}

std::vector<glm::vec3> BezierLoop::controlPolygon() const {
    std::vector<glm::vec3> points;
    points.reserve(anchors_.size() * 3);
    for (std::size_t i = 0; i < anchors_.size(); ++i) {
        const auto s = segment(i);
        points.insert(points.end(), {s[0], s[1], s[2]});
    }
    return points;
}

std::vector<glm::vec3> BezierLoop::sample(std::size_t perSegment) const {
    if (perSegment == 0)
        throw std::invalid_argument("Bezier sampling needs at least one point per segment");
    std::vector<glm::vec3> points;
    const std::size_t total = perSegment * anchors_.size();
    points.reserve(total + 1);
    for (std::size_t i = 0; i <= total; ++i)
        points.push_back(point(static_cast<double>(i) / perSegment));
    return points;
}
} // namespace sse
