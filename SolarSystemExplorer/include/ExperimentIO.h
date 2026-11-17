#pragma once
#include "LabState.h"
#include "SolarSystem.h"
#include <filesystem>
#include <string>

namespace sse {
// Versioned CPU-only experiment files. Loading applies only after full validation.
bool saveExperiment(const std::filesystem::path& path, const SolarSystem& system,
                    const RenderSettings& render, const LabState& lab, int selected, std::string& error);
bool loadExperiment(const std::filesystem::path& path, SolarSystem& system,
                    RenderSettings& render, LabState& lab, int& selected, std::string& error);
} // namespace sse
