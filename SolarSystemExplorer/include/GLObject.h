#pragma once
#include <GL/glew.h>
#include <utility>
namespace sse {
enum class GLKind { Buffer, VertexArray, Texture, Framebuffer, Renderbuffer };
// Small move-only ownership primitive. Every handle is destroyed with the context alive.
class GLObject {
  public:
    explicit GLObject(GLKind kind) : kind_(kind) {
        switch (kind_) {
        case GLKind::Buffer:
            glGenBuffers(1, &id_);
            break;
        case GLKind::VertexArray:
            glGenVertexArrays(1, &id_);
            break;
        case GLKind::Texture:
            glGenTextures(1, &id_);
            break;
        case GLKind::Framebuffer:
            glGenFramebuffers(1, &id_);
            break;
        case GLKind::Renderbuffer:
            glGenRenderbuffers(1, &id_);
            break;
        }
    }
    ~GLObject() { release(); }
    GLObject(const GLObject&) = delete;
    GLObject& operator=(const GLObject&) = delete;
    GLObject(GLObject&& o) noexcept : kind_(o.kind_), id_(std::exchange(o.id_, 0)) {}
    GLObject& operator=(GLObject&& o) noexcept {
        if (this != &o) {
            release();
            kind_ = o.kind_;
            id_ = std::exchange(o.id_, 0);
        }
        return *this;
    }
    GLuint id() const { return id_; }

  private:
    void release() noexcept {
        if (!id_)
            return;
        switch (kind_) {
        case GLKind::Buffer:
            glDeleteBuffers(1, &id_);
            break;
        case GLKind::VertexArray:
            glDeleteVertexArrays(1, &id_);
            break;
        case GLKind::Texture:
            glDeleteTextures(1, &id_);
            break;
        case GLKind::Framebuffer:
            glDeleteFramebuffers(1, &id_);
            break;
        case GLKind::Renderbuffer:
            glDeleteRenderbuffers(1, &id_);
            break;
        }
    }
    GLKind kind_;
    GLuint id_ = 0;
};
} // namespace sse
