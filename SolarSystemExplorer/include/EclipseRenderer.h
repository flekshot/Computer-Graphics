#pragma once
#include "EclipseModel.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include <filesystem>

namespace sse {
struct EclipseViewport { int x, y, width, height; };

class EclipseRenderer {
  public:
    explicit EclipseRenderer(const std::filesystem::path& root);
    // Region is in physical framebuffer pixels, with a bottom-left origin.
    // Clears the default framebuffer, then draws two stacked views inside region.
    // The caller can place titles in the 28-pixel upper margin of each view.
    void render(const EclipseSettings& settings, const RenderSettings& graphics,
                const EclipseViewport& region, int framebufferWidth, int framebufferHeight);

  private:
    void drawBody(const EclipseBody& body, const Texture& texture, const EclipseScene& scene,
                  const glm::mat4& vp, const glm::vec3& eye, const RenderSettings& graphics,
                  bool source, const EclipseBody* occluder);
    void drawGeometry(const EclipseScene& scene, const EclipseSettings& settings,
                      const glm::mat4& vp);
    Shader surface_, lines_;
    Mesh sphere_, orbit_, cone_, arrow_;
    Texture sun_, earth_, moon_;
};
} // namespace sse
