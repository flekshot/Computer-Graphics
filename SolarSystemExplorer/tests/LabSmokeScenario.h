#pragma once
#include "LabState.h"
#include "SolarSystem.h"
#include "Camera.h"
#include <functional>
struct GLFWwindow;
namespace sse {
void labSmokeFrame(int frame, GLFWwindow* window, SolarSystem& system, Camera& camera, Camera& workshopCamera, RenderSettings& settings,
                   LabState& lab, int& selected, const std::function<void(int)>& select);
void labSmokeUIFrame(int frame);
void labSmokeVerifyExit(GLFWwindow* window);
}
