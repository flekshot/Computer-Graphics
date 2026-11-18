#pragma once
#include <filesystem>

namespace sse {
class SolarSystem;
class Camera;
class UIManager;
struct RenderSettings;
struct LabState;

inline constexpr int demoTourFps = 30;
inline constexpr int demoTourFrames = 60 * demoTourFps;

void advanceDemoTour(int frame, SolarSystem& system, Camera& camera, Camera& workshopCamera,
                     RenderSettings& settings, LabState& lab, int& selected, UIManager& ui);
void drawDemoCaption(int frame);
// Capture the back buffer after the UI is drawn and before swapping buffers.
void writeDemoFrame(const std::filesystem::path& directory, int frame, int width, int height);
} // namespace sse
