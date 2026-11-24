#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sse {
// Pure CPU math: both bodies ALWAYS use the same kilometres-to-world conversion.
struct ComparisonLayout {
    double radiusA, radiusB, centerA, centerB;
    double halfWidth, halfHeight, kilometresPerUnit;
};
inline ComparisonLayout makeComparisonLayout(double diameterA, double diameterB,
                                             double aspect, double zoom = 1.0) {
    if (!std::isfinite(diameterA) || !std::isfinite(diameterB) ||
        diameterA <= 0 || diameterB <= 0 || !std::isfinite(aspect) || aspect <= 0 ||
        !std::isfinite(zoom) || zoom <= 0)
        throw std::invalid_argument("Comparison requires finite positive dimensions");
    const double largest = std::max(diameterA, diameterB);
    const double a = diameterA / largest, b = diameterB / largest;
    constexpr double gap = 0.35;
    const double extent = 2 * a + gap + 2 * b;
    const double halfHeight = std::max(1.32, (extent * 0.5 + 0.25) / aspect) / zoom;
    return {a, b, -extent * 0.5 + a, extent * 0.5 - b,
            halfHeight * aspect, halfHeight, largest * 0.5};
}
} // namespace sse
