#include "DemoTour.h"
#include "Missions.h"
#include "UIManager.h"
#include <GL/glew.h>
#include <imgui.h>
#include <stb_image_write.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace sse {
namespace {
constexpr int at(int seconds) { return seconds * demoTourFps; }
struct Caption {
    int start;
    const char* text;
};
constexpr Caption captions[] = {
    {at(0), "The Sun fills the HDR scene with light. Bloom spreads the brightest pixels."},
    {at(4), "Bloom off. The halo outside the Sun is reduced."},
    {at(7), "Bloom on. The post-process adds the blurred highlights."},
    {at(9), "One camera and one scene. Left without Bloom, right with Bloom."},
    {at(16), "Scene before Bloom is added."},
    {at(18), "Bright extraction keeps values above display white."},
    {at(20), "Blur spreads the extracted highlights into neighboring pixels."},
    {at(22), "Final composite adds Bloom and then tone maps the scene."},
    {at(24), "Earth's surface, clouds and atmosphere move with the planet."},
    {at(28), "Saturn's rings surround and cast a shadow on the planet."},
    {at(32), "Overview of the planetary orbits and the asteroid belt."},
    {at(34), "Planet Workshop. Start the Read the normals challenge."},
    {at(37), "Rotation is paused. Moving the light sweeps the terminator across the surface."},
    {at(45), "World normals encode surface direction in RGB."},
    {at(49), "The challenge check evaluates the paused rotation and the active surface view."},
    {at(54), "Solar System Explorer | Topic 2: Solar System"},
};

void focusPlanet(int index, const SolarSystem& system, Camera& camera, int& selected) {
    selected = index;
    const auto& planet = system.planets().at(static_cast<size_t>(index));
    camera.focus(index, planet.position(system.days()), planet.data().radius, index == 5);
}

void focusSun(Camera& camera, int& selected) {
    selected = -1;
    camera.overview();
    camera.zoom(14);
}
} // namespace

void advanceDemoTour(int frame, SolarSystem& system, Camera& camera, Camera& workshopCamera,
                     RenderSettings& settings, LabState& lab, int& selected, UIManager& ui) {
    constexpr int normalsChallenge = 2;
    switch (frame) {
    case at(0):
        settings = RenderSettings{};
        lab = LabState{};
        system.setPaused(true);
        focusSun(camera, selected);
        camera.update(2, {});
        ui.showProject(false);
        break;
    case at(4): settings.bloom = false; break;
    case at(7): settings.bloom = true; break;
    case at(9): settings.bloomCompare = true; break;
    case at(16): settings.bloomCompare = false; settings.pipelineStage = 1; break;
    case at(18): settings.pipelineStage = 2; break;
    case at(20): settings.pipelineStage = 3; break;
    case at(22): settings.pipelineStage = 0; break;
    case at(24): focusPlanet(2, system, camera, selected); break;
    case at(28): focusPlanet(5, system, camera, selected); break;
    case at(32):
        selected = -1;
        camera.overview();
        break;
    case at(34):
        startMission(normalsChallenge, lab);
        workshopCamera.focus(0, {}, lab.workshop.radius * (1 + lab.workshop.relief));
        workshopCamera.update(2, {});
        ui.setLabMessage("Challenge started: Read the normals");
        break;
    case at(37):
        lab.workshop.animate = false;
        break;
    case at(45):
        settings.surfaceMode = SurfaceMode::Normals;
        break;
    case at(49):
        if (!completeMission(normalsChallenge, system, settings, lab, selected))
            throw std::runtime_error("Demo tour could not complete the Read the normals challenge.");
        ui.setLabMessage("Challenge complete: Read the normals");
        break;
    case at(54):
        lab.view = LabView::Explore;
        settings.surfaceMode = SurfaceMode::Shaded;
        focusSun(camera, selected);
        ui.showProject();
        break;
    }
    if (frame >= at(37) && frame < at(45)) {
        const WorkshopSettings defaults;
        const float progress = static_cast<float>(frame - at(37)) / static_cast<float>(at(8) - 1);
        lab.workshop.lightAzimuthDegrees = glm::mix(defaults.lightAzimuthDegrees, 115.0f, progress);
        lab.workshop.lightElevationDegrees = glm::mix(defaults.lightElevationDegrees, -24.0f, progress);
    }
}

