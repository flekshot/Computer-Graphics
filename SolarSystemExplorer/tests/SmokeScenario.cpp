#include "SmokeScenario.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace sse {
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
