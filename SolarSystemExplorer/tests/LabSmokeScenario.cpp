#include "LabSmokeScenario.h"
#include "ExperimentIO.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace sse {
namespace {
class TemporarySmokeDirectory {
  public:
    TemporarySmokeDirectory() {
        static std::atomic<unsigned long long> sequence{0};
        const auto parent = std::filesystem::absolute(std::filesystem::temp_directory_path());
        for (int attempt = 0; attempt < 32; ++attempt) {
            const auto candidate = parent / ("sselab-smoke-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
                std::to_string(sequence.fetch_add(1)));
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error)) {
                path_ = candidate;
                return;
            }
            if (error && error != std::errc::file_exists)
                throw std::system_error(error, "Lab smoke: cannot create a temporary directory");
        }
        throw std::runtime_error("Lab smoke: cannot reserve a unique temporary directory");
    }
    ~TemporarySmokeDirectory() {
        std::error_code ignored;
        // Remove only this fixture's known file and its exclusively created, now-empty directory.
        std::filesystem::remove(file(), ignored);
        std::filesystem::remove(path_, ignored);
    }
    TemporarySmokeDirectory(const TemporarySmokeDirectory&) = delete;
    TemporarySmokeDirectory& operator=(const TemporarySmokeDirectory&) = delete;
    const std::filesystem::path& path() const { return path_; }
    std::filesystem::path file() const { return path_ / "lab-smoke-roundtrip.sselab"; }
  private:
    std::filesystem::path path_;
};
class ScopedWorkingDirectory {
  public:
    explicit ScopedWorkingDirectory(const std::filesystem::path& path)
        : original_(std::filesystem::current_path()) { std::filesystem::current_path(path); }
    ~ScopedWorkingDirectory() {
        std::error_code ignored;
        std::filesystem::current_path(original_, ignored);
    }
    ScopedWorkingDirectory(const ScopedWorkingDirectory&) = delete;
    ScopedWorkingDirectory& operator=(const ScopedWorkingDirectory&) = delete;
  private:
    std::filesystem::path original_;
};
} // namespace
void labSmokeFrame(int frame, GLFWwindow* window, SolarSystem& system, Camera& camera, Camera& workshopCamera, RenderSettings& settings,
                   LabState& lab, int& selected, const std::function<void(int)>& select) {
    auto click = [&](float x, float y, bool pressed) {
        ImGui::GetIO().AddMousePosEvent(x, y);
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, pressed);
    };
    if (frame == 0) {
        system.setPaused(true);
        lab.view = LabView::Explore;
    }
    if (frame == 2) click(200, 92, true);
    if (frame == 3) click(200, 92, false);
    if (frame == 5 && (lab.view != LabView::Orbit || selected != 3))
        throw std::runtime_error("Lab smoke: Orbit Lab navigation did not select its editable planet");
    if (frame == 6) {
        auto orbit = system.planets()[3].orbitParameters();
        orbit.semiMajorAxis = 20;
        orbit.eccentricity = 1.0 - (static_cast<double>(SolarSystem::sunRadius) +
                                   system.planets()[3].data().radius + 0.45) / 20.0;
        orbit.phaseDegrees = 270;
        std::string error;
        if (!system.setOrbit(3, orbit, error))
            throw std::runtime_error("Lab smoke orbit setup: " + error);
    }
    if (frame == 7) {
        const auto& orbit = system.planets()[3].orbitParameters();
        const double expectedE = 1.0 - (static_cast<double>(SolarSystem::sunRadius) +
                                        system.planets()[3].data().radius + 0.45) / 20.0;
        if (orbit.eccentricity != expectedE || orbit.phaseDegrees != 270)
            throw std::runtime_error("Lab smoke: passive orbit UI changed valid eccentricity or phase");
    }
    if (frame == 8) {
        auto orbit = system.planets()[3].orbitParameters();
        orbit.semiMajorAxis = 34;
        orbit.eccentricity = 0.55;
        orbit.inclinationDegrees = 22;
        orbit.phaseDegrees = 0;
        std::string error;
        if (!system.setOrbit(3, orbit, error))
            throw std::runtime_error(error);
        settings.trails = settings.velocityVectors = settings.apsides = true;
    }
    if (frame == 12) {
        // A controlled working directory proves the smoke roundtrip cannot touch a user's same-named save.
        const TemporarySmokeDirectory workingDirectory;
        const ScopedWorkingDirectory useWorkingDirectory(workingDirectory.path());
        const std::string sentinel = "Existing user experiment: must remain byte-for-byte unchanged.\n";
        {
            std::ofstream output(workingDirectory.file(), std::ios::binary);
            output << sentinel;
            output.close();
            if (!output)
                throw std::runtime_error("Lab smoke: cannot create sentinel experiment");
        }
        const TemporarySmokeDirectory roundtripDirectory;
        const auto path = roundtripDirectory.file();
        std::string error;
        if (!saveExperiment(path, system, settings, lab, selected, error))
            throw std::runtime_error("Lab smoke save: " + error);
        system.resetOrbits();
        settings = RenderSettings{};
        lab = LabState{};
        if (!loadExperiment(path, system, settings, lab, selected, error))
            throw std::runtime_error("Lab smoke load: " + error);
        std::ifstream preserved(workingDirectory.file(), std::ios::binary);
        const std::string contents{std::istreambuf_iterator<char>(preserved), std::istreambuf_iterator<char>()};
        if (contents != sentinel)
            throw std::runtime_error("Lab smoke: roundtrip overwrote or deleted an existing working-directory file");
        select(selected);
        if (!system.isModified() || !settings.trails || lab.view != LabView::Orbit)
            throw std::runtime_error("Lab smoke: experiment did not restore its state");
    }
    if (frame == 14) {
        select(3);
        settings.trails = settings.velocityVectors = settings.apsides = false;
    }
    if (frame == 15) click(200, 92, true);
    if (frame == 16) click(200, 92, false);
    if (frame == 18 && (camera.followed() != 3 || settings.trails || settings.velocityVectors || settings.apsides))
        throw std::runtime_error("Lab smoke: repeated Orbit Lab selection changed the camera or overlays");
    // The left Overview button must behave like 0/R: keep the edited planet.
    if (frame == 19) click(100, 250, true);
    if (frame == 20) click(100, 250, false);
    if (frame == 22 && (selected != 3 || camera.followed() >= 0))
        throw std::runtime_error("Lab smoke: Overview lost the editable selection or kept tracking");
    if (frame == 23) settings.surfaceMode = SurfaceMode::Wireframe;
    if (frame == 30) settings.surfaceMode = SurfaceMode::Normals;
    // Drag the actual eccentricity widget to its right edge. Formatting must not
    // round its upper bound into an orbit that the model rejects.
    if (frame == 31) {
        system.resetOrbits();
        select(0);
    }
    auto eccentricityEndpoint = [&](bool pressed) {
        const auto* panel = ImGui::FindWindowByName("Object data");
        if (!panel) throw std::runtime_error("Lab smoke: orbit editor panel is missing");
        // The scrollbar reduces the content width; hit inside the slider, not its border.
        click(panel->WorkRect.Max.x - 6, 456, pressed);
    };
    if (frame == 32) eccentricityEndpoint(true);
    if (frame == 33) eccentricityEndpoint(false);
    if (frame == 34) {
        const double value = system.planets()[0].orbitParameters().eccentricity;
        if (value < .3885 || value >= .388572)
            throw std::runtime_error("Lab smoke: Mercury eccentricity endpoint was rejected or rounded beyond its limit");
    }
    if (frame == 35) select(3);
    if (frame == 36) eccentricityEndpoint(true);
    if (frame == 37) eccentricityEndpoint(false);
    if (frame == 38) {
        const double value = system.planets()[3].orbitParameters().eccentricity;
        if (value < .7599 || value >= .76)
            throw std::runtime_error("Lab smoke: Mars eccentricity endpoint was rejected or rounded beyond its limit");
    }
    if (frame == 40) {
        settings.surfaceMode = SurfaceMode::Shaded;
        settings.ambient = 0.2f;
        settings.diffuse = 1.5f;
        settings.specular = 2;
    }
    if (frame == 45) click(340, 92, true);
    if (frame == 46) click(340, 92, false);
    if (frame == 49 && lab.view != LabView::Eclipse)
        throw std::runtime_error("Lab smoke: Eclipse Lab navigation did not activate");
    if (frame == 50) lab.eclipse = EclipseSettings{};
    if (frame == 65) lab.eclipse.phaseDegrees = 8;
    if (frame == 80) lab.eclipse.phaseDegrees = 30;
    static Camera expectedWorkshopCamera;
    if (frame == 82) {
        lab.view = LabView::Workshop;
        lab.workshop.animate = false;
        workshopCamera.orbit(155, -87);
        workshopCamera.pan(.2f, -.15f, .3f);
        workshopCamera.zoom(2);
    }
    if (frame == 84 || frame == 87) {
        expectedWorkshopCamera = workshopCamera;
        expectedWorkshopCamera.update(1.0f / 60, {});
        if (frame == 84) lab.workshop.relief = .22f;
        else lab.workshop.radius = 1.8f;
    }
    if (frame == 85 || frame == 88) {
        const auto actualDirection = glm::normalize(workshopCamera.position() - workshopCamera.center());
        const auto expectedDirection = glm::normalize(expectedWorkshopCamera.position() - expectedWorkshopCamera.center());
        if (glm::length(actualDirection - expectedDirection) > .0001f ||
            glm::length(workshopCamera.center() - expectedWorkshopCamera.center()) > .0001f)
            throw std::runtime_error("Lab smoke: a terrain edit reset the Workshop viewing angle or pan");
    }
    if (frame == 92) lab.view = LabView::Eclipse;
    if (frame == 95) {
        lab.eclipse.lunar = true;
        lab.eclipse.phaseDegrees = 0;
    }
    if (frame == 110) lab.eclipse.phaseDegrees = 20;
    if (frame == 125) lab.eclipse.phaseDegrees = 35;
    if (frame == 140) {
        lab.eclipse = EclipseSettings{};
        lab.eclipse.observerView = true;
    }
    if (frame == 155) lab.eclipse.lunar = true;
    if (frame == 170) glfwSetWindowSize(window, 1000, 640);
    if (frame == 175) {
        lab.eclipse.animate = true;
        lab.eclipse.angularSpeed = 12;
    }
    if (frame == 185) {
        if (lab.eclipse.phaseDegrees <= 0)
            throw std::runtime_error("Lab smoke: eclipse animation did not advance");
        glfwSetWindowSize(window, 1000, 2600);
    }
    if (frame == 190) glfwSetWindowSize(window, 1440, 900);
    if (frame == 195) lab.view = LabView::Orbit;
    if (frame == 198 || frame == 203)
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true);
    if (frame == 199 || frame == 204) {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false);
        if (glfwWindowShouldClose(window))
            throw std::runtime_error("Lab smoke: Escape from a popup requested application exit");
    }
    if (frame == 205) {
        system.resetOrbits();
        settings = RenderSettings{};
        if (system.isModified())
            throw std::runtime_error("Lab smoke: reset left modified orbital parameters");
    }
    if (frame == 215) click(80, 92, true);
    if (frame == 216) click(80, 92, false);
    if (frame == 225 && lab.view != LabView::Explore)
        throw std::runtime_error("Lab smoke: Explore navigation did not restore the atlas");
    if (frame == 239)
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true);
}
void labSmokeUIFrame(int frame) {
    if (frame < 196 || frame > 206)
        return;
    // These fixtures use the real ImGui popup lifecycle and the application's
    // actual input policy. A normal popup closes during NewFrame on Escape;
    // a modal keeps keyboard ownership until explicitly dismissed.
    ImGui::SetNextWindowPos({400, 240});
    ImGui::SetNextWindowSize({350, 120});
    ImGui::Begin("Lab input regression", nullptr, ImGuiWindowFlags_NoSavedSettings);
    if (frame == 196) ImGui::OpenPopup("Ordinary popup");
    if (ImGui::BeginPopup("Ordinary popup")) {
        ImGui::TextUnformatted("Escape closes this popup, not the application.");
        ImGui::EndPopup();
    }
    if (frame == 197 && !ImGui::IsPopupOpen("Ordinary popup"))
        throw std::runtime_error("Lab smoke: ordinary popup fixture did not open");
    if (frame == 199 && ImGui::IsPopupOpen("Ordinary popup"))
        throw std::runtime_error("Lab smoke: Escape did not dismiss the ordinary popup");
    if (frame == 201) ImGui::OpenPopup("Modal popup");
    if (ImGui::BeginPopupModal("Modal popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextUnformatted("Escape must remain owned by this modal.");
        if (frame == 206) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (frame == 202 && !ImGui::IsPopupOpen("Modal popup"))
        throw std::runtime_error("Lab smoke: modal popup fixture did not open");
    ImGui::End();
}
void labSmokeVerifyExit(GLFWwindow* window) {
    if (!glfwWindowShouldClose(window))
        throw std::runtime_error("Lab smoke: Escape outside UI no longer exits the application");
    std::cout << "LAB SMOKE PASS: navigation, editing, persistence, graphics modes, eclipses, observer, resize, reset, eccentricity endpoints, terrain camera, popup and ordinary Escape\n";
}
} // namespace sse
