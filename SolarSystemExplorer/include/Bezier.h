#pragma once
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <vector>
namespace sse {
// De Casteljau's algorithm: repeated linear interpolation between neighbouring
// control points. It works for any degree and is numerically stable, which is
// why it is preferred over expanding the Bernstein polynomials directly.
glm::vec3 deCasteljau(const glm::vec3* points, std::size_t count, float t);
inline glm::vec3 deCasteljau(const std::vector<glm::vec3>& points, float t) {
    return deCasteljau(points.data(), points.size(), t);
}
// Explicit Bernstein form of a cubic, kept for tests and for the report.
glm::vec3 cubicBernstein(const std::array<glm::vec3, 4>& p, float t);

// A closed, C1-continuous chain of cubic Bezier segments. Each anchor owns a
// handle vector; the incoming control point is (position - handle) and the
// outgoing one is (position + handle). Because both handles are collinear and
// equal in length, the tangent is continuous across every joint.
class BezierLoop {
  public:
    struct Anchor {
        glm::vec3 position, handle;
    };
    explicit BezierLoop(std::vector<Anchor> anchors);
    std::size_t segmentCount() const { return anchors_.size(); }
    std::array<glm::vec3, 4> segment(std::size_t index) const;
    // Global parameter u in [0, segmentCount()); values outside wrap around.
    glm::vec3 point(double u) const;
    glm::vec3 derivative(double u) const; // dP/du
    // All 3n control points in drawing order (anchor, out-handle, in-handle, ...).
    std::vector<glm::vec3> controlPolygon() const;
    // Evenly spaced samples in u, closed (first point repeated at the end).
    std::vector<glm::vec3> sample(std::size_t perSegment) const;

  private:
    std::size_t locate(double u, float& local) const;
    std::vector<Anchor> anchors_;
};
} // namespace sse
