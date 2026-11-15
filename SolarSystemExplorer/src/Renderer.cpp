#include "Renderer.h"
#include "VisualMath.h"
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_WINDOWS_UTF8
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
#include <stb_image_write.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
namespace sse {
namespace {
constexpr size_t diagnosticCapacity = 2048;
constexpr int ionParticles = 80, dustParticles = 110;
constexpr size_t cometTailCapacity = ionParticles + dustParticles;
// The scene temporarily changes rasterization for its diagnostic views. Fullscreen
// passes always use filled triangles, and callers keep their original raster state.
class RasterState {
  public:
    RasterState() : scissor_(glIsEnabled(GL_SCISSOR_TEST)) {
        glGetIntegerv(GL_POLYGON_MODE, polygon_);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_SCISSOR_TEST);
    }
    ~RasterState() {
        glPolygonMode(GL_FRONT_AND_BACK, polygon_[0]);
        if (scissor_)
            glEnable(GL_SCISSOR_TEST);
    }

  private:
    GLint polygon_[2]{};
    GLboolean scissor_;
};
void allocateColor(GLuint id, int w, int h) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}
void checkFramebuffer() {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("HDR framebuffer is incomplete. Check GPU memory and OpenGL support.");
}
} // namespace
Renderer::Renderer(const std::filesystem::path& root, const SolarSystem& system, bool forceFallback)
    : planet_(root / "shaders/planet.vert", root / "shaders/planet.frag"),
      sun_(root / "shaders/planet.vert", root / "shaders/sun.frag"),
      orbit_(root / "shaders/orbit.vert", root / "shaders/orbit.frag"),
      ring_(root / "shaders/planet.vert", root / "shaders/ring.frag"),
      atmosphere_(root / "shaders/planet.vert", root / "shaders/atmosphere.frag"),
      sky_(root / "shaders/fullscreen.vert", root / "shaders/sky.frag"),
      star_(root / "shaders/star.vert", root / "shaders/star.frag"),
      blur_(root / "shaders/fullscreen.vert", root / "shaders/blur.frag"),
      composite_(root / "shaders/fullscreen.vert", root / "shaders/composite.frag"),
      debugSurface_(root / "shaders/planet.vert", root / "shaders/debug_surface.frag"),
      debugLines_(root / "shaders/debug_lines.vert", root / "shaders/debug_lines.frag"),
      cometTail_(root / "shaders/comet_tail.vert", root / "shaders/comet_tail.frag"), sphere_(makeSphere()),
      lowSphere_(makeSphere(16, 8)),
      annulus_(makeAnnulus(1.25f, 2.3f)), orbitMesh_(makeOrbit(), GL_LINE_LOOP),
      stars_(makeStars(), GL_POINTS),
      sunMap_(root / (forceFallback ? "missing/sun" : "assets/textures/sun.jpg"), "Sun"),
      moonMap_(root / (forceFallback ? "missing/moon" : "assets/textures/moon.jpg"), "Moon"),
      ringMap_(root / (forceFallback ? "missing/ring" : "assets/textures/saturn_ring_alpha.png"), "Ring"),
      nightMap_(root / (forceFallback ? "missing/night" : "assets/textures/earth_nightmap.jpg"), "Night"),
      cloudMap_(root / (forceFallback ? "missing/clouds" : "assets/textures/earth_clouds.jpg"), "Clouds",
                glm::vec3(1), false),
      skyMap_(root / (forceFallback ? "missing/sky" : "assets/textures/stars_milky_way.jpg"), "Sky") {
    for (const auto& p : system.planets())
        surfaces_.emplace_back(root / (forceFallback ? "missing" : "assets/textures") / p.data().texture,
                               p.data().name, p.data().color);
    for (const auto& t : surfaces_)
        if (t.fallback())
            ++fallbackCount_;
    for (const Texture* t : {&sunMap_, &moonMap_, &ringMap_, &nightMap_, &cloudMap_, &skyMap_})
        if (t->fallback())
            ++fallbackCount_;
    diagnosticVertices_.reserve(diagnosticCapacity);
    glBindVertexArray(diagnosticVao_.id());
    glBindBuffer(GL_ARRAY_BUFFER, diagnosticVbo_.id());
    glBufferData(GL_ARRAY_BUFFER, diagnosticCapacity * sizeof(DiagnosticVertex), nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DiagnosticVertex),
                          reinterpret_cast<void*>(offsetof(DiagnosticVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DiagnosticVertex),
                          reinterpret_cast<void*>(offsetof(DiagnosticVertex, color)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    {
        // Static geometry for the comet's Bezier path: the sampled curve, its
        // closed control polygon, and a cross marker at every control point.
        const auto& path = system.comet().path();
        std::vector<DiagnosticVertex> vertices;
        for (const auto& point : path.sample(64))
            vertices.push_back({point, {0.45f, 0.78f, 1.0f, 0.42f}});
        cometPathCount_ = static_cast<GLsizei>(vertices.size());
        const auto polygon = path.controlPolygon();
        for (const auto& point : polygon)
            vertices.push_back({point, {1.0f, 0.62f, 0.25f, 0.85f}});
        cometPolygonCount_ = static_cast<GLsizei>(polygon.size());
        for (size_t i = 0; i < polygon.size(); ++i) {
            const bool anchor = i % 3 == 0;
            const float size = anchor ? 0.9f : 0.6f;
            const glm::vec4 color = anchor ? glm::vec4(1.0f) : glm::vec4(1.0f, 0.62f, 0.25f, 1.0f);
            for (const glm::vec3 axis : {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)}) {
                vertices.push_back({polygon[i] - axis * size, color});
                vertices.push_back({polygon[i] + axis * size, color});
            }
        }
        cometMarkerCount_ = static_cast<GLsizei>(polygon.size() * 6);
        glBindVertexArray(cometPathVao_.id());
        glBindBuffer(GL_ARRAY_BUFFER, cometPathVbo_.id());
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(DiagnosticVertex), vertices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DiagnosticVertex),
                              reinterpret_cast<void*>(offsetof(DiagnosticVertex, position)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(DiagnosticVertex),
                              reinterpret_cast<void*>(offsetof(DiagnosticVertex, color)));
    }
    tailVertices_.reserve(cometTailCapacity);
    glBindVertexArray(cometTailVao_.id());
    glBindBuffer(GL_ARRAY_BUFFER, cometTailVbo_.id());
    glBufferData(GL_ARRAY_BUFFER, cometTailCapacity * sizeof(TailVertex), nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TailVertex),
                          reinterpret_cast<void*>(offsetof(TailVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TailVertex),
                          reinterpret_cast<void*>(offsetof(TailVertex, color)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(TailVertex),
                          reinterpret_cast<void*>(offsetof(TailVertex, size)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnable(GL_PROGRAM_POINT_SIZE);
    std::cout << "Scene initialized: 8 planets, Moon, Saturn rings, Bezier comet, 4500 stars; " << fallbackCount_
              << " texture fallbacks\n";
}
void Renderer::resize(int width, int height) {
    drawable_ = width > 0 && height > 0;
    if (!drawable_ || (width == width_ && height == height_))
        return;
    width_ = width;
    height_ = height;
    bloomWidth_ = std::max(1, width / 2);
    bloomHeight_ = std::max(1, height / 2);
    allocateColor(sceneColor_.id(), width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo_.id());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColor_.id(), 0);
    checkFramebuffer();
    GLint maxSamples = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    const int samples = std::min(4, maxSamples);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo_.id());
    glBindRenderbuffer(GL_RENDERBUFFER, msaaColor_.id());
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA16F, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColor_.id());
    glBindRenderbuffer(GL_RENDERBUFFER, depth_.id());
    // Packed depth + 8-bit stencil: the stencil plane marks the selected planet's pixels.
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_.id());
    checkFramebuffer();
    for (size_t i = 0; i < 2; ++i) {
        allocateColor(blurColor_[i].id(), bloomWidth_, bloomHeight_);
        glBindFramebuffer(GL_FRAMEBUFFER, blurFbo_[i].id());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurColor_[i].id(), 0);
        checkFramebuffer();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Renderer::fullscreen() const {
    glBindVertexArray(emptyVao_.id());
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
void Renderer::atmosphere(const glm::mat4& model, const glm::mat4& vp, const glm::vec3& eye, glm::vec3 color,
                          float strength, bool sun, const Mesh* mesh) {
    atmosphere_.use();
    atmosphere_.set("uModel", model);
    atmosphere_.set("uVP", vp);
    atmosphere_.set("uEye", eye);
    atmosphere_.set("uColor", color);
    atmosphere_.set("uStrength", strength);
    atmosphere_.set("uSun", int(sun));
    (mesh ? *mesh : sphere_).draw();
}
void Renderer::diagnostics(const SolarSystem& system, const glm::mat4& vp, const glm::vec3& eye,
                           const RenderSettings& settings, int selected) {
    if (selected < 0 || static_cast<size_t>(selected) >= system.planets().size())
        return;
    const auto& planet = system.planets()[static_cast<size_t>(selected)];
    const auto& orbit = planet.orbitParameters();
    const double days = system.days();
    const float radius = planet.data().radius;
    diagnosticVertices_.clear();
    const auto line = [this](const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) {
        diagnosticVertices_.push_back({from, color});
        diagnosticVertices_.push_back({to, color});
    };
    if (settings.trails) {
        // Sample recent simulation time rather than frame history: seeking, pausing,
        // loading and orbit edits cannot leave stale points from another trajectory.
        constexpr int samples = 192;
        const double span = orbit.periodDays * 0.22;
        const glm::vec3 color = glm::mix(planet.data().color, glm::vec3(0.5f, 0.9f, 1.0f), 0.6f);
        glm::vec3 previous = planet.position(days - span);
        for (int i = 1; i <= samples; ++i) {
            const float t = static_cast<float>(i) / samples;
            const auto next = planet.position(days - span * (1.0 - static_cast<double>(i) / samples));
            diagnosticVertices_.push_back({previous, glm::vec4(color, 0.06f + 0.94f * (t - 1.0f / samples))});
            diagnosticVertices_.push_back({next, glm::vec4(color, 0.06f + 0.94f * t)});
            previous = next;
        }
    }
    if (settings.velocityVectors) {
        const glm::vec3 velocity = planet.velocity(days);
        const float speed = glm::length(velocity);
        if (std::isfinite(speed) && speed > 1e-8f) {
            const glm::vec3 position = planet.position(days), direction = velocity / speed;
            // A fixed fraction of the orbit's mean-motion timescale makes velocity
            // legible in scene units while retaining its variation around the ellipse.
            const float length = std::clamp(static_cast<float>(speed * orbit.periodDays * 0.045),
                                            radius * 2.2f, 18.0f);
            const glm::vec3 tip = position + direction * length;
            glm::vec3 side = glm::cross(direction, eye - tip);
            if (glm::dot(side, side) < 1e-8f)
                side = glm::cross(direction, glm::vec3(0, 1, 0));
            if (glm::dot(side, side) < 1e-8f)
                side = glm::cross(direction, glm::vec3(0, 0, 1));
            side = glm::normalize(side);
            const float head = std::min(length * 0.25f, std::max(0.45f, radius * 0.8f));
            const glm::vec4 color(0.25f, 1.0f, 0.68f, 1.0f);
            line(position, tip, color);
            line(tip, tip - direction * head + side * head * 0.48f, color);
            line(tip, tip - direction * head - side * head * 0.48f, color);
        }
    }
    if (settings.apsides) {
        const auto plane = glm::rotate(glm::mat4(1), glm::radians(static_cast<float>(orbit.inclinationDegrees)),
                                        glm::vec3(0, 0, 1));
        const glm::vec3 axis = glm::vec3(plane * glm::vec4(1, 0, 0, 0));
        const glm::vec3 across = glm::vec3(plane * glm::vec4(0, 0, 1, 0));
        const float size = std::max(0.6f, radius * 1.35f);
        const auto marker = [&](const glm::vec3& center, const glm::vec4& color) {
            const glm::vec3 a = center + axis * size, b = center + across * size,
                            c = center - axis * size, d = center - across * size;
            line(a, b, color);
            line(b, c, color);
            line(c, d, color);
            line(d, a, color);
            line(center - axis * size * 0.35f, center + axis * size * 0.35f, color);
            line(center - across * size * 0.35f, center + across * size * 0.35f, color);
        };
        marker(axis * static_cast<float>(planet.periapsis()), {0.25f, 1.0f, 0.68f, 1.0f});
        marker(-axis * static_cast<float>(planet.apoapsis()), {1.0f, 0.48f, 0.25f, 1.0f});
    }
    if (settings.surfaceMode == SurfaceMode::Normals) {
        const auto model = planet.modelMatrix(days);
        const auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        const auto glyph = [&](const glm::vec3& local) {
            const glm::vec3 normal = glm::normalize(normalMatrix * local);
            const glm::vec3 start = glm::vec3(model * glm::vec4(local * 1.003f, 1));
            line(start, start + normal * radius * 0.28f, glm::vec4(normal * 0.5f + 0.5f, 1));
        };
        for (int latitude = -60; latitude <= 60; latitude += 30) {
            const float theta = glm::radians(static_cast<float>(latitude));
            for (int longitude = 0; longitude < 12; ++longitude) {
                const float phi = glm::radians(longitude * 30.0f + (latitude == 0 ? 15.0f : 0.0f));
                glyph({std::cos(theta) * std::cos(phi), std::sin(theta), std::cos(theta) * std::sin(phi)});
            }
        }
        glyph({0, 1, 0});
        glyph({0, -1, 0});
    }
    if (diagnosticVertices_.empty())
        return;
    debugLines_.use();
    debugLines_.set("uVP", vp);
    glBindVertexArray(diagnosticVao_.id());
    glBindBuffer(GL_ARRAY_BUFFER, diagnosticVbo_.id());
    glBufferSubData(GL_ARRAY_BUFFER, 0, diagnosticVertices_.size() * sizeof(DiagnosticVertex),
                    diagnosticVertices_.data());
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(diagnosticVertices_.size()));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void Renderer::cometPath(const glm::mat4& vp, const RenderSettings& settings) {
    if (!settings.comet || (!settings.orbits && !settings.bezierControl))
        return;
    debugLines_.use();
    debugLines_.set("uVP", vp);
    glBindVertexArray(cometPathVao_.id());
    glDrawArrays(GL_LINE_STRIP, 0, cometPathCount_);
    if (settings.bezierControl) {
        // The control polygon is closed: the last in-handle connects back to anchor 0.
        glDrawArrays(GL_LINE_LOOP, cometPathCount_, cometPolygonCount_);
        glDrawArrays(GL_LINES, cometPathCount_ + cometPolygonCount_, cometMarkerCount_);
    }
}
void Renderer::cometTails(const SolarSystem& system, const glm::mat4& vp, float pixelScale) {
    const auto& comet = system.comet();
    const double days = system.days();
    const glm::vec3 nucleus = comet.position(days);
    const float r = std::max(glm::length(nucleus), 0.001f);
    const glm::vec3 away = nucleus / r;
    // Sublimation grows quickly as the comet approaches the Sun, so the tails are
    // bright and long near perihelion and almost vanish in the outer system.
    const float activity = std::clamp(std::pow(12.0f / r, 1.6f), 0.0f, 1.6f);
    if (activity < 0.02f)
        return;
    const float ionLength = std::clamp(70.0f / r, 1.5f, 13.0f);
    tailVertices_.clear();
    // Ion (plasma) tail: blown straight away from the Sun by the solar wind.
    for (int i = 0; i < ionParticles; ++i) {
        const float s = static_cast<float>(i) / (ionParticles - 1);
        const float fade = std::pow(1.0f - s, 1.5f);
        tailVertices_.push_back({nucleus + away * (ionLength * s), {0.56f, 1.0f, 1.6f, 0.5f * activity * fade},
                                 Comet::radius * (1.2f + 3.0f * s)});
    }
    // Dust tail: grains released at earlier times keep the comet's old orbit and
    // are pushed outward by radiation pressure, so the tail curves behind it.
    constexpr double maxAge = 9.0;
    const float push = ionLength * 0.55f / static_cast<float>(maxAge);
    for (int i = 1; i <= dustParticles; ++i) {
        const float s = static_cast<float>(i) / dustParticles;
        const double age = maxAge * s;
        const glm::vec3 origin = comet.position(days - age);
        const glm::vec3 radial = glm::normalize(origin);
        tailVertices_.push_back({origin + radial * (push * static_cast<float>(age)),
                                 {1.5f, 1.15f, 0.7f, 0.42f * activity * (1.0f - s)},
                                 Comet::radius * (1.6f + 5.0f * s)});
    }
    cometTail_.use();
    cometTail_.set("uVP", vp);
    cometTail_.set("uPixelScale", pixelScale);
    glBindVertexArray(cometTailVao_.id());
    glBindBuffer(GL_ARRAY_BUFFER, cometTailVbo_.id());
    glBufferSubData(GL_ARRAY_BUFFER, 0, tailVertices_.size() * sizeof(TailVertex), tailVertices_.data());
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(tailVertices_.size()));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void Renderer::render(const SolarSystem& system, const Camera& camera, const RenderSettings& settings,
                      int selected, float realTime, bool focusPresentation) {
    if (!drawable_)
        return;
    const RasterState rasterState;
    const bool shaded = settings.surfaceMode == SurfaceMode::Shaded;
    const bool wireframe = settings.surfaceMode == SurfaceMode::Wireframe;
    const glm::vec3 lighting(settings.ambient, settings.diffuse, settings.specular);
    // A coarse 16x8 sphere makes the difference between per-vertex (Gouraud)
    // and per-pixel (Phong) lighting obvious; the Sun always stays smooth.
    const Mesh& body = settings.lowPoly ? lowSphere_ : sphere_;
    const auto view = camera.view(), projection = camera.projection(float(width_) / height_),
               vp = projection * view;
    // The sky and stars are at infinity: they always use the perspective
    // projection, otherwise an orthographic view would collapse them to a smear.
    const auto skyVP = camera.perspectiveProjection(float(width_) / height_) * glm::mat4(glm::mat3(view));
    const auto eye = camera.position();
    const double days = system.days();
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo_.id());
    glViewport(0, 0, width_, height_);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glClearColor(0, 0, 0, 1);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    const bool outline = settings.outline && shaded && selected >= 0 &&
                         static_cast<size_t>(selected) < system.planets().size();
    glDisable(GL_DEPTH_TEST);
    sky_.use();
    sky_.set("uInverseVP", glm::inverse(skyVP));
    sky_.set("uSky", 0);
    skyMap_.bind();
    fullscreen();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    star_.use();
    star_.set("uVP", skyVP);
    star_.set("uTime", realTime);
    stars_.draw();
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    const auto sunModel =
        glm::rotate(glm::mat4(1), static_cast<float>(std::remainder(days / 1500.0, 1.0) * 6.2831853),
                    glm::vec3(0, 1, 0)) *
        glm::scale(glm::mat4(1), glm::vec3(SolarSystem::sunRadius));
    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if (shaded) {
        sun_.use();
        sun_.set("uModel", sunModel);
        sun_.set("uVP", vp);
        sun_.set("uEye", eye);
        sun_.set("uTime", realTime);
        sun_.set("uSurface", 0);
        sunMap_.bind();
    } else {
        debugSurface_.use();
        debugSurface_.set("uVP", vp);
        debugSurface_.set("uModel", sunModel);
        debugSurface_.set("uNormals", int(settings.surfaceMode == SurfaceMode::Normals));
        debugSurface_.set("uColor", glm::vec3(1.0f, 0.75f, 0.3f));
    }
    sphere_.draw();
    if (shaded) {
        planet_.use();
        planet_.set("uVP", vp);
        planet_.set("uEye", eye);
        planet_.set("uSurface", 0);
        planet_.set("uNight", 1);
        planet_.set("uRing", 2);
        planet_.set("uClouds", 0);
        planet_.set("uLighting", lighting);
        planet_.set("uShadingModel", static_cast<int>(settings.shading));
        nightMap_.bind(1);
        ringMap_.bind(2);
    }
    for (size_t i = 0; i < system.planets().size(); ++i) {
        const auto& p = system.planets()[i];
        if (shaded) {
            planet_.set("uModel", p.modelMatrix(days));
            planet_.set("uEarth", int(i == 2));
            planet_.set("uRingShadow", int(i == 5));
            if (i == 5) {
                planet_.set("uBodyCenter", p.position(days));
                planet_.set("uRadius", p.data().radius);
                planet_.set("uRingNormal", glm::vec3(p.equatorialFrame(days) * glm::vec4(0, 1, 0, 0)));
            }
            surfaces_[i].bind();
        } else {
            debugSurface_.set("uModel", p.modelMatrix(days));
            debugSurface_.set("uColor", glm::mix(p.data().color, glm::vec3(1), 0.3f));
        }
        const bool marked = outline && static_cast<int>(i) == selected;
        if (marked) {
            // Pass 1: every visible fragment of the selected planet writes 1 to the stencil buffer.
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilMask(0xFF);
        }
        body.draw();
        if (marked)
            glDisable(GL_STENCIL_TEST);
    }
    if (shaded) {
        planet_.set("uEarth", 0);
        planet_.set("uRingShadow", 0);
        planet_.set("uModel", system.moonModel(days));
        moonMap_.bind();
    } else {
        debugSurface_.set("uModel", system.moonModel(days));
        debugSurface_.set("uColor", glm::vec3(0.75f));
    }
    body.draw();
    const glm::vec3 cometPosition = system.comet().position(days);
    if (settings.comet) {
        // A slightly flattened, tumbling nucleus; it reuses the Moon's rocky texture.
        const auto cometModel =
            glm::translate(glm::mat4(1), cometPosition) *
            glm::rotate(glm::mat4(1), static_cast<float>(std::remainder(days / 3.0, 1.0) * 6.2831853),
                        glm::normalize(glm::vec3(0.3f, 1, 0.2f))) *
            glm::scale(glm::mat4(1), glm::vec3(Comet::radius, Comet::radius * 0.8f, Comet::radius));
        if (shaded) {
            planet_.set("uModel", cometModel);
            moonMap_.bind();
        } else {
            debugSurface_.set("uModel", cometModel);
            debugSurface_.set("uColor", glm::vec3(0.6f, 0.85f, 1.0f));
        }
        body.draw();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (outline) {
        // Pass 2: draw a slightly enlarged copy in a flat HDR colour, but only where
        // the stencil is NOT 1. What survives is a thin ring around the silhouette,
        // which bloom turns into a soft glow. The width is kept constant in pixels.
        const auto& planet = system.planets()[static_cast<size_t>(selected)];
        const glm::vec3 center = planet.position(days);
        const float w = std::max((vp * glm::vec4(center, 1)).w, 1e-4f);
        const float worldPerPixel = 2.0f * w / (projection[1][1] * static_cast<float>(height_));
        const float pulse = 1.0f + 0.2f * std::sin(realTime * 3.0f);
        const float scale = std::min(1.0f + 3.0f * pulse * worldPerPixel / planet.data().radius, 1.6f);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDepthMask(GL_FALSE);
        debugSurface_.use();
        debugSurface_.set("uVP", vp);
        debugSurface_.set("uNormals", 0);
        debugSurface_.set("uModel", planet.modelMatrix(days) * glm::scale(glm::mat4(1), glm::vec3(scale)));
        debugSurface_.set("uColor", glm::mix(planet.data().color, glm::vec3(1), 0.25f) * 2.4f);
        body.draw();
        glStencilMask(0xFF);
        glDisable(GL_STENCIL_TEST);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (settings.orbits) {
        float closeUp = 0;
        if (focusPresentation && selected >= 0 && static_cast<size_t>(selected) < system.planets().size() &&
            camera.followed() == selected) {
            const float relativeDistance = camera.distance() / system.planets()[selected].data().radius;
            closeUp = 1.0f - glm::smoothstep(10.0f, 25.0f, relativeDistance);
        }
        orbit_.use();
        orbit_.set("uVP", vp);
        for (size_t i = 0; i < system.planets().size(); ++i) {
            const auto& p = system.planets()[i];
            const auto& parameters = p.orbitParameters();
            const float a = static_cast<float>(parameters.semiMajorAxis);
            const float e = static_cast<float>(parameters.eccentricity);
            const bool chosen = static_cast<int>(i) == selected;
            orbit_.set("uColor", chosen ? p.data().color * 0.65f : glm::vec3(0.09f, 0.15f, 0.21f));
            orbit_.set("uAlpha", chosen ? glm::mix(.9f, .12f, closeUp) : glm::mix(.5f, .025f, closeUp));
            orbit_.set("uModel", glm::rotate(glm::mat4(1), glm::radians(static_cast<float>(parameters.inclinationDegrees)),
                                             glm::vec3(0, 0, 1)) *
                                     glm::translate(glm::mat4(1), glm::vec3(-a * e, 0, 0)) *
                                     glm::scale(glm::mat4(1), glm::vec3(a, 1, a * std::sqrt(1 - e * e))));
            orbitMesh_.draw();
        }
        orbit_.set("uColor", glm::vec3(0.2f, 0.33f, 0.48f));
        orbit_.set("uAlpha", glm::mix(.6f, .12f, closeUp));
        orbit_.set("uModel", system.moonFrame(days) *
                                 glm::scale(glm::mat4(1), glm::vec3(SolarSystem::moonOrbitRadius)));
        orbitMesh_.draw();
    }
    cometPath(vp, settings);
    // Transparent annulus: opaque planet depth masks the hidden half; draw both faces.
    glDisable(GL_CULL_FACE);
    const auto& saturn = system.planets()[5];
    const auto ringModel = saturn.equatorialFrame(days) * glm::scale(glm::mat4(1), glm::vec3(saturn.data().radius));
    if (shaded) {
        ring_.use();
        ring_.set("uVP", vp);
        ring_.set("uModel", ringModel);
        ring_.set("uCenter", saturn.position(days));
        ring_.set("uRadius", saturn.data().radius);
        ring_.set("uRing", 0);
        ring_.set("uLighting", lighting);
        ringMap_.bind();
    } else {
        debugSurface_.use();
        debugSurface_.set("uModel", ringModel);
        debugSurface_.set("uColor", glm::vec3(0.78f, 0.68f, 0.5f));
    }
    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    annulus_.draw();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
    if (shaded) {
        // Cloud and atmosphere shells are omitted in diagnostic modes so that
        // overlapping shells do not obscure topology or the surface normals.
        const auto& earth = system.planets()[2];
        planet_.use();
        planet_.set("uModel",
                    earth.modelMatrix(days) *
                        glm::rotate(glm::mat4(1), cloudRotationRadians(days), glm::vec3(0, 1, 0)) *
                        glm::scale(glm::mat4(1), glm::vec3(1.012f)));
        planet_.set("uClouds", 1);
        cloudMap_.bind();
        body.draw();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        atmosphere(earth.modelMatrix(days) * glm::scale(glm::mat4(1), glm::vec3(1.055f)), vp, eye,
                   {0.12f, 0.48f, 1.5f}, 0.82f, false, &body);
        atmosphere(sunModel * glm::scale(glm::mat4(1), glm::vec3(1.12f)), vp, eye, {2.0f, 0.55f, 0.08f}, 0.48f,
                   true);
        if (settings.comet) {
            const float coma = std::clamp(std::pow(12.0f / glm::length(cometPosition), 1.2f), 0.15f, 1.4f);
            atmosphere(glm::translate(glm::mat4(1), cometPosition) *
                           glm::scale(glm::mat4(1), glm::vec3(Comet::radius * 3.2f)),
                       vp, eye, {0.5f, 1.1f, 1.6f}, coma, true);
            cometTails(system, vp, projection[1][1] * static_cast<float>(height_) * 0.5f);
        }
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    diagnostics(system, vp, eye, settings, selected);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFbo_.id());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneFbo_.id());
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    const bool normalColors = settings.surfaceMode == SurfaceMode::Normals;
    const bool bloom = settings.bloom && !normalColors;
    if (bloom) {
        blur_.use();
        blur_.set("uInput", 0);
        glActiveTexture(GL_TEXTURE0);
        glViewport(0, 0, bloomWidth_, bloomHeight_);
        for (int pass = 0; pass < 10; ++pass) {
            glBindFramebuffer(GL_FRAMEBUFFER, blurFbo_[pass % 2].id());
            blur_.set("uHorizontal", int(pass % 2 == 0));
            blur_.set("uExtract", int(pass == 0));
            glBindTexture(GL_TEXTURE_2D, pass == 0 ? sceneColor_.id() : blurColor_[(pass - 1) % 2].id());
            fullscreen();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);
    composite_.use();
    composite_.set("uScene", 0);
    composite_.set("uBloom", 1);
    composite_.set("uBloomEnabled", int(bloom));
    composite_.set("uNormalColors", int(normalColors));
    composite_.set("uExposure", settings.exposure);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColor_.id());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurColor_[1].id());
    fullscreen();
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}
void Renderer::capture(const std::filesystem::path& file, int width, int height) const {
    if (width <= 0 || height <= 0)
        throw std::runtime_error("Cannot capture a zero-size framebuffer");
    if (file.has_parent_path())
        std::filesystem::create_directories(file.parent_path());
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    stbi_flip_vertically_on_write(1);
    if (!stbi_write_png(file.u8string().c_str(), width, height, 3, pixels.data(), width * 3))
        throw std::runtime_error("Could not save screenshot: " + file.u8string());
    std::cout << "Captured: " << file.u8string() << '\n';
}
} // namespace sse
