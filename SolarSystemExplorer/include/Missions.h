#pragma once
#include "LabState.h"
#include "SolarSystem.h"
#include <array>

namespace sse {
struct MissionInfo {
    const char* title;
    const char* description;
};
const std::array<MissionInfo, 5>& workshopMissions();
// Starting selects the relevant laboratory without changing its settings.
void startMission(int index, LabState& lab);
bool checkMission(int index, const SolarSystem& system, const RenderSettings& render, const LabState& lab, int selected);
// Only explicit successful checks award progress; previous completion is retained.
bool completeMission(int index, const SolarSystem& system, const RenderSettings& render, LabState& lab, int selected);
} // namespace sse
