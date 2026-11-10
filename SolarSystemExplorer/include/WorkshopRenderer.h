#pragma once
#include "Camera.h"
#include "LabState.h"
#include "Mesh.h"
#include "Shader.h"
#include <filesystem>
#include <memory>

namespace sse {
// Procedural preview uses its own camera and never changes the orbital model.
class WorkshopRenderer {
  public:
    explicit WorkshopRenderer(const std::filesystem::path& root);
    void render(const WorkshopSettings& settings, const RenderSettings& graphics,
                const Camera& camera, int width, int height);

  private:
    void updateGeometry(const WorkshopSettings& settings);
    Shader surface_;
    Mesh sphere_;
    GLObject backgroundVao_{GLKind::VertexArray};
    std::unique_ptr<Mesh> planet_;
    WorkshopSettings geometrySettings_;
    float outerRadius_ = 1.25f;
};
} // namespace sse
