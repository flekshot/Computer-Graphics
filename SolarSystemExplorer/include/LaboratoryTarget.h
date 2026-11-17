#pragma once
#include "GLObject.h"
#include "Shader.h"
#include <filesystem>

namespace sse {
// Shared linear-light, multisampled scene target. The renderer's GL state guard
// surrounds begin()/present(); the final image is written to framebuffer zero.
class LaboratoryTarget {
  public:
    explicit LaboratoryTarget(const std::filesystem::path& root);
    void begin(int width, int height);
    void present(float exposure, bool normalColors);

  private:
    Shader composite_;
    GLObject multisampleFbo_{GLKind::Framebuffer}, color_{GLKind::Renderbuffer}, depth_{GLKind::Renderbuffer};
    GLObject resolvedFbo_{GLKind::Framebuffer}, resolvedColor_{GLKind::Texture}, fullscreenVao_{GLKind::VertexArray};
    int width_ = 0, height_ = 0;
};
} // namespace sse
