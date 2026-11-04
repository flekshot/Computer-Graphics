#include "Geometry.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <random>
#include <stdexcept>
namespace sse {
Geometry makeSphere(unsigned segments, unsigned rings) {
    if (segments < 3 || rings < 2 || segments > 1024 || rings > 512)
        throw std::invalid_argument("Sphere tessellation outside supported limits");
    Geometry g;
    for (unsigned y = 0; y <= rings; ++y)
        for (unsigned x = 0; x <= segments; ++x) {
            float u = float(x) / segments, v = float(y) / rings;
            float theta = v * glm::pi<float>(), phi = u * glm::two_pi<float>();
            glm::vec3 p(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
            g.vertices.push_back({p, p, {u, v}});
        }
    for (unsigned y = 0; y < rings; ++y)
        for (unsigned x = 0; x < segments; ++x) {
            uint32_t a = y * (segments + 1) + x, b = a + segments + 1;
            if (y > 0)
                g.indices.insert(g.indices.end(), {a, a + 1, b});
            if (y + 1 < rings)
                g.indices.insert(g.indices.end(), {a + 1, b + 1, b});
        }
    return g;
}
Geometry makeAnnulus(float innerRadius, float outerRadius, unsigned segments) {
    if (segments < 3 || innerRadius <= 0 || outerRadius <= innerRadius)
        throw std::invalid_argument("Invalid annulus");
    Geometry g;
    for (unsigned i = 0; i <= segments; ++i) {
        const float a = float(i) / segments * glm::two_pi<float>();
        for (int j = 0; j < 2; ++j) {
            float r = j ? outerRadius : innerRadius;
            g.vertices.push_back(
                {{r * std::cos(a), 0, r * std::sin(a)}, {0, 1, 0}, {float(j), float(i) / segments}});
        }
    }
    for (unsigned i = 0; i < segments; ++i) {
        unsigned a = i * 2;
        g.indices.insert(g.indices.end(), {a, a + 2, a + 1, a + 1, a + 2, a + 3});
    }
    return g;
}
Geometry makeOrbit(unsigned segments) {
    if (segments < 3)
        throw std::invalid_argument("Invalid orbit tessellation");
    Geometry g;
    for (unsigned i = 0; i < segments; ++i) {
        float a = float(i) / segments * glm::two_pi<float>();
        g.vertices.push_back({{std::cos(a), 0, std::sin(a)}, {0, 1, 0}, {0, 0}});
        g.indices.push_back(i);
    }
    return g;
}
Geometry makeStars(unsigned count) {
    Geometry g;
    std::mt19937 random(4751);
    std::uniform_real_distribution<float> unit(0, 1);
    for (unsigned i = 0; i < count; ++i) {
        float y = unit(random) * 2 - 1, angle = unit(random) * glm::two_pi<float>(), r = std::sqrt(1 - y * y);
        glm::vec3 color = glm::mix(glm::vec3(0.55f, 0.7f, 1), glm::vec3(1, 0.79f, 0.48f), unit(random));
        g.vertices.push_back({glm::vec3(r * std::cos(angle), y, r * std::sin(angle)) * 850.0f,
                              color,
                              {1 + std::pow(unit(random), 6.0f) * 3.8f, unit(random) * 6.28f}});
        g.indices.push_back(i);
    }
    return g;
}
} // namespace sse
