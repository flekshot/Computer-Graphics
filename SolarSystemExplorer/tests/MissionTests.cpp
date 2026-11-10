#include "Missions.h"
#include "EclipseModel.h"
#include "WorkshopModel.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
int checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
void testStarting() {
    sse::LabState lab;
    lab.workshop.seed = 734;
    lab.workshop.kind = sse::WorkshopKind::Gas;
    lab.workshop.animate = true;
    lab.eclipse.phaseDegrees = 31;
    lab.eclipse.observerView = false;
    lab.missions.completed[3] = true;
    const std::array<sse::LabView, 5> expected{sse::LabView::Workshop, sse::LabView::Workshop,
                                            sse::LabView::Workshop, sse::LabView::Orbit, sse::LabView::Eclipse};
    for (int index = 0; index < 5; ++index) {
        sse::startMission(index, lab);
        check(lab.missions.active == index && lab.view == expected[index], "Starting a mission must select its lab and active index");
        check(lab.workshop.seed == 734 && lab.workshop.kind == sse::WorkshopKind::Gas && lab.workshop.animate &&
                  lab.eclipse.phaseDegrees == 31 && !lab.eclipse.observerView,
              "Starting missions must preserve settings instead of creating the answer");
        check(lab.missions.completed[3], "Starting missions must preserve completed progress");
    }
    sse::startMission(-1, lab); sse::startMission(5, lab);
    check(lab.missions.active == 4 && lab.view == sse::LabView::Eclipse, "Invalid starts must preserve live state");
    for (const auto& mission : sse::workshopMissions())
        check(mission.title && mission.title[0] && mission.description && mission.description[0], "Every challenge must explain its objective");
}
void testWorkshopMissions() {
    sse::SolarSystem system;
    sse::RenderSettings render;
    sse::LabState lab;
    lab.view = sse::LabView::Workshop;
    lab.workshop = sse::workshopPreset(0);
    check(sse::checkMission(0, system, render, lab, -1), "A rocky ocean world with useful relief must satisfy the ocean mission");
    lab.workshop.relief = .099f;
    check(!sse::checkMission(0, system, render, lab, -1), "Ocean mission must require at least 0.10 relief");
    lab.workshop.relief = .1f;
    check(sse::checkMission(0, system, render, lab, -1), "The relief threshold must be inclusive");
    for (float sea : {0.0f, 1.0f}) {
        lab.workshop.seaLevel = sea;
        check(!sse::checkMission(0, system, render, lab, -1), "Ocean mission must reject both dry and fully submerged worlds");
    }
    lab.workshop = sse::workshopPreset(3);
    lab.workshop.gasBands = 12; lab.workshop.gasTurbulence = .5f;
    check(sse::checkMission(1, system, render, lab, -1), "Gas band and turbulence lower bounds must be inclusive");
    check(!sse::checkMission(0, system, render, lab, -1), "Gas worlds must not satisfy a rocky ocean mission");
    lab.workshop.gasBands = 11;
    check(!sse::checkMission(1, system, render, lab, -1), "The gas mission must reject too few bands");
    lab.workshop.gasBands = 12; lab.workshop.gasTurbulence = .499f;
    check(!sse::checkMission(1, system, render, lab, -1), "The gas mission must reject insufficient turbulence");
    lab.workshop.gasTurbulence = .5f; lab.workshop.kind = sse::WorkshopKind::Rocky;
    check(!sse::checkMission(1, system, render, lab, -1), "A rocky world must not satisfy a gas mission");
    render.surfaceMode = sse::SurfaceMode::Normals; lab.workshop.animate = false;
    check(sse::checkMission(2, system, render, lab, -1), "A paused workshop in normals mode must pass");
    lab.workshop.animate = true;
    check(!sse::checkMission(2, system, render, lab, -1), "An animated workshop must not pass the paused challenge");
    lab.workshop.animate = false; render.surfaceMode = sse::SurfaceMode::Wireframe;
    check(!sse::checkMission(2, system, render, lab, -1), "Wireframe must not satisfy a normals-mode challenge");
    lab.view = sse::LabView::Explore; render.surfaceMode = sse::SurfaceMode::Normals;
    check(!sse::checkMission(2, system, render, lab, -1), "The normals mission must require the workshop view");
}
void testOrbitMission() {
    sse::SolarSystem system;
    sse::RenderSettings render;
    sse::LabState lab; lab.view = sse::LabView::Orbit;
    auto orbit = system.planets()[4].orbitParameters();
    std::string error;
    for (double eccentricity : {.5, .7})
        for (double inclination : {-15.0, 15.0}) {
            orbit.eccentricity = eccentricity; orbit.inclinationDegrees = inclination;
            check(system.setOrbit(4, orbit, error), "Orbit fixture must remain physically valid");
            check(sse::checkMission(3, system, render, lab, 4), "Orbit thresholds and either inclination sign must be inclusive");
        }
    check(!sse::checkMission(3, system, render, lab, -1) && !sse::checkMission(3, system, render, lab, 8),
          "An orbit challenge requires an in-range selected planet");
    check(!sse::checkMission(3, system, render, lab, 3), "Only the selected orbit may satisfy the challenge");
    for (const auto& fixture : {std::pair<double, double>{.499, 15}, {.701, 15}, {.6, 14.99}}) {
        orbit.eccentricity = fixture.first; orbit.inclinationDegrees = fixture.second;
        check(system.setOrbit(4, orbit, error), "Rejected-predicate fixture must be a valid orbit");
        check(!sse::checkMission(3, system, render, lab, 4), "Orbit mission must reject values outside its target interval");
    }
}
void testEclipseAndCompletion() {
    sse::SolarSystem system;
    sse::RenderSettings render;
    sse::LabState lab; lab.view = sse::LabView::Eclipse; lab.eclipse.observerView = true;
    for (bool lunar : {false, true}) {
        lab.eclipse.lunar = lunar;
        lab.eclipse.phaseDegrees = lunar ? 20.0f : 8.0f;
        const double visible = sse::eclipseCenterVisibility(sse::makeEclipseScene(lab.eclipse));
        check(visible > .2 && visible < .8, "Partial-eclipse fixture must meet the target-center visibility interval");
        check(sse::checkMission(4, system, render, lab, -1), "Solar and lunar partial eclipses must satisfy the observer mission");
        check(!lab.missions.completed[4], "Checking must never complete progress implicitly");
        lab.eclipse.phaseDegrees = 0;
        check(!sse::checkMission(4, system, render, lab, -1), "A total eclipse must not pass the partial-eclipse challenge");
        lab.eclipse.phaseDegrees = 90;
        check(!sse::checkMission(4, system, render, lab, -1), "Full sunlight must not pass the partial-eclipse challenge");
    }
    lab.eclipse.lunar = false; lab.eclipse.phaseDegrees = 8; lab.eclipse.observerView = false;
    check(!sse::completeMission(4, system, render, lab, -1) && !lab.missions.completed[4],
          "Failure must neither award progress nor bypass observer view");
    lab.eclipse.observerView = true; lab.missions.active = 4;
    check(sse::completeMission(4, system, render, lab, -1) && lab.missions.completed[4],
          "An explicit successful check must save completion");
    lab.eclipse.phaseDegrees = 90;
    check(!sse::completeMission(4, system, render, lab, -1) && lab.missions.completed[4],
          "A later failed check must preserve earned progress");
    for (int index : {-1, 5, 100})
        check(!sse::checkMission(index, system, render, lab, -1) &&
                  !sse::completeMission(index, system, render, lab, -1),
              "Invalid mission checks must fail without indexing progress");
}
}
int main() {
    try {
        testStarting(); testWorkshopMissions(); testOrbitMission(); testEclipseAndCompletion();
        std::cout << "PASS: " << checks << " mission assertions (objectives, thresholds, selection, explicit sticky progress)\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
        return 1;
    }
}
