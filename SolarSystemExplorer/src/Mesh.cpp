#include "Mesh.h"
#include <cstddef>
namespace sse {
Mesh::Mesh(const Geometry& geometry, GLenum primitive)
    : count_(static_cast<GLsizei>(geometry.indices.size())), primitive_(primitive) {
    glBindVertexArray(vao_.id());
    glBindBuffer(GL_ARRAY_BUFFER, vbo_.id());
    glBufferData(GL_ARRAY_BUFFER, geometry.vertices.size() * sizeof(Vertex), geometry.vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_.id());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, geometry.indices.size() * sizeof(uint32_t), geometry.indices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glBindVertexArray(0);
}
void Mesh::draw() const {
    glBindVertexArray(vao_.id());
    glDrawElements(primitive_, count_, GL_UNSIGNED_INT, nullptr);
}
} // namespace sse