void drawDemoCaption(int frame) {
    if (frame < 0 || frame >= demoTourFrames)
        return;
    const char* text = captions[0].text;
    for (const auto& caption : captions) {
        if (frame < caption.start)
            break;
        text = caption.text;
    }
    const auto* viewport = ImGui::GetMainViewport();
    const auto size = viewport->Size;
    const float wrap = std::clamp(size.x * .53f, 80.0f, 720.0f);
    const auto extent = ImGui::CalcTextSize(text, nullptr, false, wrap);
    const float x = viewport->Pos.x + (size.x - extent.x) * .5f;
    const float y = viewport->Pos.y + std::max(14.0f, size.y - extent.y - 131.0f);
    auto* draw = ImGui::GetForegroundDrawList();
    draw->AddRectFilled({x - 12, y - 7}, {x + extent.x + 12, y + extent.y + 7},
                        IM_COL32(10, 19, 34, 245), 7.0f);
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {x, y}, IM_COL32(246, 249, 255, 255),
                  text, nullptr, wrap);
}

void writeDemoFrame(const std::filesystem::path& directory, int frame, int width, int height) {
    if (directory.empty() || frame < 0)
        throw std::invalid_argument("Demo recording needs a directory and a non-negative frame index.");
    // JPEG uses 16-bit dimensions and the encoder indexes the RGB input with int offsets.
    if (width <= 0 || height <= 0 || width > 65535 || height > 65535 ||
        height > std::numeric_limits<int>::max() / (width * 3))
        throw std::invalid_argument("Invalid demo recording dimensions.");
    if (frame == 0)
        std::filesystem::create_directories(directory);
    char name[32];
    std::snprintf(name, sizeof(name), "frame_%04d.jpg", frame);
    const auto destination = directory / name;
    const size_t stride = static_cast<size_t>(width) * 3;
    std::vector<unsigned char> pixels(stride * static_cast<size_t>(height));

    GLint framebuffer, buffer, alignment, rowLength, skipRows, skipPixels;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &framebuffer);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &rowLength);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &skipRows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &skipPixels);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    GLint readBuffer;
    glGetIntegerv(GL_READ_BUFFER, &readBuffer);
    glReadBuffer(GL_BACK);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
    glPixelStorei(GL_PACK_ROW_LENGTH, rowLength);
    glPixelStorei(GL_PACK_SKIP_ROWS, skipRows);
    glPixelStorei(GL_PACK_SKIP_PIXELS, skipPixels);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(buffer));
    glReadBuffer(static_cast<GLenum>(readBuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(framebuffer));
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("OpenGL could not read demo frame " + std::to_string(frame) + ".");

    for (int row = 0; row < height / 2; ++row) {
        auto* top = pixels.data() + stride * static_cast<size_t>(row);
        auto* bottom = pixels.data() + stride * static_cast<size_t>(height - row - 1);
        std::swap_ranges(top, top + stride, bottom);
    }
    std::ofstream output(destination, std::ios::binary);
    if (!output)
        throw std::runtime_error("Cannot open demo frame " + destination.u8string());
    const auto writeBytes = [](void* context, void* data, int length) {
        static_cast<std::ofstream*>(context)->write(static_cast<const char*>(data), length);
    };
    const int encoded = stbi_write_jpg_to_func(writeBytes, &output, width, height, 3, pixels.data(), 90);
    output.close();
    if (!encoded || !output)
        throw std::runtime_error("Cannot write complete demo frame " + destination.u8string());
}
} // namespace sse
