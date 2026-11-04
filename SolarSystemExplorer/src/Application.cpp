#include "Application.h"
#include "UIManager.h"
#include "EclipseRenderer.h"
#include "ExperimentDialogs.h"
#include "ExperimentIO.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef SSE_WITH_TESTS
#include "SmokeScenario.h"
#include "LabSmokeScenario.h"
#endif
namespace sse {
std::filesystem::path assetRoot(const char* argv0) {
    std::filesystem::path executable;
#ifdef _WIN32
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length > 0 && length < buffer.size()) {
        buffer.resize(length);
        executable = buffer;
    }
#endif
    if (executable.empty())
        executable = std::filesystem::absolute(std::filesystem::u8path(argv0));
    for (const auto& p :
         {executable.parent_path(), std::filesystem::current_path(), std::filesystem::u8path(SSE_SOURCE_DIR)})
        if (std::filesystem::is_directory(p / "shaders"))
            return p;
    throw std::runtime_error("Cannot find the shaders directory. Keep shaders/ and assets/ beside the "
                             "executable, or use --assets PATH.");
}
Application::Application(Options options) : options_(std::move(options)) {
    glfwSetErrorCallback(
        [](int code, const char* message) { std::cerr << "GLFW " << code << ": " << message << '\n'; });
    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed.");
    try {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_VISIBLE, options_.hidden ? GLFW_FALSE : GLFW_TRUE);
        window_ = glfwCreateWindow(options_.width, options_.height,
                                   "Solar System Explorer | Interactive 3D Atlas", nullptr, nullptr);
        if (!window_)
            throw std::runtime_error(
                "OpenGL 3.3 context creation failed. Install the graphics driver and try again.");
        glfwSetWindowSizeLimits(window_, 1000, 640, GLFW_DONT_CARE, GLFW_DONT_CARE);
        glfwMakeContextCurrent(window_);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
            throw std::runtime_error("GLEW initialization failed.");
        while (glGetError() != GL_NO_ERROR) {
        } // GLEW's initialization may probe deprecated enums.
        if (!GLEW_VERSION_3_3)
            throw std::runtime_error("OpenGL 3.3 or later is required.");
        glfwSwapInterval(options_.benchmark || options_.hidden ? 0 : 1);
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\nRenderer: " << glGetString(GL_RENDERER)
                  << "\nAssets: " << options_.root.u8string() << '\n';
    } catch (...) {
        if (window_)
            glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
        throw;
    }
}
Application::~Application() {
    if (window_)
        glfwDestroyWindow(window_);
    glfwTerminate();
}
int Application::run() {
    // Stack order destroys UI/Renderer (all GPU handles) before the context.
    SolarSystem system;
    system.setDays(options_.days);
    system.setSpeed(options_.speed);
    system.setPaused(options_.paused);
    Camera camera;
    RenderSettings settings;
    settings.bloom = !options_.noBloom;
    LabState lab;
    Renderer renderer(options_.root, system, options_.fallback);
    std::unique_ptr<EclipseRenderer> eclipseRenderer;
    UIManager ui(window_, options_.root);
    int selected = -1, frame = 0, errorCount = 0;
    auto select = [&](int index) {
        selected = index;
        if (index < 0)
            camera.overview();
        else {
            const auto& p = system.planets().at(static_cast<size_t>(index));
            camera.focus(index, p.position(system.days()), p.data().radius, index == 5);
        }
    };
    if (!options_.experiment.empty()) {
        std::string error;
        if (!loadExperiment(options_.experiment, system, settings, lab, selected, error))
            throw std::runtime_error("Cannot load experiment: " + error);
        select(selected);
    }
    if (options_.lab) lab.view = *options_.lab;
    if (options_.surfaceMode) settings.surfaceMode = *options_.surfaceMode;
    if (options_.lunar) lab.eclipse.lunar = *options_.lunar;
    if (options_.observer) lab.eclipse.observerView = *options_.observer;
    if (options_.eclipsePhase) lab.eclipse.phaseDegrees = *options_.eclipsePhase;
    if (options_.eclipseInclination) lab.eclipse.inclinationDegrees = *options_.eclipseInclination;
    if (options_.paused) {
        system.setPaused(true);
        lab.eclipse.animate = false;
    }
    if (options_.noBloom) settings.bloom = false;
    if (lab.view == LabView::Orbit && options_.experiment.empty()) {
        if (selected < 0) {
            select(3);
            camera.overview();
        }
        settings.trails = settings.velocityVectors = settings.apsides = true;
    }
    if (options_.planet >= 0)
        select(options_.planet);
    double previous = glfwGetTime();
    const auto start = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(window_) && (options_.frames == 0 || frame < options_.frames)) {
        glfwPollEvents();
        double now = glfwGetTime();
        const float realDelta = static_cast<float>(std::clamp(now - previous, 0.0, 0.1));
        previous = now;
        const float dt = options_.frames > 0 ? 1.0f / 60.0f : realDelta;
#ifdef SSE_WITH_TESTS
        if (options_.smoke)
            smokeFrame(frame, window_, system, camera, renderer, settings, select);
        if (options_.labSmoke)
            labSmokeFrame(frame, window_, system, settings, lab, selected, select);
#endif
        int width = 0, height = 0;
        glfwGetFramebufferSize(window_, &width, &height);
        if (width <= 0 || height <= 0) {
            glfwWaitEventsTimeout(0.05);
            continue;
        }
        renderer.resize(width, height);
        ui.begin();
        float sceneRadius = 60;
        if (system.isModified())
            for (const auto& planet : system.planets())
                sceneRadius = std::max(sceneRadius, static_cast<float>(planet.apoapsis()) + planet.data().radius);
        camera.setViewport(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, !options_.noUI, sceneRadius);
        // Text widgets may clear ActiveId while consuming Escape during draw().
        const bool escapeOwnedByUI = ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive();
        UIActions action;
        if (!options_.noUI)
            action = ui.draw(system, camera, settings, selected, lab);
        if (action.select != -2)
            select(action.select);
        if (action.reset) {
            if (lab.view == LabView::Orbit) camera.overview();
            else select(-1);
        }
        if (action.refocus && selected >= 0)
            select(selected);
        if (action.resetExperiment) {
            system = SolarSystem{};
            system.setPaused(true);
            settings = RenderSettings{};
            lab.eclipse = EclipseSettings{};
            if (lab.view == LabView::Orbit) {
                select(selected < 0 ? 3 : selected);
                settings.trails = settings.velocityVectors = settings.apsides = true;
                camera.overview();
            } else
                select(-1);
            ui.setLabMessage("Original orbits, time, lighting, and eclipse settings restored.");
        }
        if (action.saveExperiment || action.loadExperiment) {
            try {
                const auto file = chooseExperimentFile(window_, action.saveExperiment);
                if (!file.empty()) {
                    std::string error;
                    const bool ok = action.saveExperiment
                        ? saveExperiment(file, system, settings, lab, selected, error)
                        : loadExperiment(file, system, settings, lab, selected, error);
                    if (ok) {
                        if (action.loadExperiment)
                            select(selected);
                        ui.setLabMessage(std::string(action.saveExperiment ? "Saved: " : "Loaded: ") +
                                         file.filename().u8string());
                    } else
                        ui.setLabMessage(error, true);
                }
            } catch (const std::exception& error) {
                ui.setLabMessage(error.what(), true);
            }
            previous = glfwGetTime();
        }
        const auto& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard && !ImGui::IsAnyItemActive()) {
            if (lab.view == LabView::Eclipse) {
                if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
                    lab.eclipse.animate = !lab.eclipse.animate;
            } else {
            for (int i = 0; i < 8; ++i)
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i), false))
                    select(i);
            if (ImGui::IsKeyPressed(ImGuiKey_0, false) || ImGui::IsKeyPressed(ImGuiKey_R, false))
                if (lab.view == LabView::Orbit) camera.overview();
                else select(-1);
            if (ImGui::IsKeyPressed(ImGuiKey_F, false) && selected >= 0)
                select(selected);
            if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
                system.setPaused(!system.paused());
            if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd))
                system.setSpeed(system.speed() * 1.4f);
            if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
                system.setSpeed(system.speed() / 1.4f);
            float right = float(glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) -
                          float(glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS);
            float forward = float(glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) -
                            float(glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS);
            camera.pan(right, forward, dt);
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_H, false) && !io.WantTextInput)
            ui.toggleHelp();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !escapeOwnedByUI)
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        if (!io.WantCaptureMouse && lab.view != LabView::Eclipse) {
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2))
                camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
            if (io.MouseWheel != 0)
                camera.zoom(io.MouseWheel);
        }
        if (lab.view == LabView::Eclipse) {
            if (lab.eclipse.animate)
                lab.eclipse.phaseDegrees = std::remainder(lab.eclipse.phaseDegrees + dt * lab.eclipse.angularSpeed, 360.0f);
        } else
            system.update(dt);
        camera.update(dt, camera.followed() >= 0 ? system.planets()[camera.followed()].position(system.days())
                                                 : glm::vec3(0));
        camera.avoidSphere({0, 0, 0}, SolarSystem::sunRadius);
        for (const auto& p : system.planets())
            camera.avoidSphere(p.position(system.days()), p.data().radius);
        camera.avoidSphere(system.moonPosition(system.days()), SolarSystem::moonRadius);
        if (lab.view == LabView::Eclipse) {
            if (!eclipseRenderer)
                eclipseRenderer = std::make_unique<EclipseRenderer>(options_.root);
            EclipseViewport area{0, 0, width, height};
            if (!options_.noUI) {
                const auto display = io.DisplaySize;
                const float scaleX = static_cast<float>(width) / display.x;
                const float scaleY = static_cast<float>(height) / display.y;
                const float left = (display.x < 1200 ? 202.0f : 222.0f) + 32;
                const float right = (display.x < 1200 ? 248.0f : 284.0f) + 32;
                area = {static_cast<int>(left * scaleX), static_cast<int>(112 * scaleY),
                        static_cast<int>((display.x - left - right) * scaleX),
                        static_cast<int>((display.y - 244) * scaleY)};
            }
            eclipseRenderer->render(lab.eclipse, settings, area, width, height);
            if (!options_.noUI)
                ui.eclipseLabels(lab);
        } else {
            renderer.render(system, camera, settings, selected, static_cast<float>(now));
            if (settings.labels && !options_.noUI)
                ui.labels(system, camera, selected);
        }
        ui.finish(!options_.noUI);
        if (!options_.capture.empty() && frame + 1 == options_.frames)
            renderer.capture(options_.capture, width, height);
        for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError()) {
            std::cerr << "OpenGL error 0x" << std::hex << error << std::dec << " at frame " << frame << '\n';
            ++errorCount;
        }
        if (errorCount > 0)
            throw std::runtime_error("OpenGL reported an error; see diagnostics above.");
        glfwSwapBuffers(window_);
        ++frame;
    }
    glFinish();
    if ((options_.smoke || options_.labSmoke) && frame != options_.frames)
        throw std::runtime_error("Smoke scenario exited before all assertions ran");
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "Completed " << frame << " frames in " << elapsed << " s; average "
              << (elapsed > 0 ? frame / elapsed : 0) << " FPS; GL errors: " << errorCount << '\n';
    if (options_.smoke)
        std::cout << "SMOKE PASS: focus all planets, fast tracking, pause/speed, resize, zero-size guard, "
                     "visibility, overview\n";
    return 0;
}
} // namespace sse
