#include "WorkshopRenderer.h"
#include "WorkshopModel.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace sse {
namespace {
// Preview rendering also runs between other renderers and test captures. Keep
// independent read/draw targets and every state changed by the preview intact.
class GLState {
  public:
    GLState() {
        glGetIntegerv(GL_VIEWPORT, viewport_);
        glGetIntegerv(GL_SCISSOR_BOX, scissorBox_);
        glGetIntegerv(GL_POLYGON_MODE, polygon_);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo_);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo_);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc_);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask_);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask_);
        glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRGB_);
        glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRGB_);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha_);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha_);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &blendEquationRGB_);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &blendEquationAlpha_);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao_);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer_);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer_);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture_);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0_);
        glActiveTexture(static_cast<GLenum>(activeTexture_));
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor_);
        glGetDoublev(GL_DEPTH_CLEAR_VALUE, &clearDepth_);
        glGetFloatv(GL_LINE_WIDTH, &lineWidth_);
        depth_ = glIsEnabled(GL_DEPTH_TEST);
        blend_ = glIsEnabled(GL_BLEND);
        scissor_ = glIsEnabled(GL_SCISSOR_TEST);
        cull_ = glIsEnabled(GL_CULL_FACE);
        srgb_ = glIsEnabled(GL_FRAMEBUFFER_SRGB);
        multisample_ = glIsEnabled(GL_MULTISAMPLE);
    }
    ~GLState() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFbo_));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFbo_));
        glViewport(viewport_[0], viewport_[1], viewport_[2], viewport_[3]);
        glScissor(scissorBox_[0], scissorBox_[1], scissorBox_[2], scissorBox_[3]);
        glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygon_[0]));
        glDepthFunc(static_cast<GLenum>(depthFunc_));
        glDepthMask(depthMask_);
        glColorMask(colorMask_[0], colorMask_[1], colorMask_[2], colorMask_[3]);
        glBlendFuncSeparate(static_cast<GLenum>(blendSrcRGB_), static_cast<GLenum>(blendDstRGB_),
                            static_cast<GLenum>(blendSrcAlpha_), static_cast<GLenum>(blendDstAlpha_));
        glBlendEquationSeparate(static_cast<GLenum>(blendEquationRGB_), static_cast<GLenum>(blendEquationAlpha_));
        enable(GL_DEPTH_TEST, depth_); enable(GL_BLEND, blend_); enable(GL_SCISSOR_TEST, scissor_);
        enable(GL_CULL_FACE, cull_); enable(GL_FRAMEBUFFER_SRGB, srgb_);
        enable(GL_MULTISAMPLE, multisample_);
        glClearColor(clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]);
        glClearDepth(clearDepth_);
        glLineWidth(lineWidth_);
        glUseProgram(static_cast<GLuint>(program_));
        glBindVertexArray(static_cast<GLuint>(vao_));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(arrayBuffer_));
        glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(renderbuffer_));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture0_));
        glActiveTexture(static_cast<GLenum>(activeTexture_));
    }
  private:
    static void enable(GLenum setting, GLboolean enabled) { if (enabled) glEnable(setting); else glDisable(setting); }
    GLint viewport_[4]{}, scissorBox_[4]{}, polygon_[2]{};
    GLint drawFbo_{}, readFbo_{}, depthFunc_{}, blendSrcRGB_{}, blendDstRGB_{}, blendSrcAlpha_{}, blendDstAlpha_{};
    GLint blendEquationRGB_{}, blendEquationAlpha_{}, program_{}, vao_{}, arrayBuffer_{};
    GLint renderbuffer_{}, activeTexture_{}, texture0_{};
    GLboolean depthMask_{}, colorMask_[4]{}, depth_{}, blend_{}, scissor_{}, cull_{}, srgb_{}, multisample_{};
    GLfloat clearColor_[4]{}, lineWidth_{};
    GLdouble clearDepth_{};
};

bool sameGeometry(const WorkshopSettings& a, const WorkshopSettings& b) {
    if (a.kind != b.kind || a.radius != b.radius)
        return false;
    if (a.kind == WorkshopKind::Gas)
        return true;
    return a.seed == b.seed && a.relief == b.relief && a.featureScale == b.featureScale && a.seaLevel == b.seaLevel;
}
glm::vec3 color(const std::array<float, 3>& rgb) { return {rgb[0], rgb[1], rgb[2]}; }
} // namespace

