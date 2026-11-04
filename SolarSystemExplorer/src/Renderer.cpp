#include "Renderer.h"
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
#include <iostream>
#include <stdexcept>
namespace sse {
namespace {
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
      composite_(root / "shaders/fullscreen.vert", root / "shaders/composite.frag"), sphere_(makeSphere()),
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
    glEnable(GL_PROGRAM_POINT_SIZE);
    std::cout << "Scene initialized: 8 planets, Moon, Saturn rings, 4500 stars; " << fallbackCount_
              << " texture fallbacks\n";
}
void Renderer::resize(int width, int height) {
    if (width <= 0 || height <= 0 || (width == width_ && height == height_))
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
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_.id());
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
                          float strength, bool sun) {
    atmosphere_.use();
    atmosphere_.set("uModel", model);
    atmosphere_.set("uVP", vp);
    atmosphere_.set("uEye", eye);
    atmosphere_.set("uColor", color);
    atmosphere_.set("uStrength", strength);
    atmosphere_.set("uSun", int(sun));
    sphere_.draw();
}
void Renderer::render(const SolarSystem& system, const Camera& camera, const RenderSettings& settings,
                      int selected, float realTime) {
    if (width_ <= 0 || height_ <= 0)
        return;
    const auto view = camera.view(), projection = camera.projection(float(width_) / height_),
               vp = projection * view;
    const auto skyVP = projection * glm::mat4(glm::mat3(view));
    const auto eye = camera.position();
    const double days = system.days();
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFbo_.id());
    glViewport(0, 0, width_, height_);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    sun_.use();
    sun_.set("uModel", sunModel);
    sun_.set("uVP", vp);
    sun_.set("uEye", eye);
    sun_.set("uTime", realTime);
    sun_.set("uSurface", 0);
    sunMap_.bind();
    sphere_.draw();
    planet_.use();
    planet_.set("uVP", vp);
    planet_.set("uEye", eye);
    planet_.set("uSurface", 0);
    planet_.set("uNight", 1);
    planet_.set("uRing", 2);
    planet_.set("uClouds", 0);
    nightMap_.bind(1);
    ringMap_.bind(2);
    for (size_t i = 0; i < system.planets().size(); ++i) {
        const auto& p = system.planets()[i];
        planet_.set("uModel", p.modelMatrix(days));
        planet_.set("uEarth", int(i == 2));
        planet_.set("uRingShadow", int(i == 5));
        if (i == 5) {
            planet_.set("uBodyCenter", p.position(days));
            planet_.set("uRadius", p.data().radius);
            planet_.set("uRingNormal", glm::vec3(p.equatorialFrame(days) * glm::vec4(0, 1, 0, 0)));
        }
        surfaces_[i].bind();
        sphere_.draw();
    }
    planet_.set("uEarth", 0);
    planet_.set("uRingShadow", 0);
    planet_.set("uModel", system.moonModel(days));
    moonMap_.bind();
    sphere_.draw();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    if (settings.orbits) {
        orbit_.use();
        orbit_.set("uVP", vp);
        for (size_t i = 0; i < system.planets().size(); ++i) {
            const auto& p = system.planets()[i];
            const bool chosen = static_cast<int>(i) == selected;
            orbit_.set("uColor", chosen ? p.data().color * 0.65f : glm::vec3(0.09f, 0.15f, 0.21f));
            orbit_.set("uAlpha", chosen ? 0.9f : 0.5f);
            orbit_.set("uModel", glm::rotate(glm::mat4(1), glm::radians(p.data().inclinationDegrees),
                                             glm::vec3(0, 0, 1)) *
                                     glm::scale(glm::mat4(1), glm::vec3(p.data().orbitRadius)));
            orbitMesh_.draw();
        }
        orbit_.set("uColor", glm::vec3(0.2f, 0.33f, 0.48f));
        orbit_.set("uAlpha", 0.6f);
        orbit_.set("uModel", system.moonFrame(days) *
                                 glm::scale(glm::mat4(1), glm::vec3(SolarSystem::moonOrbitRadius)));
        orbitMesh_.draw();
    }
    // Transparent annulus: opaque planet depth masks the hidden half; draw both faces.
    glDisable(GL_CULL_FACE);
    const auto& saturn = system.planets()[5];
    ring_.use();
    ring_.set("uVP", vp);
    ring_.set("uModel",
              saturn.equatorialFrame(days) * glm::scale(glm::mat4(1), glm::vec3(saturn.data().radius)));
    ring_.set("uCenter", saturn.position(days));
    ring_.set("uRadius", saturn.data().radius);
    ring_.set("uRing", 0);
    ringMap_.bind();
    annulus_.draw();
    glEnable(GL_CULL_FACE);
    const auto& earth = system.planets()[2];
    planet_.use();
    planet_.set("uModel",
                earth.modelMatrix(days) *
                    glm::rotate(glm::mat4(1), float(std::remainder(days / 1000.0, 1.0)), glm::vec3(0, 1, 0)) *
                    glm::scale(glm::mat4(1), glm::vec3(1.012f)));
    planet_.set("uClouds", 1);
    cloudMap_.bind();
    sphere_.draw();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    atmosphere(earth.modelMatrix(days) * glm::scale(glm::mat4(1), glm::vec3(1.055f)), vp, eye,
               {0.12f, 0.48f, 1.5f}, 0.82f, false);
    atmosphere(sunModel * glm::scale(glm::mat4(1), glm::vec3(1.12f)), vp, eye, {2.0f, 0.55f, 0.08f}, 0.48f,
               true);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFbo_.id());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sceneFbo_.id());
    glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    if (settings.bloom) {
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
    composite_.set("uBloomEnabled", int(settings.bloom));
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
