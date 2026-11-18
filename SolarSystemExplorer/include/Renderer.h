#pragma once
#include "Camera.h"
#include "LabState.h"
#include "Mesh.h"
#include "Shader.h"
#include "SolarSystem.h"
#include "Texture.h"
#include <array>
#include <filesystem>
#include <vector>
namespace sse {
class Renderer {
  public:
    Renderer(const std::filesystem::path& root, const SolarSystem& system, bool forceFallback = false);
    void resize(int width, int height);
    void render(const SolarSystem& system, const Camera& camera, const RenderSettings& settings, int selected,
                float realTime, bool focusPresentation = false);
    void capture(const std::filesystem::path& file, int width, int height) const;
    int fallbackCount() const { return fallbackCount_; }

  private:
    void allocateTargets();
    void initializeAsteroids();
    void fullscreen() const;
    void atmosphere(const glm::mat4& model, const glm::mat4& vp, const glm::vec3& eye, glm::vec3 color,
                    float strength, bool sun, const Mesh* mesh = nullptr);
    void diagnostics(const SolarSystem& system, const glm::mat4& vp, const glm::vec3& eye,
                     const RenderSettings& settings, int selected);
    void cometTails(const SolarSystem& system, const glm::mat4& vp, float pixelScale);
    void cometPath(const glm::mat4& vp, const RenderSettings& settings);
    struct DiagnosticVertex {
        glm::vec3 position;
        glm::vec4 color;
    };
    struct TailVertex {
        glm::vec3 position;
        glm::vec4 color;
        float size;
    };
    Shader planet_, sun_, orbit_, ring_, atmosphere_, sky_, star_, blur_, composite_, debugSurface_,
        debugLines_, cometTail_, asteroid_;
    GLObject asteroidVao_{GLKind::VertexArray}, asteroidVertices_{GLKind::Buffer},
        asteroidIndices_{GLKind::Buffer}, asteroidInstances_{GLKind::Buffer};
    GLsizei asteroidIndexCount_ = 0;
    int quality_ = 1;
    Mesh sphere_, lowSphere_, annulus_, orbitMesh_, stars_;
    std::vector<Texture> surfaces_;
    Texture sunMap_, moonMap_, ringMap_, nightMap_, cloudMap_, skyMap_;
    GLObject emptyVao_{GLKind::VertexArray}, sceneFbo_{GLKind::Framebuffer}, sceneColor_{GLKind::Texture},
        depth_{GLKind::Renderbuffer};
    GLObject msaaFbo_{GLKind::Framebuffer}, msaaColor_{GLKind::Renderbuffer};
    GLObject diagnosticVao_{GLKind::VertexArray}, diagnosticVbo_{GLKind::Buffer};
    std::vector<DiagnosticVertex> diagnosticVertices_;
    GLObject cometPathVao_{GLKind::VertexArray}, cometPathVbo_{GLKind::Buffer};
    GLObject cometTailVao_{GLKind::VertexArray}, cometTailVbo_{GLKind::Buffer};
    std::vector<TailVertex> tailVertices_;
    GLsizei cometPathCount_ = 0, cometPolygonCount_ = 0, cometMarkerCount_ = 0;
    std::array<GLObject, 2> blurFbo_{GLObject(GLKind::Framebuffer), GLObject(GLKind::Framebuffer)};
    std::array<GLObject, 2> blurColor_{GLObject(GLKind::Texture), GLObject(GLKind::Texture)};
    int width_ = 0, height_ = 0, bloomWidth_ = 0, bloomHeight_ = 0, fallbackCount_ = 0;
    bool drawable_ = false;
};
} // namespace sse