WorkshopRenderer::WorkshopRenderer(const std::filesystem::path& root)
    : target_(root), surface_(root / "shaders/workshop.vert", root / "shaders/workshop.frag"), sphere_(makeSphere()) {}

void WorkshopRenderer::updateGeometry(const WorkshopSettings& settings) {
    if (planet_ && sameGeometry(settings, geometrySettings_))
        return;
    auto geometry = makeWorkshopPlanet(settings);
    outerRadius_ = settings.radius;
    for (auto& vertex : geometry.vertices) {
        const float radius = glm::length(vertex.position);
        outerRadius_ = std::max(outerRadius_, radius);
        // This dedicated mesh needs no texture coordinates. Interpolating the
        // actual CPU height keeps shorelines and displaced land in agreement.
        vertex.uv.x = settings.kind == WorkshopKind::Rocky
                          ? sampleWorkshopTerrain(glm::normalize(vertex.position), settings) : .5f;
    }
    planet_ = std::make_unique<Mesh>(geometry);
    geometrySettings_ = settings;
}

void WorkshopRenderer::render(const WorkshopSettings& settings, const RenderSettings& graphics,
                              const Camera& camera, int width, int height) {
    if (width <= 0 || height <= 0)
        return;
    const GLState restore;
    updateGeometry(settings);
    target_.begin(width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearDepth(1.0);
    glClearColor(.015f, .025f, .045f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendEquation(GL_FUNC_ADD);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1);

    surface_.use();
    surface_.set("uLayer", 3);
    surface_.set("uSurfaceMode", int(graphics.surfaceMode));
    surface_.set("uViewport", glm::vec3(float(width), float(height), 0));
    glBindVertexArray(backgroundVao_.id());
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_DEPTH_TEST);
    surface_.set("uVP", camera.projection(float(width) / float(height)) * camera.view());
    surface_.set("uEye", camera.position());
    surface_.set("uKind", int(settings.kind));
    surface_.set("uSeed", float(settings.seed));
    surface_.set("uScale", settings.featureScale);
    surface_.set("uSeaLevel", settings.seaLevel);
    surface_.set("uIceCap", settings.iceCap);
    surface_.set("uClouds", settings.clouds);
    surface_.set("uAtmosphere", settings.atmosphere);
    surface_.set("uGasBands", float(settings.gasBands));
    surface_.set("uTurbulence", settings.gasTurbulence);
    surface_.set("uLandColor", color(settings.landColor));
    surface_.set("uWaterColor", color(settings.waterColor));
    surface_.set("uAtmosphereColor", color(settings.atmosphereColor));
    surface_.set("uGasColorA", color(settings.gasColorA));
    surface_.set("uGasColorB", color(settings.gasColorB));
    surface_.set("uAmbient", graphics.ambient);
    surface_.set("uDiffuse", graphics.diffuse);
    surface_.set("uSpecular", graphics.specular);
    const auto rotation = glm::rotate(glm::mat4(1), glm::radians(settings.rotationDegrees), glm::vec3(0, 1, 0));
    surface_.set("uModel", rotation);
    surface_.set("uLayer", 0);
    glPolygonMode(GL_FRONT_AND_BACK, graphics.surfaceMode == SurfaceMode::Wireframe ? GL_LINE : GL_FILL);
    planet_->draw();

    // Diagnostic views expose only the generated surface; shells would obscure
    // its topology and normals. Both effects are illustrative, not a simulation.
    if (graphics.surfaceMode == SurfaceMode::Shaded) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        if (settings.kind == WorkshopKind::Rocky && settings.clouds > 0) {
            surface_.set("uLayer", 1);
            surface_.set("uModel", rotation * glm::scale(glm::mat4(1), glm::vec3(outerRadius_ + settings.radius * .012f)));
            sphere_.draw();
        }
        if (settings.atmosphere > 0) {
            surface_.set("uLayer", 2);
            surface_.set("uModel", rotation * glm::scale(glm::mat4(1),
                glm::vec3(outerRadius_ + settings.radius * (.025f + .065f * settings.atmosphere))));
            sphere_.draw();
        }
    }
    target_.present(graphics.exposure, graphics.surfaceMode == SurfaceMode::Normals);
}
} // namespace sse
