#include "WorkshopSmokeScenario.h"
#include "ExperimentIO.h"
#include "Missions.h"
#include "WorkshopModel.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace sse {
void workshopSmokeFrame(int frame, GLFWwindow* window, SolarSystem& system, RenderSettings& settings, LabState& lab) {
    static float animatedAngle = 0, pausedAngle = 0;
    static ImVec2 pausedMarker;
    auto click = [](float x, float y, bool pressed) {
        ImGui::GetIO().AddMousePosEvent(x, y);
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, pressed);
    };
    if (frame == 0) {
        system.setDays(12);
        system.setPaused(true);
        lab = LabState{};
    }
    if (frame == 2) click(480, 92, true);
    if (frame == 3) click(480, 92, false);
    if (frame == 5) {
        if (lab.view != LabView::Workshop)
            throw std::runtime_error("Workshop smoke: navigation did not open the preview");
        animatedAngle = lab.workshop.rotationDegrees;
    }
    if (frame == 15 && (lab.workshop.rotationDegrees <= animatedAngle || system.days() != 12))
        throw std::runtime_error("Workshop smoke: preview clock must run independently of planetary time");
    if (frame == 16) ImGui::GetIO().AddKeyEvent(ImGuiKey_Space, true);
    if (frame == 17) ImGui::GetIO().AddKeyEvent(ImGuiKey_Space, false);
    if (frame == 19) {
        if (lab.workshop.animate)
            throw std::runtime_error("Workshop smoke: Space did not pause rotation");
        pausedAngle = lab.workshop.rotationDegrees;
    }
    if (frame == 23 && lab.workshop.rotationDegrees != pausedAngle)
        throw std::runtime_error("Workshop smoke: paused world rotated");
    if (frame == 25) lab.workshop = workshopPreset(0);
    if (frame == 45) lab.workshop = workshopPreset(3);
    if (frame == 65) settings.surfaceMode = SurfaceMode::Normals;
    if (frame == 85) {
        settings.surfaceMode = SurfaceMode::Wireframe;
        lab.workshop = workshopPreset(1);
    }
    if (frame == 105) {
        lab.workshop = workshopPreset(2);
        settings.surfaceMode = SurfaceMode::Shaded;
        glfwSetWindowSize(window, 1000, 640);
    }
    if (frame == 125) {
        lab.workshop.seed = 999999;
        lab.workshop.radius = 2.5f;
        lab.workshop.relief = .25f;
        lab.workshop.animate = false;
    }
    if (frame == 140) {
        startMission(2, lab);
        settings.surfaceMode = SurfaceMode::Normals;
        if (!completeMission(2, system, settings, lab, -1) || !lab.missions.completed[2])
            throw std::runtime_error("Workshop smoke: explicit challenge check failed");
        const auto folder = std::filesystem::temp_directory_path() /
            ("sse-workshop-smoke-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        if (!std::filesystem::create_directory(folder))
            throw std::runtime_error("Workshop smoke: cannot reserve temporary directory");
        struct Cleanup {
            std::filesystem::path directory;
            ~Cleanup() {
                std::error_code ignored;
                std::filesystem::remove(directory / "roundtrip.sselab", ignored);
                std::filesystem::remove(directory, ignored);
            }
        } cleanup{folder};
        std::string error;
        int selected = -1;
        if (!saveExperiment(folder / "roundtrip.sselab", system, settings, lab, selected, error))
            throw std::runtime_error("Workshop smoke save: " + error);
        lab = LabState{};
        if (!loadExperiment(folder / "roundtrip.sselab", system, settings, lab, selected, error) ||
            lab.workshop.seed != 999999 || !lab.missions.completed[2] || lab.view != LabView::Workshop)
            throw std::runtime_error("Workshop smoke: full preview state did not roundtrip: " + error);
    }
    if (frame == 155) {
        lab.view = LabView::Eclipse;
        lab.eclipse.lunar = true;
        lab.eclipse.observerView = true;
        glfwSetWindowSize(window, 1000, 2600);
    }
    if (frame == 165) {
        lab.view = LabView::Explore;
        glfwSetWindowSize(window, 1440, 900);
    }
    if (frame == 175) {
        lab.view = LabView::Workshop;
        settings.surfaceMode = SurfaceMode::Shaded;
        lab.workshop = workshopPreset(0);
    }
    // Exercise the real reduced-motion checkbox while preserving the simulation.
    if (frame == 185) click(1195, 39, true);
    if (frame == 186) click(1195, 39, false);
    if (frame == 190 || frame == 200) {
        const auto* header = ImGui::FindWindowByName("Top");
        if (!header)
            throw std::runtime_error("Workshop smoke: UI header is missing");
        ImVec2 marker{};
        int vertices = 0;
        for (const auto& vertex : header->DrawList->VtxBuffer)
            if (vertex.col == IM_COL32(102, 218, 209, 255) && vertex.pos.x > 24 && vertex.pos.x < 80 &&
                vertex.pos.y > 24 && vertex.pos.y < 58) {
                marker.x += vertex.pos.x;
                marker.y += vertex.pos.y;
                ++vertices;
            }
        if (vertices == 0)
            throw std::runtime_error("Workshop smoke: orbital emblem is missing");
        marker.x /= static_cast<float>(vertices);
        marker.y /= static_cast<float>(vertices);
        if (frame == 190)
            pausedMarker = marker;
        else if (std::abs(marker.x - pausedMarker.x) + std::abs(marker.y - pausedMarker.y) > .001f)
            throw std::runtime_error("Workshop smoke: Motion off did not stop decorative animation");
    }
    if (frame == 205) {
        lab.workshop.seed = 71;
        lab.workshop.seaLevel = .7f;
    }
    if (frame == 239)
        std::cout << "WORKSHOP SMOKE PASS: navigation, independent clock, pause, terrain, gas, normals, wireframe, persistence, challenges, resize, renderer switching\n";
}
} // namespace sse
