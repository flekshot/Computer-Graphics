#include "Missions.h"
#include "EclipseModel.h"
#include "WorkshopModel.h"
#include <cmath>
#include <stdexcept>

namespace sse {
const std::array<MissionInfo, 5>& workshopMissions() {
    static const std::array<MissionInfo, 5> missions{{
        {"Ocean world", "Create a rocky world with 45-75% ocean coverage and relief of at least 0.10."},
        {"Storm bands", "Create a gas giant with at least 12 bands and turbulence of at least 0.50."},
        {"Read the normals", "Pause your workshop world's rotation and select the Normals surface mode."},
        {"An unusual orbit", "Select an orbit with eccentricity 0.50-0.70 and absolute inclination of at least 15 degrees."},
        {"Partial sunlight", "Use the eclipse observer with 20-80% of the Sun visible at the target center."}}};
    return missions;
}

void startMission(int index, LabState& lab) {
    if (index < 0 || index >= int(workshopMissions().size()))
        return;
    lab.missions.active = index;
    lab.view = index < 3 ? LabView::Workshop : index == 3 ? LabView::Orbit : LabView::Eclipse;
}

bool checkMission(int index, const SolarSystem& system, const RenderSettings& render, const LabState& lab, int selected) {
    switch (index) {
    case 0: {
        if (lab.view != LabView::Workshop || lab.workshop.kind != WorkshopKind::Rocky || lab.workshop.relief < .10f)
            return false;
        try {
            const float coverage = workshopOceanCoverage(lab.workshop);
            return coverage >= .45f && coverage <= .75f;
        } catch (const std::invalid_argument&) {
            return false;
        }
    }
    case 1:
        return lab.view == LabView::Workshop && lab.workshop.kind == WorkshopKind::Gas &&
               lab.workshop.gasBands >= 12 && lab.workshop.gasTurbulence >= .50f;
    case 2:
        return lab.view == LabView::Workshop && !lab.workshop.animate && render.surfaceMode == SurfaceMode::Normals;
    case 3: {
        if (lab.view != LabView::Orbit || selected < 0 || size_t(selected) >= system.planets().size())
            return false;
        const auto& orbit = system.planets()[size_t(selected)].orbitParameters();
        return orbit.eccentricity >= .50 && orbit.eccentricity <= .70 && std::abs(orbit.inclinationDegrees) >= 15.0;
    }
    case 4: {
        if (lab.view != LabView::Eclipse || !lab.eclipse.observerView ||
            !std::isfinite(lab.eclipse.phaseDegrees) || !std::isfinite(lab.eclipse.inclinationDegrees))
            return false;
        const double visible = eclipseCenterVisibility(makeEclipseScene(lab.eclipse));
        return visible >= .20 && visible <= .80;
    }
    default:
        return false;
    }
}

bool completeMission(int index, const SolarSystem& system, const RenderSettings& render, LabState& lab, int selected) {
    if (!checkMission(index, system, render, lab, selected))
        return false;
    lab.missions.completed[size_t(index)] = true;
    return true;
}
} // namespace sse
