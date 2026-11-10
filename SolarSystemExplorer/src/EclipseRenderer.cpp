#include "EclipseRenderer.h"
#include "VisualMath.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace sse {
namespace {
constexpr float pi = 3.14159265358979323846f;

// Renderers can be switched while ImGui/capture uses different state. Save the
// complete state that this renderer changes, including independent read/draw FBOs.
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
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer_);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture_);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0_);
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
        // OpenGL core accepts GL_FRONT_AND_BACK; its two queried values match.
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
        glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(renderbuffer_));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture0_));
        glActiveTexture(static_cast<GLenum>(activeTexture_));
    }
  private:
    static void enable(GLenum setting, GLboolean enabled) { if (enabled) glEnable(setting); else glDisable(setting); }
    GLint viewport_[4]{}, scissorBox_[4]{}, polygon_[2]{};
    GLint drawFbo_{}, readFbo_{}, depthFunc_{}, blendSrcRGB_{}, blendDstRGB_{}, blendSrcAlpha_{}, blendDstAlpha_{};
    GLint blendEquationRGB_{}, blendEquationAlpha_{}, program_{}, vao_{}, renderbuffer_{}, activeTexture_{}, texture0_{};
    GLboolean depthMask_{}, colorMask_[4]{}, depth_{}, blend_{}, scissor_{}, cull_{}, srgb_{}, multisample_{};
    GLfloat clearColor_[4]{}, lineWidth_{};
    GLdouble clearDepth_{};
};

