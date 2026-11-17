#include "UIManager.h"
#include "EclipseModel.h"
#include "Missions.h"
#include "WorkshopModel.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <string>

namespace sse {
namespace {
const ImVec4 muted(.64f, .68f, .74f, 1), aqua(.96f, .31f, .34f, 1), warm(1, .70f, .42f, 1);
void section(const char* title) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(aqua, "%s", title);
}
void hint(const char* text) {
    ImGui::PushTextWrapPos(0);
    ImGui::TextColored(muted, "%s", text);
    ImGui::PopTextWrapPos();
}
void slider(const char* title, const char* id, float& value, float minimum, float maximum,
            const char* format = "%.2f") {
    ImGui::TextUnformatted(title);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat(id, &value, minimum, maximum, format, ImGuiSliderFlags_AlwaysClamp);
}
void editColor(const char* title, const char* id, std::array<float, 3>& color) {
    ImGui::TextUnformatted(title);
    ImGui::SetNextItemWidth(-1);
    ImGui::ColorEdit3(id, color.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
}
const char* missionHint(int index) {
    switch (index) {
    case 0: return "Choose Rocky. Aim for 45-75% ocean coverage and relief of at least 0.10. Adjust sea level and watch the coverage estimate.";
    case 1: return "Choose Gas giant. Set at least 12 bands and turbulence of 0.50 or more.";
    case 2: return "Open Planet Workshop, pause rotation, then choose World normals in Surface view.";
    case 3: return "In Orbit Lab, select a planet. Set eccentricity between 0.50 and 0.70 and inclination at least 15 degrees above or below the reference plane.";
    case 4: return "In Eclipse Lab, turn on Observer viewpoint. Adjust the alignment until visible sunlight is between 20% and 80%.";
    default: return "Select a challenge, start it, then check your result when ready.";
    }
}
} // namespace

void UIManager::workshopControls(LabState& lab, RenderSettings& settings) {
    auto& world = lab.workshop;
    ImGui::PushFont(heading_);
    animatedHeading("Workshop");
    ImGui::PopFont();
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##workshop-preset", "Choose a preset")) {
        const char* presets[] = {"Ocean world", "Volcanic world", "Ice world", "Banded giant"};
        for (int index = 0; index < 4; ++index)
            if (ImGui::Selectable(presets[index])) {
                world = workshopPreset(index);
                setLabMessage(std::string(presets[index]) + " loaded. Challenge progress is preserved.");
            }
        ImGui::EndCombo();
    }
    int kind = int(world.kind);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::Combo("##workshop-kind", &kind, "Rocky\0Gas giant\0"))
        world.kind = WorkshopKind(kind);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("World type");

    // Keep the main sculpting controls visible even in the compact window.
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
    if (ImGui::BeginTabBar("World tools", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
        if (ImGui::BeginTabItem("Shape")) {
            slider("Radius", "##workshop-radius", world.radius, .5f, 3, "%.2f");
            if (world.kind == WorkshopKind::Rocky) {
                slider("Relief", "##workshop-relief", world.relief, 0, .25f, "%.3f");
                slider("Sea level", "##workshop-sea", world.seaLevel, 0, 1);
                static WorkshopSettings coverageSettings;
                static bool coverageReady = false;
                static float coverage = 0;
                if (!coverageReady || coverageSettings.seed != world.seed ||
                    coverageSettings.featureScale != world.featureScale || coverageSettings.seaLevel != world.seaLevel ||
                    coverageSettings.kind != world.kind) {
                    coverage = workshopOceanCoverage(world);
                    coverageSettings = world;
                    coverageReady = true;
                }
                ImGui::TextColored(aqua, "Ocean coverage  %.0f%%", coverage * 100);
                ImGui::ProgressBar(coverage, {-1, 4}, "");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Estimated surface area covered by ocean");
            } else {
                ImGui::TextUnformatted("Bands");
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderInt("##workshop-bands", &world.gasBands, 3, 24, "%d", ImGuiSliderFlags_AlwaysClamp);
                slider("Turbulence", "##workshop-turbulence", world.gasTurbulence, 0, 1);
            }
            slider("Feature scale", "##workshop-scale", world.featureScale, 1, 12, "%.1f");
            section("Generation");
            ImGui::TextUnformatted("Seed");
            int seed = int(world.seed);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputInt("##workshop-seed", &seed, 1, 100))
                world.seed = uint32_t(std::clamp(seed, 0, 999999));
            if (ImGui::Button("New seed", {-1, 27}))
                world.seed = (world.seed * 1664525u + 1013904223u) % 1000000u;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("The same seed and settings always recreate the same world.");
            if (ImGui::Button("Reset world", {-1, 27})) {
                world = WorkshopSettings{};
                setLabMessage("World reset. Challenge progress is preserved.");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Surface")) {
            ImGui::TextUnformatted("Surface view");
            int surface = int(settings.surfaceMode);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##workshop-surface", &surface, "Shaded\0Wireframe\0World normals\0"))
                settings.surfaceMode = SurfaceMode(surface);
            if (settings.surfaceMode == SurfaceMode::Wireframe)
                hint("Edges show the actual terrain mesh.");
            else if (settings.surfaceMode == SurfaceMode::Normals)
                hint("RGB encodes the surface direction.");
            if (world.kind == WorkshopKind::Rocky) {
                editColor("Land", "##workshop-land", world.landColor);
                editColor("Ocean", "##workshop-water", world.waterColor);
                slider("Polar ice", "##workshop-ice", world.iceCap, 0, .6f);
            } else {
                editColor("Dark bands", "##workshop-color-a", world.gasColorA);
                editColor("Light bands", "##workshop-color-b", world.gasColorB);
            }
            if (ImGui::CollapsingHeader("Lighting")) {
                ImGui::BeginDisabled(settings.surfaceMode != SurfaceMode::Shaded);
                slider("Ambient", "##workshop-ambient", settings.ambient, 0, 3);
                slider("Diffuse", "##workshop-diffuse", settings.diffuse, 0, 3);
                slider("Specular", "##workshop-specular", settings.specular, 0, 3);
                if (ImGui::Button("Reset lighting", {-1, 27}))
                    settings.ambient = settings.diffuse = settings.specular = 1;
                ImGui::EndDisabled();
            }
            if (ImGui::CollapsingHeader("Rotation")) {
                ImGui::Checkbox("Animate world", &world.animate);
                slider("Speed", "##workshop-speed", world.rotationSpeed, -45, 45, "%.1f deg/s");
                float angle = std::remainder(world.rotationDegrees, 360.0f);
                ImGui::TextUnformatted("Angle");
                ImGui::SetNextItemWidth(-1);
                if (ImGui::SliderFloat("##workshop-angle", &angle, -180, 180, "%.1f deg", ImGuiSliderFlags_AlwaysClamp)) {
                    world.rotationDegrees = angle;
                    world.animate = false;
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Air")) {
            slider("Atmosphere", "##workshop-atmosphere", world.atmosphere, 0, 1);
            editColor("Atmosphere tint", "##workshop-air", world.atmosphereColor);
            if (world.kind == WorkshopKind::Rocky)
                slider("Clouds", "##workshop-clouds", world.clouds, 0, 1);
            hint("Clouds and atmosphere are illustrative, not a weather simulation.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopStyleVar(2);
}

void UIManager::missionControls(SolarSystem& system, RenderSettings& settings, LabState& lab, int selected) {
    const auto& missions = workshopMissions();
    const int completed = int(std::count(lab.missions.completed.begin(), lab.missions.completed.end(), true));
    ImGui::PushFont(heading_);
    animatedHeading("Challenges");
    ImGui::PopFont();
    ImGui::TextColored(aqua, "%d of 5 complete", completed);
    ImGui::ProgressBar(float(completed) / 5, {-1, 4}, "");
    hint("Experiment, then check your result.");
    ImGui::Spacing();
    for (int index = 0; index < int(missions.size()); ++index) {
        ImGui::PushID(index);
        const auto position = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const auto textSize = ImGui::CalcTextSize(missions[size_t(index)].title, nullptr, false, width - 29);
        const float height = std::max(34.0f, textSize.y + 15);
        if (ImGui::Selectable("##challenge", lab.missions.active == index, 0, {0, height}))
            lab.missions.active = index;
        auto* draw = ImGui::GetWindowDrawList();
        const ImVec2 center(position.x + 8, position.y + height * .5f);
        if (lab.missions.completed[size_t(index)]) {
            draw->AddCircleFilled(center, 7, IM_COL32(102, 218, 209, 255));
            draw->AddLine({center.x - 3, center.y}, {center.x - .5f, center.y + 2.5f}, IM_COL32(8, 13, 25, 255), 1.6f);
            draw->AddLine({center.x - .5f, center.y + 2.5f}, {center.x + 3.5f, center.y - 2.5f}, IM_COL32(8, 13, 25, 255), 1.6f);
        } else {
            draw->AddCircle(center, 6, lab.missions.active == index ? IM_COL32(255, 179, 107, 255) : IM_COL32(85, 109, 141, 255), 24, 1.5f);
        }
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {position.x + 24, position.y + (height - textSize.y) * .5f},
                      IM_COL32(234, 241, 251, 255), missions[size_t(index)].title, nullptr, width - 29);
        ImGui::PopID();
    }
    const int active = lab.missions.active;
    if (active >= 0 && active < int(missions.size())) {
        section(lab.missions.completed[size_t(active)] ? "Completed challenge" : "Your experiment");
        ImGui::TextWrapped("%s", missions[size_t(active)].description);
        ImGui::Spacing();
        hint(missionHint(active));
        ImGui::Spacing();
        if (ImGui::Button("Start challenge", {-1, 31})) {
            startMission(active, lab);
            setLabMessage(std::string("Challenge started: ") + missions[size_t(active)].title);
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.29f, .075f, .10f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(.39f, .10f, .14f, 1));
        if (ImGui::Button("Check challenge", {-1, 34})) {
            if (completeMission(active, system, settings, lab, selected)) {
                setLabMessage(std::string("Challenge complete: ") + missions[size_t(active)].title);
            } else
                setLabMessage(std::string("Not quite yet. ") + missionHint(active), true);
        }
        ImGui::PopStyleColor(2);
        if (active == 4 && lab.view == LabView::Eclipse)
            ImGui::TextColored(warm, "Visible sunlight  %.1f%%", eclipseCenterVisibility(makeEclipseScene(lab.eclipse)) * 100);
    } else {
        ImGui::Spacing();
        hint("Choose a challenge above to see its goal.");
    }
    section("Saved with your experiment");
    hint("Save stores your world and completed challenges together.");
    if (ImGui::Button("Reset challenge progress...", {-1, 28}))
        ImGui::OpenPopup("Reset challenge progress?");
    const auto size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({size.x * .5f, size.y * .5f}, ImGuiCond_Appearing, {.5f, .5f});
    ImGui::SetNextWindowSize({std::min(400.0f, size.x - 40), 0}, ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Reset challenge progress?", nullptr,
                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !ImGui::GetIO().WantTextInput)
            ImGui::CloseCurrentPopup();
        ImGui::TextWrapped("Clear all completed challenges? Your world and other experiment settings stay as they are.");
        ImGui::Spacing();
        if (ImGui::Button("Reset progress", {155, 32})) {
            lab.missions = MissionProgress{};
            setLabMessage("Challenge progress reset.");
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Keep progress", {155, 32}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}
} // namespace sse
