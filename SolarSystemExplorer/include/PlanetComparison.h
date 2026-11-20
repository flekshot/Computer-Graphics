#pragma once
#include "GLObject.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "SolarSystem.h"
#include <filesystem>
#include <memory>
#include <vector>

namespace sse {
// A self-contained ImGui window with an offscreen OpenGL 3.3 scene.
// Construct/destroy while the application's GL context is current.
class PlanetComparison {
  public:
    explicit PlanetComparison(const std::filesystem::path& root);
    void draw(const SolarSystem& system, bool& open);
  private:
    void render(const SolarSystem& system, int width, int height);
    void resize(int width, int height);
    std::filesystem::path root_;
    Shader shader_;
    Mesh sphere_;
    GLObject framebuffer_{GLKind::Framebuffer};
    GLObject color_{GLKind::Texture};
    GLObject depth_{GLKind::Renderbuffer};
    std::vector<std::unique_ptr<Texture>> textures_;
    int width_ = 0, height_ = 0;
    int first_ = 2, second_ = 4;
    float zoom_ = 1, yaw_ = -25, pitch_ = 0;
    bool rotate_ = true, wireframe_ = false;
};
} // namespace sse