Geometry coneOutline() {
    Geometry geometry;
    constexpr unsigned segments = 64;
    for (unsigned ring = 0; ring < 2; ++ring)
        for (unsigned i = 0; i < segments; ++i) {
            const float angle = float(i) * 2.0f * pi / segments;
            geometry.vertices.push_back({{float(ring), std::cos(angle), std::sin(angle)}, {0, 1, 0}, {0, 0}});
            geometry.indices.push_back(ring * segments + i);
            geometry.indices.push_back(ring * segments + (i + 1) % segments);
            if (ring == 0 && i % 8 == 0) {
                geometry.indices.push_back(i);
                geometry.indices.push_back(segments + i);
            }
        }
    return geometry;
}
Geometry lightArrow() {
    Geometry geometry;
    geometry.vertices = {{{0, 0, 0}, {0, 1, 0}, {0, 0}}, {{1, 0, 0}, {0, 1, 0}, {0, 0}},
                         {{.92f, .32f, 0}, {0, 1, 0}, {0, 0}}, {{.92f, -.32f, 0}, {0, 1, 0}, {0, 0}}};
    geometry.indices = {0, 1, 1, 2, 1, 3};
    return geometry;
}
glm::mat4 bodyMatrix(const EclipseBody& body) {
    return glm::translate(glm::mat4(1), glm::vec3(body.center)) *
           glm::rotate(glm::mat4(1), -0.9f, glm::vec3(0, 1, 0)) *
           glm::scale(glm::mat4(1), glm::vec3(float(body.radius)));
}
void setViewport(const EclipseViewport& view, glm::vec3 background, bool normalColors) {
    glViewport(view.x, view.y, view.width, view.height);
    glScissor(view.x, view.y, view.width, view.height);
    if (!normalColors) background = glm::pow(background, glm::vec3(2.2f));
    glClearColor(background.x, background.y, background.z, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
EclipseViewport contentViewport(EclipseViewport view) {
    view.height = std::max(1, view.height - std::min(28, view.height / 5));
    return view;
}
} // namespace

EclipseRenderer::EclipseRenderer(const std::filesystem::path& root)
    : target_(root), surface_(root / "shaders/eclipse.vert", root / "shaders/eclipse.frag"),
      lines_(root / "shaders/eclipse_line.vert", root / "shaders/eclipse_line.frag"),
      sphere_(makeSphere()), orbit_(makeOrbit(), GL_LINE_LOOP), cone_(coneOutline(), GL_LINES),
      arrow_(lightArrow(), GL_LINES), sun_(root / "assets/textures/sun.jpg", "Sun"),
      earth_(root / "assets/textures/earth_daymap.jpg", "Earth"),
      moon_(root / "assets/textures/moon.jpg", "Moon") {}

void EclipseRenderer::drawBody(const EclipseBody& body, const Texture& texture,
                               const EclipseScene& scene, const glm::mat4& vp, const glm::vec3& eye,
                               const RenderSettings& graphics, bool source, const EclipseBody* occluder) {
    surface_.use();
    surface_.set("uVP", vp);
    surface_.set("uModel", bodyMatrix(body));
    surface_.set("uEye", eye);
    surface_.set("uSurface", 0);
    surface_.set("uSunCenter", glm::vec3(scene.sun.center));
    surface_.set("uSunRadius", float(scene.sun.radius));
    surface_.set("uOccluderCenter", glm::vec3(occluder ? occluder->center : scene.sun.center));
    surface_.set("uOccluderRadius", occluder ? float(occluder->radius) : 0.0f);
    surface_.set("uSource", int(source));
    surface_.set("uShadowed", int(occluder != nullptr));
    surface_.set("uSurfaceMode", int(graphics.surfaceMode));
    surface_.set("uAmbient", graphics.ambient);
    surface_.set("uDiffuse", graphics.diffuse);
    surface_.set("uSpecular", graphics.specular);
    texture.bind();
    sphere_.draw();
}

void EclipseRenderer::drawGeometry(const EclipseScene& scene, const EclipseSettings& settings,
                                   const glm::mat4& vp) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    lines_.use();
    lines_.set("uVP", vp);
    lines_.set("uCone", 0);
    lines_.set("uColor", glm::vec3(.24f, .43f, .57f));
    lines_.set("uAlpha", .65f);
    lines_.set("uModel", glm::translate(glm::mat4(1), glm::vec3(scene.earth.center)) *
                              glm::rotate(glm::mat4(1), glm::radians(settings.inclinationDegrees), glm::vec3(0, 0, 1)) *
                              glm::scale(glm::mat4(1), glm::vec3(5)));
    orbit_.draw();
    lines_.set("uColor", glm::vec3(1, .77f, .36f));
    lines_.set("uAlpha", .85f);
    lines_.set("uModel", glm::translate(glm::mat4(1), glm::vec3(-11, -3.4f, 0)) *
                              glm::scale(glm::mat4(1), glm::vec3(12, 1, 1)));
    arrow_.draw();
    if (settings.showCones) {
        const auto& blocker = scene.occluder();
        const glm::vec3 axis = glm::normalize(glm::vec3(blocker.center - scene.sun.center));
        const glm::vec3 u = glm::normalize(glm::cross(axis, glm::vec3(0, 1, 0)));
        const glm::vec3 v = glm::cross(axis, u);
        const float distance = float(glm::length(blocker.center - scene.sun.center));
        const float radius = float(blocker.radius);
        const float end = float(glm::length(scene.target().center - blocker.center) + scene.target().radius + .8);
        lines_.set("uCone", 1);
        for (bool penumbra : {true, false}) {
            // Tangent cones of two finite spheres. The umbra ends at its apex;
            // beyond it an antumbra would start (outside these scene presets).
            const float sine = (float(scene.sun.radius) + (penumbra ? radius : -radius)) / distance;
            const float cosine = std::sqrt(1.0f - sine * sine);
            const float slope = sine / cosine;
            const float start = (penumbra ? -1.0f : 1.0f) * radius * sine;
            const float far = penumbra ? end : std::min(end, radius / sine);
            const float firstRadius = radius * cosine;
            const float lastRadius = std::max(0.0f, firstRadius + (penumbra ? 1.0f : -1.0f) * (far - start) * slope);
            glm::mat4 model(1);
            model[0] = glm::vec4(axis * (far - start), 0);
            model[1] = glm::vec4(u, 0);
            model[2] = glm::vec4(v, 0);
            model[3] = glm::vec4(glm::vec3(blocker.center) + axis * start, 1);
            lines_.set("uModel", model);
            lines_.set("uRadius0", firstRadius);
            lines_.set("uRadius1", lastRadius);
            lines_.set("uColor", penumbra ? glm::vec3(1, .65f, .23f) : glm::vec3(.72f, .44f, 1));
            lines_.set("uAlpha", penumbra ? .48f : .88f);
            cone_.draw();
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void EclipseRenderer::render(const EclipseSettings& settings, const RenderSettings& graphics,
                             const EclipseViewport& region, int framebufferWidth, int framebufferHeight) {
    if (framebufferWidth <= 0 || framebufferHeight <= 0)
        return;
    const GLState restore;
    target_.begin(framebufferWidth, framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearDepth(1.0);
    const bool normalColors = graphics.surfaceMode == SurfaceMode::Normals;
    const glm::vec3 background = normalColors ? glm::vec3(.012f, .021f, .035f)
        : glm::pow(glm::vec3(.012f, .021f, .035f), glm::vec3(2.2f));
    glClearColor(background.x, background.y, background.z, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(1);
    glPolygonMode(GL_FRONT_AND_BACK, graphics.surfaceMode == SurfaceMode::Wireframe ? GL_LINE : GL_FILL);

    const int left = std::clamp(region.x, 0, framebufferWidth);
    const int bottom = std::clamp(region.y, 0, framebufferHeight);
    const int right = std::clamp(region.x + std::max(0, region.width), left, framebufferWidth);
    const int top = std::clamp(region.y + std::max(0, region.height), bottom, framebufferHeight);
    if (right - left < 2 || top - bottom < 8) {
        target_.present(graphics.exposure, normalColors);
        return;
    }
    const int gap = 6, overviewHeight = (top - bottom - gap) * 38 / 100;
    const EclipseViewport overview{left, top - overviewHeight, right - left, overviewHeight};
    const EclipseViewport detail{left, bottom, right - left, top - bottom - overviewHeight - gap};
    const auto scene = makeEclipseScene(settings);
    glEnable(GL_SCISSOR_TEST);
    setViewport(overview, {.018f, .035f, .055f}, normalColors);
    const auto overviewContent = contentViewport(overview);
    glViewport(overviewContent.x, overviewContent.y, overviewContent.width, overviewContent.height);
    const float aspect = float(overviewContent.width) / std::max(1, overviewContent.height);
    const float halfWidth = std::max(17.5f, 4.9f * aspect), halfHeight = halfWidth / aspect;
    const glm::vec3 center(-3.5f, 0, 0), eye = center + glm::vec3(0, 16, 32);
    const auto vp = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, .1f, 100.0f) *
                    glm::lookAt(eye, center, glm::vec3(0, 1, 0));
    drawBody(scene.sun, sun_, scene, vp, eye, graphics, true, nullptr);
    drawBody(scene.earth, earth_, scene, vp, eye, graphics, false, &scene.moon);
    drawBody(scene.moon, moon_, scene, vp, eye, graphics, false, &scene.earth);
    drawGeometry(scene, settings, vp);

    setViewport(detail, {.012f, .025f, .045f}, normalColors);
    const auto detailContent = contentViewport(detail);
    glViewport(detailContent.x, detailContent.y, detailContent.width, detailContent.height);
    const float detailAspect = float(detailContent.width) / std::max(1, detailContent.height);
    const auto& target = scene.target();
    const glm::vec3 targetCenter(target.center);
    const glm::vec3 towardSun = glm::normalize(glm::vec3(scene.sun.center - target.center));
    if (settings.observerView) {
        const glm::vec3 observer = targetCenter + towardSun * float(target.radius * 1.002);
        const double sourceAngle = std::asin(scene.sun.radius / glm::length(scene.sun.center - glm::dvec3(observer)));
        const double blockerAngle = std::asin(scene.occluder().radius / glm::length(scene.occluder().center - glm::dvec3(observer)));
        const float verticalFov = std::clamp(float(std::max(sourceAngle, blockerAngle) * 3.4), .35f, 1.55f);
        const auto observerVP = glm::perspective(fitVerticalFov(verticalFov, detailAspect), detailAspect, .01f, 100.0f) *
                                glm::lookAt(observer, glm::vec3(scene.sun.center), glm::vec3(0, 1, 0));
        drawBody(scene.sun, sun_, scene, observerVP, observer, graphics, true, nullptr);
        drawBody(scene.occluder(), scene.lunar ? earth_ : moon_, scene, observerVP, observer, graphics, false, &target);
    } else {
        const glm::vec3 detailEye = targetCenter + towardSun * float(target.radius * 6.0) + glm::vec3(0, float(target.radius * .45), float(target.radius * .9));
        const float halfHeightDetail = float(target.radius * 1.3) / std::min(1.0f, detailAspect);
        const auto detailVP = glm::ortho(-halfHeightDetail * detailAspect, halfHeightDetail * detailAspect,
                                        -halfHeightDetail, halfHeightDetail, .01f, 100.0f) *
                              glm::lookAt(detailEye, targetCenter, glm::vec3(0, 1, 0));
        drawBody(target, scene.lunar ? moon_ : earth_, scene, detailVP, detailEye, graphics, false, &scene.occluder());
    }
    target_.present(graphics.exposure, normalColors);
}
} // namespace sse
