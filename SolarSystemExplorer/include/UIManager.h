#pragma once
#include "Renderer.h"
#include <filesystem>
struct GLFWwindow;
struct ImFont;
namespace sse {
struct UIActions {
    int select = -2;
    bool reset = false, refocus = false;
};
class UIManager {
  public:
    UIManager(GLFWwindow* window, const std::filesystem::path& root);
    ~UIManager();
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    void begin();
    UIActions draw(SolarSystem& system, const Camera& camera, RenderSettings& settings, int selected);
    void labels(const SolarSystem& system, const Camera& camera, int selected) const;
    void finish(bool visible = true);
    void toggleHelp() { help_ = !help_; }

  private:
    void helpWindow();
    ImFont *body_ = nullptr, *heading_ = nullptr, *hero_ = nullptr;
    bool glfwReady_ = false, glReady_ = false, help_ = false;
};
} // namespace sse
