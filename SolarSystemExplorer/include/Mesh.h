#pragma once
#include "GLObject.h"
#include "Geometry.h"
namespace sse {
class Mesh {
  public:
    explicit Mesh(const Geometry& geometry, GLenum primitive = GL_TRIANGLES);
    void draw() const;

  private:
    GLObject vao_{GLKind::VertexArray}, vbo_{GLKind::Buffer}, ebo_{GLKind::Buffer};
    GLsizei count_ = 0;
    GLenum primitive_;
};
} // namespace sse
