#include "UIManager.h"
#include "EclipseModel.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <utility>

namespace sse {
namespace {
const ImVec4 muted(0.48f, 0.57f, 0.68f, 1), cyan(0.45f, 0.80f, 0.94f, 1),
    gold(0.98f, 0.73f, 0.39f, 1);
void section(const char* title) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(cyan, "%s", title);
}
bool slider(const char* title, const char* id, double& value, double minimum, double maximum,
            const char* format) {
    ImGui::TextUnformatted(title);
    ImGui::SetNextItemWidth(-1);
    return ImGui::SliderScalar(id, ImGuiDataType_Double, &value, &minimum, &maximum, format,
                              ImGuiSliderFlags_AlwaysClamp);
}
void hint(const char* message) {
    ImGui::PushTextWrapPos(0);
    ImGui::TextColored(muted, "%s", message);
    ImGui::PopTextWrapPos();
}
} // namespace

void UIManager::graphicsControls(RenderSettings& settings) {
    section("GRAPHICS LAB");
    int surface = static_cast<int>(settings.surfaceMode);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##surface", &surface, "Shaded\0Wireframe\0World normals\0"))
        settings.surfaceMode = static_cast<SurfaceMode>(surface);
    if (settings.surfaceMode == SurfaceMode::Normals)
        hint("RGB encodes world-space X/Y/Z normals. Short lines show surface normal directions.");
    else if (settings.surfaceMode == SurfaceMode::Wireframe)
        hint("Triangle edges reveal the indexed sphere mesh.");
    else
        hint("Change each lighting term independently.");
    ImGui::BeginDisabled(settings.surfaceMode != SurfaceMode::Shaded);
    ImGui::TextUnformatted("Ambient");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##ambient", &settings.ambient, 0, 3, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextUnformatted("Diffuse");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##diffuse", &settings.diffuse, 0, 3, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextUnformatted("Specular");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##specular", &settings.specular, 0, 3, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::EndDisabled();
    if (ImGui::Button("Reset lighting", {-1, 28}))
        settings.ambient = settings.diffuse = settings.specular = 1;
}

