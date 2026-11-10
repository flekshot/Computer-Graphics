#pragma once
#include "LabState.h"
#include <filesystem>
#include <optional>
struct GLFWwindow;
namespace sse {
struct Options {
    int frames = 0, planet = -1, width = 1440, height = 900;
    double days = 0;
    float speed = 1;
    bool paused = false, noUI = false, noBloom = false, hidden = false, fallback = false, smoke = false,
         benchmark = false, labSmoke = false, workshopSmoke = false;
    std::optional<LabView> lab;
    std::optional<SurfaceMode> surfaceMode;
    std::optional<bool> lunar, observer;
    std::optional<float> eclipsePhase, eclipseInclination;
    std::optional<WorkshopKind> workshopKind;
    std::optional<uint32_t> seed;
    std::filesystem::path root, capture, experiment;
};
class Application {
  public:
    explicit Application(Options options);
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    int run();

  private:
    Options options_;
    GLFWwindow* window_ = nullptr;
};
std::filesystem::path assetRoot(const char* argv0);
} // namespace sse
