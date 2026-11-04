#include "LabSmokeScenario.h"
#include "ExperimentIO.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
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
void labSmokeFrame(int frame, GLFWwindow* window, SolarSystem& system, RenderSettings& settings,
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
    if (frame == 20) settings.surfaceMode = SurfaceMode::Wireframe;
    if (frame == 30) settings.surfaceMode = SurfaceMode::Normals;
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
        glfwSetWindowSize(window, 1440, 900);
    }
    if (frame == 195) lab.view = LabView::Orbit;
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
        std::cout << "LAB SMOKE PASS: navigation, editing, persistence, graphics modes, eclipses, observer, resize, reset\n";
}
} // namespace sse
