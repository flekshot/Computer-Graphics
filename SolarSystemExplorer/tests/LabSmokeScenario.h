#pragma once
#include "LabState.h"
#include "SolarSystem.h"
#include <functional>
struct GLFWwindow;
namespace sse {
void labSmokeFrame(int frame, GLFWwindow* window, SolarSystem& system, RenderSettings& settings,
                   LabState& lab, int& selected, const std::function<void(int)>& select);
}
