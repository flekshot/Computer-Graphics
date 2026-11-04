#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
namespace sse {
struct Vertex {
    glm::vec3 position, normal;
    glm::vec2 uv;
};
struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
Geometry makeSphere(unsigned segments = 128, unsigned rings = 64);
Geometry makeAnnulus(float innerRadius, float outerRadius, unsigned segments = 256);
Geometry makeOrbit(unsigned segments = 256);
Geometry makeStars(unsigned count = 4500);
} // namespace sse
