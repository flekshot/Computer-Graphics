#pragma once
#include <algorithm>
#include <cmath>

namespace sse {
inline float cloudRotationRadians(double days) {
    constexpr double twoPi = 6.28318530717958647692;
    return static_cast<float>(std::remainder(days / 1000.0, 1.0) * twoPi);
}

inline float fitVerticalFov(float fieldOfView, float aspect) {
    // Preserve the horizontal field of view in portrait viewports, without
    // crossing pi and reversing the projection's screen axes.
    return 2.0f * std::atan(std::tan(fieldOfView * .5f) / std::clamp(aspect, .001f, 1.0f));
}
} // namespace sse
