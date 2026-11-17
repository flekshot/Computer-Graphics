#include "Bezier.h"
#include "Comet.h"
#include "SolarSystem.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
int checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
bool near(const glm::vec3& a, const glm::vec3& b, float eps = 1e-4f) {
    return glm::length(a - b) < eps;
}
} // namespace

int main() {
    try {
        const std::array<glm::vec3, 4> cubic{glm::vec3(0, 0, 0), glm::vec3(1, 2, 0), glm::vec3(3, 2, 1),
                                             glm::vec3(4, 0, 1)};
        check(near(sse::deCasteljau(cubic.data(), 4, 0.0f), cubic[0]), "Bezier must start at the first point");
        check(near(sse::deCasteljau(cubic.data(), 4, 1.0f), cubic[3]), "Bezier must end at the last point");
        for (float t = 0; t <= 1.0f; t += 0.05f)
            check(near(sse::deCasteljau(cubic.data(), 4, t), sse::cubicBernstein(cubic, t)),
                  "De Casteljau must agree with the Bernstein polynomial form");
        // Degree 1 is plain linear interpolation; degree 0 is a constant.
        check(near(sse::deCasteljau(cubic.data(), 2, 0.25f), glm::vec3(0.25f, 0.5f, 0)),
              "Two control points must produce a straight segment");
        check(near(sse::deCasteljau(cubic.data(), 1, 0.7f), cubic[0]), "One control point is a constant curve");
        {
            // Degree 23: all control points on a line x = i must evaluate to x = 23 t.
            std::vector<glm::vec3> line;
            for (int i = 0; i < 24; ++i)
                line.emplace_back(static_cast<float>(i), 0.0f, 0.0f);
            check(near(sse::deCasteljau(line, 0.25f), glm::vec3(5.75f, 0, 0), 1e-3f),
                  "De Casteljau must evaluate curves with more than 16 control points");
        }
        bool rejected = false;
        try {
            sse::deCasteljau(nullptr, 0, 0.5f);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "An empty control polygon must be rejected");

        const sse::Comet comet;
        const auto& path = comet.path();
        check(path.segmentCount() == 4, "The comet path is built from four cubic segments");
        check(path.controlPolygon().size() == 12, "A closed loop of n cubics has 3n control points");
        check(near(path.point(0), path.point(static_cast<double>(path.segmentCount()))),
              "The Bezier loop must be closed");
        for (std::size_t joint = 0; joint < path.segmentCount(); ++joint) {
            const double u = static_cast<double>(joint);
            check(near(path.point(u - 1e-6), path.point(u + 1e-6), 1e-3f), "Bezier loop must be C0 at every joint");
            const auto before = path.derivative(u - 1e-7), after = path.derivative(u + 1e-7);
            check(glm::length(before - after) < 1e-3f * glm::length(after), "Bezier loop must be C1 at every joint");
        }
        const auto samples = path.sample(16);
        check(samples.size() == 65 && near(samples.front(), samples.back()), "Sampled loop must be closed");

        check(comet.perihelion() > sse::SolarSystem::sunRadius * 1.3f,
              "The comet must never graze the Sun");
        check(comet.perihelion() < 7.0f && comet.aphelion() > 38.0f,
              "The comet must cross from inside Mercury's orbit to beyond Saturn's");
        check(near(comet.position(0), comet.position(sse::Comet::periodDays), 1e-3f),
              "The comet must return to its start after one period");
        check(near(comet.position(-100), comet.position(sse::Comet::periodDays - 100), 1e-3f),
              "Negative days must wrap onto the same orbit");
        double previous = comet.parameterAt(0);
        for (int day = 1; day < 1200; ++day) {
            const double u = comet.parameterAt(day);
            check(u > previous, "Comet motion along the loop must be monotonic");
            previous = u;
        }
        // Find perihelion and aphelion passages and compare their speeds (Kepler's second law).
        float fastest = 0, slowest = 1e9f, nearSpeed = 0, farSpeed = 0, nearR = 1e9f, farR = 0;
        for (int day = 0; day < 1200; day += 2) {
            const float r = glm::length(comet.position(day));
            const float v = glm::length(comet.velocity(day));
            check(std::isfinite(v) && v > 0, "Comet velocity must be finite and nonzero");
            fastest = std::max(fastest, v);
            slowest = std::min(slowest, v);
            if (r < nearR) {
                nearR = r;
                nearSpeed = v;
            }
            if (r > farR) {
                farR = r;
                farSpeed = v;
            }
        }
        check(nearSpeed > 4.0f * farSpeed, "The comet must move much faster at perihelion than at aphelion");
        check(fastest / slowest < 50.0f, "Comet speed must stay within a plausible range");

        sse::SolarSystem system;
        system.setDays(250);
        check(near(system.cometPosition(), comet.position(250)), "The solar system must expose the comet state");
        std::cout << "PASS: " << checks << " Bezier and comet assertions\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << " assertions: " << e.what() << '\n';
        return 1;
    }
}
