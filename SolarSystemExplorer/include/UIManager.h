#pragma once
#include "Renderer.h"
#include "LabState.h"
#include <filesystem>
#include <array>
#include <string>
struct GLFWwindow;
struct ImFont;
namespace sse {
struct UIActions {
    int select = -2;
    bool reset = false, refocus = false;
    bool saveExperiment = false, loadExperiment = false, resetExperiment = false;
};
class UIManager {
  public:
    UIManager(GLFWwindow* window, const std::filesystem::path& root);
    ~UIManager();
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    void begin();
    UIActions draw(SolarSystem& system, const Camera& camera, RenderSettings& settings, int selected,
                   LabState& lab);
    void labels(const SolarSystem& system, const Camera& camera, int selected) const;
    void eclipseLabels(const LabState& lab) const;
    void setLabMessage(std::string message, bool error = false);
    void finish(bool visible = true);
    void toggleHelp() { help_ = !help_; }

  private:
    void helpWindow();
    void orbitPanel(SolarSystem& system, RenderSettings& settings, int selected, UIActions& actions);
    void orbitEditor(SolarSystem& system, RenderSettings& settings, int selected, UIActions& actions);
    void eclipseControls(EclipseSettings& settings);
    void eclipseInfo(const EclipseSettings& settings, RenderSettings& render);
    void graphicsControls(RenderSettings& settings);
    void workshopControls(LabState& lab, RenderSettings& settings);
    void missionControls(SolarSystem& system, RenderSettings& settings, LabState& lab, int selected);
    void labMessage();
    ImFont *body_ = nullptr, *heading_ = nullptr, *hero_ = nullptr;
    bool glfwReady_ = false, glReady_ = false, help_ = false;
    std::string labMessage_;
    bool labMessageError_ = false;
    double labMessageUntil_ = 0;
    bool motion_ = true;
    float navigationPosition_ = 0;
    float reveal_ = 1;
    float ambientPhase_ = 0;
    int previousView_ = -1, previousSelection_ = -2;
    std::array<float, 8> destinationGlow_{};
};
} // namespace sse
