#pragma once
#include "LabState.h"
#include "SolarSystem.h"
struct GLFWwindow;
namespace sse {
void workshopSmokeFrame(int frame, GLFWwindow* window, SolarSystem& system, RenderSettings& settings, LabState& lab);
}
