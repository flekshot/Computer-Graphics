#pragma once
#include "Renderer.h"
#include <functional>
struct GLFWwindow;
namespace sse {
void smokeFrame(int frame, GLFWwindow* window, SolarSystem& system, Camera& camera, Renderer& renderer,
                RenderSettings& settings, const std::function<void(int)>& select);
}
