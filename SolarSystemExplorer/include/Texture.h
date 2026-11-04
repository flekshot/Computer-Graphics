#pragma once
#include "GLObject.h"
#include <glm/glm.hpp>
#include <filesystem>
#include <string>
namespace sse {
class Texture {
  public:
    Texture(const std::filesystem::path& path, const std::string& kind, glm::vec3 color = {0.6f, 0.6f, 0.6f},
            bool srgb = true);
    void bind(unsigned slot = 0) const;
    GLuint id() const { return texture_.id(); }
    bool fallback() const { return fallback_; }

  private:
    GLObject texture_{GLKind::Texture};
    bool fallback_ = false;
};
} // namespace sse
