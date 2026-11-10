#pragma once
#include "LabState.h"
#include "SolarSystem.h"
#include <filesystem>
struct GLFWwindow;
namespace sse {
void workshopSmokeFrame(int frame, GLFWwindow* window, SolarSystem& system, RenderSettings& settings, LabState& lab,
                       const std::filesystem::path& root);
}
