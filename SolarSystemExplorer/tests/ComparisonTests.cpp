#include "ComparisonLayout.h"
#include <array>
#include <iostream>
#include <limits>

namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
bool near(double a, double b) { return std::abs(a - b) < 1e-10 * std::max({1.0, std::abs(a), std::abs(b)}); }
}
int main() {
    try {
        const std::array<double, 8> diameters{4879, 12104, 12742, 6779, 139820, 116460, 50724, 49244};
        for (double a : diameters) for (double b : diameters)
            for (double aspect : {.25, .75, 1.0, 1.777777, 4.0})
                for (double zoom : {.5, 1.0, 3.0}) {
                    const auto p = sse::makeComparisonLayout(a, b, aspect, zoom);
                    require(near(p.radiusA / p.radiusB, a / b), "Physical size ratio changed");
                    require(near(p.radiusA * 2 * p.kilometresPerUnit, a), "Scale bar units incorrect");
                    require(p.centerA + p.radiusA < p.centerB - p.radiusB, "Bodies overlap");
                    require(near(p.halfWidth / p.halfHeight, aspect), "Projection distorts spheres");
                    const auto swapped = sse::makeComparisonLayout(b, a, aspect, zoom);
                    require(near(p.centerA, -swapped.centerB), "Swap is not symmetric");
                    if (zoom <= 1) {
                        require(p.centerA - p.radiusA > -p.halfWidth &&
                                p.centerB + p.radiusB < p.halfWidth, "Fit clips a planet horizontally");
                        require(std::max(p.radiusA, p.radiusB) < p.halfHeight, "Fit clips a planet vertically");
                    }
                }
        require(near(sse::makeComparisonLayout(12742, 139820, 1).radiusB /
                     sse::makeComparisonLayout(12742, 139820, 1).radiusA, 139820.0 / 12742),
                "Earth/Jupiter ratio incorrect");
        for (double bad : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN()}) {
            for (int field = 0; field < 4; ++field) {
                double values[]{12742, 139820, 1.0, 1.0}; values[field] = bad;
                bool rejected = false;
                try { sse::makeComparisonLayout(values[0], values[1], values[2], values[3]); }
                catch (const std::invalid_argument&) { rejected = true; }
                require(rejected, "Invalid input was accepted");
            }
        }
        std::cout << "PASS: 960 pair/aspect/zoom cases and invalid dimensions\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
