#pragma once
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "SolarSystem.h"
#include "Texture.h"
#include <array>
#include <filesystem>
#include <vector>
namespace sse {
struct RenderSettings {
    bool orbits = true, labels = true, bloom = true;
    float exposure = 1.05f;
};
class Renderer {
  public:
    Renderer(const std::filesystem::path& root, const SolarSystem& system, bool forceFallback = false);
    void resize(int width, int height);
    void render(const SolarSystem& system, const Camera& camera, const RenderSettings& settings, int selected,
                float realTime);
    void capture(const std::filesystem::path& file, int width, int height) const;
    int fallbackCount() const { return fallbackCount_; }

  private:
    void fullscreen() const;
    void atmosphere(const glm::mat4& model, const glm::mat4& vp, const glm::vec3& eye, glm::vec3 color,
                    float strength, bool sun);
    Shader planet_, sun_, orbit_, ring_, atmosphere_, sky_, star_, blur_, composite_;
    Mesh sphere_, annulus_, orbitMesh_, stars_;
    std::vector<Texture> surfaces_;
    Texture sunMap_, moonMap_, ringMap_, nightMap_, cloudMap_, skyMap_;
    GLObject emptyVao_{GLKind::VertexArray}, sceneFbo_{GLKind::Framebuffer}, sceneColor_{GLKind::Texture},
        depth_{GLKind::Renderbuffer};
    GLObject msaaFbo_{GLKind::Framebuffer}, msaaColor_{GLKind::Renderbuffer};
    std::array<GLObject, 2> blurFbo_{GLObject(GLKind::Framebuffer), GLObject(GLKind::Framebuffer)};
    std::array<GLObject, 2> blurColor_{GLObject(GLKind::Texture), GLObject(GLKind::Texture)};
    int width_ = 0, height_ = 0, bloomWidth_ = 0, bloomHeight_ = 0, fallbackCount_ = 0;
};
} // namespace sse
