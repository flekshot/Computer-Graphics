#pragma once
#include <filesystem>
struct GLFWwindow;
namespace sse {
// Empty path means the user cancelled. Errors are reported as exceptions.
std::filesystem::path chooseExperimentFile(GLFWwindow* window, bool save);
}