void UIManager::orbitPanel(SolarSystem& system, RenderSettings& settings, int selected, UIActions& actions) {
    ImGui::TextColored(cyan, "INTERACTIVE LABORATORY");
    if (selected >= 0 && static_cast<size_t>(selected) < system.planets().size()) {
        ImGui::PushFont(heading_);
        ImGui::TextUnformatted(system.planets()[static_cast<size_t>(selected)].data().name.c_str());
        ImGui::PopFont();
        if (ImGui::Button("Focus planet  [F]", {-1, 28}))
            actions.refocus = true;
    }
    if (ImGui::BeginTabBar("Laboratory tools")) {
        if (ImGui::BeginTabItem("Orbit")) {
            orbitEditor(system, settings, selected, actions);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Graphics")) {
            graphicsControls(settings);
            ImGui::Spacing();
            ImGui::Checkbox("Bloom", &settings.bloom);
            ImGui::TextUnformatted("Exposure");
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("##exposure", &settings.exposure, 0.2f, 3, "%.2f",
                              ImGuiSliderFlags_AlwaysClamp);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
void UIManager::orbitEditor(SolarSystem& system, RenderSettings& settings, int selected, UIActions& actions) {
    ImGui::TextColored(cyan, "ORBIT LABORATORY");
    if (selected < 0 || static_cast<size_t>(selected) >= system.planets().size()) {
        hint("Choose a planet on the left to edit its orbit.");
        return;
    }
    const auto& planet = system.planets()[static_cast<size_t>(selected)];
    hint("Edit an illustrative orbit. All distances below are scene units.");
    auto parameters = planet.orbitParameters();
    parameters.phaseDegrees = std::remainder(parameters.phaseDegrees, 360.0);
    const bool axisChanged = slider("Semi-major axis", "##axis", parameters.semiMajorAxis, 6, 180, "%.2f");
    bool changed = axisChanged;
    // The model requires strictly more than 0.4 units; keep a tiny numerical margin for edits.
    const double clearance = static_cast<double>(SolarSystem::sunRadius) + planet.data().radius + 0.400001;
    const double maxE = std::clamp(1.0 - clearance / parameters.semiMajorAxis, 0.0, 0.85);
    if (axisChanged && parameters.eccentricity > maxE) {
        parameters.eccentricity = maxE;
    }
    changed |= slider("Eccentricity", "##eccentricity", parameters.eccentricity, 0, maxE, "%.3f");
    changed |= slider("Inclination", "##inclination", parameters.inclinationDegrees, -60, 60, "%.1f deg");
    changed |= slider("Orbital period", "##period", parameters.periodDays, 5, 200000, "%.1f days");
    changed |= slider("Initial phase", "##phase", parameters.phaseDegrees, -180, 180, "%.1f deg");
    if (changed) {
        std::string error;
        if (!system.setOrbit(static_cast<size_t>(selected), parameters, error))
            setLabMessage(error, true);
    }
    hint("Periapsis stays outside the Sun. Size and period are independently editable.");
    if (ImGui::Button("Try elliptical orbit", {-1, 30})) {
        parameters.eccentricity = std::min(0.55, maxE);
        parameters.inclinationDegrees = 22;
        parameters.phaseDegrees = 0;
        std::string error;
        if (system.setOrbit(static_cast<size_t>(selected), parameters, error)) {
            system.setDays(0);
            system.setPaused(true);
            settings.trails = settings.velocityVectors = settings.apsides = true;
            actions.reset = true;
            setLabMessage("Elliptical orbit ready. Resume time or focus the planet with F.");
        } else
            setLabMessage(error, true);
    }
    const auto current = planet.orbitParameters();
    ImGui::TextColored(gold, "Periapsis  %.2f", current.semiMajorAxis * (1 - current.eccentricity));
    ImGui::TextColored(gold, "Apoapsis   %.2f", current.semiMajorAxis * (1 + current.eccentricity));
    ImGui::Text("Speed  %.4f units/day", glm::length(planet.velocity(system.days())));
    section("SIMULATION TIME");
    double day = system.days();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputDouble("##seek-day", &day, 0, 0, "%.2f days", ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (std::isfinite(day) && day >= 0 && day <= 1e10) {
            system.setDays(day);
            system.setPaused(true);
        } else
            setLabMessage("Simulation day must be between 0 and 10 billion.", true);
    }
    hint("Enter a simulation day and press Enter to seek and pause.");
    section("ORBIT OVERLAYS");
    ImGui::Checkbox("Recent trajectory", &settings.trails);
    ImGui::Checkbox("Velocity vector", &settings.velocityVectors);
    ImGui::Checkbox("Periapsis / apoapsis", &settings.apsides);
    hint("Velocity arrow length is scaled for display. Green: periapsis. Orange: apoapsis.");
}

void UIManager::eclipseControls(EclipseSettings& settings) {
    ImGui::TextColored(cyan, "ECLIPSE LABORATORY");
    int kind = settings.lunar ? 1 : 0;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##eclipse-kind", &kind, "Solar eclipse\0Lunar eclipse\0")) {
        settings.lunar = kind == 1;
        settings.phaseDegrees = settings.inclinationDegrees = 0;
        settings.animate = false;
    }
    hint(settings.lunar ? "Earth moves between sunlight and the Moon."
                        : "The Moon moves between sunlight and Earth.");
    section("ALIGNMENT");
    ImGui::TextUnformatted("Phase offset");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##eclipse-phase", &settings.phaseDegrees, -180, 180, "%.1f deg",
                      ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextUnformatted("Orbit inclination");
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##eclipse-inclination", &settings.inclinationDegrees, -30, 30, "%.1f deg",
                      ImGuiSliderFlags_AlwaysClamp);
    section("PRESET EXPERIMENTS");
    auto preset = [&](const char* title, float phase) {
        if (ImGui::Button(title, {-1, 30})) {
            settings.phaseDegrees = phase;
            settings.inclinationDegrees = 0;
            settings.animate = false;
        }
    };
    preset("Total alignment", 0);
    preset("Partial alignment", settings.lunar ? 20.0f : 8.0f);
    preset("Clear of shadow", settings.lunar ? 35.0f : 30.0f);
    section("VIEWS");
    ImGui::Checkbox("Shadow cones", &settings.showCones);
    ImGui::Checkbox("Observer viewpoint", &settings.observerView);
    hint("Top: geometry of the event. Bottom: the target surface or an observer's view.");
}

void UIManager::eclipseInfo(const EclipseSettings& settings, RenderSettings& render) {
    ImGui::TextColored(cyan, "LIGHT AND SHADOW");
    ImGui::PushFont(heading_);
    ImGui::TextUnformatted(settings.lunar ? "Lunar eclipse" : "Solar eclipse");
    ImGui::PopFont();
    int surface = static_cast<int>(render.surfaceMode);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##eclipse-surface", &surface, "Shaded\0Wireframe\0World normals\0"))
        render.surfaceMode = static_cast<SurfaceMode>(surface);
    if (render.surfaceMode != SurfaceMode::Shaded)
        hint("Select Shaded to observe the eclipse shadow.");
    const auto scene = makeEclipseScene(settings);
    section("TARGET CENTER");
    ImGui::TextWrapped("%s", eclipseStatus(scene));
    ImGui::Text("Visible sunlight: %.1f%%", eclipseCenterVisibility(scene) * 100.0);
    hint("Measured at the target's Sun-facing surface center. Other surface points can have a different view.");
    section("WHAT TO LOOK FOR");
    hint("Umbra: sunlight is fully blocked. Penumbra: only part of the Sun is hidden.");
    ImGui::TextColored(ImVec4(0.75f, 0.50f, 0.95f, 1), "Purple: umbra boundary");
    ImGui::TextColored(gold, "Amber: penumbra boundary");
    section("TRY AN EXPERIMENT");
    hint("Start with total alignment, then increase the phase or inclination. Watch the shadow move and the visible sunlight change.");
    section("MODEL SCOPE");
    hint("A separate illustrative scale keeps both bodies and shadow regions visible. This model demonstrates geometry, not calendar dates or atmospheric scattering.");
}

void UIManager::eclipseLabels(const LabState& lab) const {
    const auto size = ImGui::GetIO().DisplaySize;
    const float left = (size.x < 1200 ? 202.0f : 222.0f) + 32;
    const float detailTop = 132 + std::max(1.0f, size.y - 244) * 0.38f + 6;
    auto* draw = ImGui::GetBackgroundDrawList();
    draw->AddText({left + 12, 136}, IM_COL32(116, 204, 240, 255), "SYSTEM GEOMETRY");
    const char* title = lab.eclipse.observerView ? "OBSERVER VIEW"
                        : lab.eclipse.lunar ? "MOON / SHADOW DETAIL" : "EARTH / SHADOW DETAIL";
    draw->AddText({left + 12, detailTop + 8}, IM_COL32(250, 184, 96, 255), title);
}

void UIManager::setLabMessage(std::string message, bool error) {
    labMessage_ = std::move(message);
    labMessageError_ = error;
    labMessageUntil_ = ImGui::GetTime() + (error ? 12.0 : 7.0);
}
void UIManager::labMessage() {
    if (labMessage_.empty() || ImGui::GetTime() > labMessageUntil_)
        return;
    const auto size = ImGui::GetIO().DisplaySize;
    const double remaining = labMessageUntil_ - ImGui::GetTime();
    const double elapsed = (labMessageError_ ? 12.0 : 7.0) - remaining;
    const float entrance = motion_ ? 1.0f - std::exp(-static_cast<float>(elapsed) * 12) : 1.0f;
    const float opacity = motion_ ? entrance * std::min(1.0f, static_cast<float>(remaining) * 3) : 1;
    ImGui::SetNextWindowPos({size.x * 0.5f, size.y - 112 + 12 * (1 - entrance)}, ImGuiCond_Always, {0.5f, 1.0f});
    ImGui::SetNextWindowSize({std::min(550.0f, size.x - 80), 0});
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, opacity);
    ImGui::Begin("Experiment status", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);
    ImGui::PushStyleColor(ImGuiCol_Text, labMessageError_ ? ImVec4(1, 0.52f, 0.45f, 1) : cyan);
    ImGui::TextWrapped("%s", labMessage_.c_str());
    ImGui::PopStyleColor();
    ImGui::End();
    ImGui::PopStyleVar();
}
} // namespace sse
