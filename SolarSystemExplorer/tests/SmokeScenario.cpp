#include "SmokeScenario.h"
#include "Application.h"
#include "LaboratoryTarget.h"
#include "WorkshopRenderer.h"
#include "EclipseRenderer.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace sse {
namespace {
void verifyLaboratoryTarget(const std::filesystem::path& root) {
    LaboratoryTarget target(root);
    target.begin(32, 32);
    GLint samples = 0, maximum = 0;
    glGetIntegerv(GL_SAMPLES, &samples);
    glGetIntegerv(GL_MAX_SAMPLES, &maximum);
    if (samples != std::min(4, maximum))
        throw std::runtime_error("Smoke: laboratory target did not enable multisampling");
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClearColor(.25f, .25f, .25f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    Shader overlay(root / "shaders/fullscreen.vert", root / "shaders/eclipse_line.frag");
    overlay.use();
    overlay.set("uColor", glm::vec3(.75f, .25f, .1f));
    overlay.set("uAlpha", .5f);
    GLObject triangle(GLKind::VertexArray);
    glBindVertexArray(triangle.id());
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    target.present(1, false);
    std::array<unsigned char, 4> pixel{};
    glReadBuffer(GL_BACK);
    glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    const glm::vec3 linear(.5f, .25f, .175f);
    for (int channel = 0; channel < 3; ++channel) {
        const float expected = std::pow(1 - std::exp(-linear[channel]), 1.0f / 2.2f) * 255;
        if (std::abs(float(pixel[size_t(channel)]) - expected) > 2)
            throw std::runtime_error("Smoke: laboratory transparency was not composed in linear light");
    }
    target.present(.2f, true);
    glReadPixels(16, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    for (int channel = 0; channel < 3; ++channel)
        if (std::abs(float(pixel[size_t(channel)]) - linear[channel] * 255) > 2)
            throw std::runtime_error("Smoke: laboratory diagnostic colors were tone-mapped");
    glBindVertexArray(0);
    glUseProgram(0);
}

void verifyLaboratoryState(const std::filesystem::path& root) {
    WorkshopRenderer workshop(root);
    EclipseRenderer eclipse(root);
    GLObject texture(GLKind::Texture), renderbuffer(GLKind::Renderbuffer);
    glActiveTexture(GL_TEXTURE0);
    GLint originalTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &originalTexture);
    glBindTexture(GL_TEXTURE_2D, texture.id());
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer.id());
    glActiveTexture(GL_TEXTURE3);
    glDisable(GL_MULTISAMPLE);
    Camera camera;
    camera.setViewport(320, 240, false);
    camera.focus(0, {}, 1.25f);
    for (int step = 0; step < 200; ++step) camera.update(.1f, {});
    RenderSettings graphics;
    graphics.surfaceMode = SurfaceMode::Normals;
    auto readCenter = [](int y) {
        std::array<unsigned char, 4> pixel{};
        glReadBuffer(GL_BACK);
        glReadPixels(160, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        return pixel;
    };
    for (int renderer = 0; renderer < 2; ++renderer) {
        std::array<unsigned char, 4> reference{};
        for (int variant = 0; variant < 2; ++variant) {
            graphics.exposure = variant == 0 ? .2f : 3;
            if (renderer == 0) workshop.render(WorkshopSettings{}, graphics, camera, 320, 240);
            else eclipse.render(EclipseSettings{}, graphics, {0, 0, 320, 240}, 320, 240);
            const auto pixel = readCenter(renderer == 0 ? 120 : 59);
            if (std::max({pixel[0], pixel[1], pixel[2]}) < 80)
                throw std::runtime_error("Smoke: laboratory diagnostic fixture missed the surface");
            if (variant == 0) reference = pixel;
            else if (reference != pixel)
                throw std::runtime_error("Smoke: laboratory normal colors depend on exposure");
            GLint active = 0, binding = 0;
            glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
            glGetIntegerv(GL_RENDERBUFFER_BINDING, &binding);
            if (active != GL_TEXTURE3 || GLuint(binding) != renderbuffer.id() || glIsEnabled(GL_MULTISAMPLE))
                throw std::runtime_error("Smoke: laboratory renderer changed caller GL state");
            glActiveTexture(GL_TEXTURE0);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
            if (GLuint(binding) != texture.id())
                throw std::runtime_error("Smoke: laboratory renderer changed caller texture binding");
            glActiveTexture(GL_TEXTURE3);
        }
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GLuint(originalTexture));
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glEnable(GL_MULTISAMPLE);
}

void verifyNormalColors(GLFWwindow* window, Renderer& renderer) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    renderer.resize(width, height);
    SolarSystem fixture;
    Camera camera;
    camera.setViewport(float(width), float(height), false);
    const auto& earth = fixture.planets()[2];
    const auto center = earth.position(0);
    camera.focus(2, center, earth.data().radius);
    for (int step = 0; step < 200; ++step)
        camera.update(.1f, center);
    RenderSettings settings;
    settings.surfaceMode = SurfaceMode::Normals;
    settings.orbits = settings.labels = settings.trails = settings.velocityVectors = settings.apsides = false;
    std::array<unsigned char, 4> reference{};
    for (int variant = 0; variant < 4; ++variant) {
        settings.exposure = variant < 2 ? .2f : 3.0f;
        settings.bloom = (variant % 2) != 0;
        renderer.render(fixture, camera, settings, -1, 0);
        std::array<unsigned char, 4> pixel{};
        glReadBuffer(GL_BACK);
        glReadPixels(width / 2, height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        if (variant == 0)
            reference = pixel;
        else if (pixel != reference)
            throw std::runtime_error("Smoke: world-normal colors changed with exposure or bloom");
    }
    // At the center of a sphere the visible normal points toward the camera.
    // Allow a few byte values for the half-pixel offset and mesh interpolation.
    const auto expected = glm::normalize(camera.position() - center) * .5f + .5f;
    for (int channel = 0; channel < 3; ++channel)
        if (std::abs(float(reference[size_t(channel)]) - expected[channel] * 255) > 4)
            throw std::runtime_error("Smoke: world-normal RGB does not encode the surface direction");
}
} // namespace
void smokeFrame(int frame, GLFWwindow* window, SolarSystem& system, Camera& camera, Renderer& renderer,
                RenderSettings& settings, const std::function<void(int)>& select) {
    if (frame == 0) {
        system.setDays(0);
        system.setPaused(true);
    }
    // A focused ImGui panel must not swallow a destination shortcut at startup.
    if (frame == 3)
        ImGui::GetIO().AddKeyEvent(ImGuiKey_3, true);
    if (frame == 4)
        ImGui::GetIO().AddKeyEvent(ImGuiKey_3, false);
    if (frame == 5 && camera.followed() != 2)
        throw std::runtime_error("Smoke: startup UI swallowed the planet keyboard shortcut");
    // Ctrl-click the real speed slider, then cancel its temporary text editor.
    // Coordinates are in the default 1440x900 ImGui display, before the resize test.
    if (frame == 10) {
        ImGui::GetIO().AddMousePosEvent(210, 838);
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, true);
        ImGui::GetIO().AddMouseButtonEvent(0, true);
    }
    if (frame == 11) {
        ImGui::GetIO().AddMouseButtonEvent(0, false);
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, false);
    }
    if (frame == 13) {
        if (!ImGui::GetIO().WantTextInput)
            throw std::runtime_error("Smoke: speed slider text editor did not activate");
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true);
    }
    if (frame == 14)
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false);
    if (frame == 16 && (glfwWindowShouldClose(window) || ImGui::GetIO().WantTextInput))
        throw std::runtime_error("Smoke: Escape did not cancel the slider editor independently");
    if (frame == 29 && system.days() != 0)
        throw std::runtime_error("Smoke: paused time advanced");
    if (frame == 20)
        verifyNormalColors(window, renderer);
    if (frame == 21) {
        const auto root = assetRoot("");
        verifyLaboratoryTarget(root);
        verifyLaboratoryState(root);
    }
    if (frame == 30) {
        system.setPaused(false);
        system.setSpeed(100);
        select(0);
    }
    if (frame == 59 && system.days() < 280)
        throw std::runtime_error("Smoke: accelerated time did not advance");
    if (frame == 60) {
        system.setSpeed(1);
        select(5);
    }
    if (frame == 90)
        glfwSetWindowSize(window, 1000, 680);
    if (frame == 100) {
        renderer.resize(0, 0);
        renderer.resize(1000, 680);
    }
    if (frame >= 120 && frame < 240 && frame % 15 == 0)
        select((frame - 120) / 15);
    if (frame == 245) {
        settings.orbits = false;
        settings.labels = false;
        settings.bloom = false;
        camera.orbit(340, 160);
        camera.zoom(8);
    }
    if (frame == 260) {
        camera.pan(1, 1, 0.5f);
        camera.zoom(-4);
    }
    if (frame == 275) {
        settings.orbits = true;
        settings.labels = true;
        settings.bloom = true;
        glfwSetWindowSize(window, 1440, 900);
    }
    if (frame == 300)
        select(-1);
    if (frame == 350 && camera.followed() != -1)
        throw std::runtime_error("Smoke: overview did not detach camera");
    const auto p = camera.position();
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        throw std::runtime_error("Smoke: invalid camera position");
    if (frame == 359)
        std::cout << "Smoke assertions passed; final simulation day " << system.days() << '\n';
}
} // namespace sse
