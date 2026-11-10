#include "UIManager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
namespace sse {
namespace {
const ImVec4 muted(0.61f, 0.69f, 0.79f, 1), white(0.92f, 0.95f, 0.99f, 1), gold(1.0f, 0.70f, 0.42f, 1),
    cyan(0.40f, 0.85f, 0.82f, 1);
ImU32 color(glm::vec3 c, int alpha = 255);
constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
void panel(const char* name, ImVec2 pos, ImVec2 size, bool scrollable = true) {
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::Begin(name, nullptr,
                 panelFlags |
                     (scrollable ? 0 : ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse));
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddLine({pos.x + 20, pos.y + 1}, {pos.x + size.x - 20, pos.y + 1}, IM_COL32(127, 169, 206, 46));
}
float ease(float current, float target, float speed, bool motion) {
    return motion ? current + (target - current) * (1.0f - std::exp(-speed * std::min(ImGui::GetIO().DeltaTime, .1f))) : target;
}
bool softButton(const char* label, ImVec2 size, bool motion, bool selected = false) {
    const auto id = ImGui::GetID(label);
    auto* storage = ImGui::GetStateStorage();
    const float hover = storage->GetFloat(id, 0);
    const ImVec4 base = selected ? ImVec4(.16f, .31f, .37f, 1) : ImVec4(.09f, .15f, .24f, .85f);
    const ImVec4 tint(base.x + .045f * hover, base.y + .09f * hover, base.z + .09f * hover, base.w);
    ImGui::PushStyleColor(ImGuiCol_Button, tint);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, tint);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, {.20f, .42f, .46f, 1});
    const bool clicked = ImGui::Button(label, size);
    storage->SetFloat(id, ease(hover, ImGui::IsItemHovered() ? 1.0f : 0.0f, 14, motion));
    ImGui::PopStyleColor(3);
    return clicked;
}
void planetDisc(ImDrawList* dl, ImVec2 center, float radius, glm::vec3 tint, bool rings = false) {
    if (rings)
        dl->AddEllipse(center, {radius * 1.65f, radius * .46f}, color(tint, 165), -.35f, 36, 2);
    dl->AddCircleFilled(center, radius, color(tint * .40f));
    dl->AddCircleFilled({center.x - radius * .16f, center.y - radius * .13f}, radius * .81f, color(tint * .78f));
    dl->AddCircleFilled({center.x - radius * .32f, center.y - radius * .26f}, radius * .45f, color(tint));
}
void eyebrow(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, muted);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}
void rule() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}
void fact(const char* name, const char* value) {
    eyebrow(name);
    ImGui::TextColored(white, "%s", value);
    ImGui::Dummy(ImVec2(0, 9));
}
ImU32 color(glm::vec3 c, int alpha) {
    return IM_COL32(int(c.r * 255), int(c.g * 255), int(c.b * 255), alpha);
}
} // namespace
UIManager::UIManager(GLFWwindow* window, const std::filesystem::path& root) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    try {
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Merely focusing a panel should not reserve every application shortcut.
        // Active widgets and text fields still capture input normally.
        io.ConfigNavCaptureKeyboard = false;
        const auto font = root / "assets/fonts/Roboto-Medium.ttf";
        if (std::filesystem::exists(font)) {
            const auto* glyphs = io.Fonts->GetGlyphRangesCyrillic();
            body_ = io.Fonts->AddFontFromFileTTF(font.u8string().c_str(), 16, nullptr, glyphs);
            heading_ = io.Fonts->AddFontFromFileTTF(font.u8string().c_str(), 23, nullptr, glyphs);
            hero_ = io.Fonts->AddFontFromFileTTF(font.u8string().c_str(), 38, nullptr, glyphs);
        }
        if (!body_)
            body_ = io.Fonts->AddFontDefault();
        if (!heading_)
            heading_ = body_;
        if (!hero_)
            hero_ = heading_;
        io.FontDefault = body_;
        auto& style = ImGui::GetStyle();
        style.WindowPadding = {18, 16};
        style.FramePadding = {11, 8};
        style.ItemSpacing = {8, 9};
        style.WindowRounding = 18;
        style.FrameRounding = 9;
        style.GrabRounding = 6;
        style.PopupRounding = 12;
        style.TabRounding = 7;
        style.WindowBorderSize = 1;
        style.FrameBorderSize = 0;
        style.ScrollbarSize = 6;
        style.Colors[ImGuiCol_WindowBg] = {0.035f, 0.063f, 0.112f, 0.94f};
        style.Colors[ImGuiCol_PopupBg] = {0.055f, 0.09f, 0.15f, .99f};
        style.Colors[ImGuiCol_Border] = {0.32f, 0.46f, 0.59f, 0.24f};
        style.Colors[ImGuiCol_Text] = white;
        style.Colors[ImGuiCol_TextDisabled] = muted;
        style.Colors[ImGuiCol_Button] = {0.09f, 0.15f, 0.24f, 0.9f};
        style.Colors[ImGuiCol_ButtonHovered] = {0.15f, 0.29f, 0.36f, 1};
        style.Colors[ImGuiCol_ButtonActive] = {0.20f, 0.42f, 0.46f, 1};
        style.Colors[ImGuiCol_FrameBg] = {0.07f, 0.115f, 0.19f, 1};
        style.Colors[ImGuiCol_FrameBgHovered] = {0.105f, 0.21f, 0.29f, 1};
        style.Colors[ImGuiCol_FrameBgActive] = {0.14f, 0.30f, 0.35f, 1};
        style.Colors[ImGuiCol_Header] = {0.12f, 0.24f, 0.30f, .9f};
        style.Colors[ImGuiCol_HeaderHovered] = {0.15f, 0.29f, 0.36f, 1};
        style.Colors[ImGuiCol_HeaderActive] = {0.18f, 0.36f, 0.40f, 1};
        style.Colors[ImGuiCol_Tab] = {.075f, .13f, .21f, 1};
        style.Colors[ImGuiCol_TabSelected] = {.15f, .29f, .35f, 1};
        style.Colors[ImGuiCol_TabHovered] = {.19f, .36f, .41f, 1};
        style.Colors[ImGuiCol_NavCursor] = cyan;
        style.Colors[ImGuiCol_CheckMark] = cyan;
        style.Colors[ImGuiCol_SliderGrab] = cyan;
        style.Colors[ImGuiCol_SliderGrabActive] = white;
        style.Colors[ImGuiCol_Separator] = {0.18f, 0.27f, 0.36f, 0.5f};
        if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
            throw std::runtime_error("ImGui GLFW backend initialization failed");
        glfwReady_ = true;
        if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
            throw std::runtime_error("ImGui OpenGL backend initialization failed");
        glReady_ = true;
    } catch (...) {
        if (glReady_)
            ImGui_ImplOpenGL3_Shutdown();
        if (glfwReady_)
            ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        throw;
    }
}
UIManager::~UIManager() {
    if (glReady_)
        ImGui_ImplOpenGL3_Shutdown();
    if (glfwReady_)
        ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
void UIManager::begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
UIActions UIManager::draw(SolarSystem& system, const Camera& camera, RenderSettings& settings, int selected,
                          LabState& lab) {
    UIActions actions;
    const auto size = ImGui::GetIO().DisplaySize;
    const float left = size.x < 1200 ? 202.0f : 222.0f, right = size.x < 1200 ? 248.0f : 284.0f;
    const float top = 116, bottom = 96;
    const int currentView = static_cast<int>(lab.view);
    if (previousView_ != currentView || previousSelection_ != selected) {
        reveal_ = motion_ ? 0.0f : 1.0f;
        previousView_ = currentView;
        previousSelection_ = selected;
    }
    reveal_ = ease(reveal_, 1, 10, motion_);
    if (motion_)
        ambientPhase_ += std::min(ImGui::GetIO().DeltaTime, .1f) * .45f;
    navigationPosition_ = ease(navigationPosition_, currentView * 128.0f, 16, motion_);
    panel("Top", {16, 12}, {size.x - 32, top - 12}, false);
    auto* draw = ImGui::GetWindowDrawList();
    const ImVec2 emblem(52, 40);
    draw->AddCircleFilled(emblem, 9, IM_COL32(255, 179, 107, 255));
    draw->AddEllipse(emblem, {22, 12}, IM_COL32(144, 185, 210, 155), -.45f, 48, 1);
    draw->AddCircleFilled({emblem.x + 22 * std::cos(ambientPhase_), emblem.y + 12 * std::sin(ambientPhase_)},
                          3, IM_COL32(102, 218, 209, 255));
    ImGui::SetCursorPos({68, 12});
    ImGui::PushFont(heading_);
    ImGui::TextUnformatted("Solar System Explorer");
    ImGui::PopFont();
    ImGui::SetCursorPos({69, 39});
    ImGui::TextColored(muted, "Your observatory, beyond Earth");
    ImGui::SetCursorPos({size.x - 397, 18});
    const bool running = lab.view == LabView::Eclipse ? lab.eclipse.animate :
                         lab.view == LabView::Workshop ? lab.workshop.animate : !system.paused();
    const auto statusPos = ImGui::GetCursorScreenPos();
    const float pulse = motion_ && running ? .65f + .35f * std::sin(ambientPhase_ * 4) : 1;
    draw->AddCircleFilled({statusPos.x + 4, statusPos.y + 9}, 3.5f,
                          running ? IM_COL32(102, 218, 209, int(180 + 75 * pulse)) : IM_COL32(255, 179, 107, 255));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15);
    ImGui::TextColored(running ? cyan : gold, running ? "Live" : "Paused");
    ImGui::SameLine(0, 18);
    ImGui::TextColored(muted, "%.0f FPS", ImGui::GetIO().Framerate);
    ImGui::SameLine(0, 18);
    ImGui::Checkbox("Motion", &motion_);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Decorative UI motion. Simulation and camera remain independently controlled.");
    ImGui::SameLine(0, 16);
    if (softButton("H  Help", {76, 28}, motion_))
        help_ = !help_;
    const auto topPosition = ImGui::GetWindowPos();
    const float indicatorX = topPosition.x + 20 + navigationPosition_;
    draw->AddRectFilled({indicatorX + 18, topPosition.y + 96}, {indicatorX + 102, topPosition.y + 99},
                        IM_COL32(102, 218, 209, 240), 2);
    ImGui::SetCursorPos({20, 64});
    auto mode = [&](const char* title, LabView view) {
        const bool active = lab.view == view;
        if (softButton(title, {120, 29}, motion_, active) && !active) {
            lab.view = view;
            if (view == LabView::Orbit) {
                if (selected < 0)
                    actions.select = 3;
                actions.reset = true;
                settings.trails = settings.velocityVectors = settings.apsides = true;
            }
        }
    };
    mode("Explore", LabView::Explore);
    ImGui::SameLine();
    mode("Orbit Lab", LabView::Orbit);
    ImGui::SameLine();
    mode("Eclipse Lab", LabView::Eclipse);
    ImGui::SameLine();
    mode("Workshop", LabView::Workshop);
    ImGui::SameLine(size.x - 392);
    if (softButton("Save...", {90, 29}, motion_))
        actions.saveExperiment = true;
    ImGui::SameLine();
    if (softButton("Load...", {90, 29}, motion_))
        actions.loadExperiment = true;
    ImGui::SameLine();
    if (softButton("Reset experiment", {140, 29}, motion_))
        actions.resetExperiment = true;
    ImGui::End();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, .60f + .40f * reveal_);
    panel("Destinations", {16, top + 16}, {left, size.y - top - bottom - 32});
    if (lab.view == LabView::Eclipse) {
        eclipseControls(lab.eclipse);
    } else if (lab.view == LabView::Workshop) {
        workshopControls(lab, settings);
    } else {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, size.y < 760 ? 5.0f : 9.0f));
    ImGui::PushFont(heading_);
    ImGui::TextUnformatted("Destinations");
    ImGui::PopFont();
    if (size.y >= 760) {
        ImGui::TextColored(muted, "Find your next world");
        ImGui::Dummy({0, 6});
    }
    if (softButton("System overview", {-1, 32}, motion_, camera.followed() < 0)) {
        if (lab.view == LabView::Orbit) actions.reset = true;
        else actions.select = -1;
    }
    rule();
    for (size_t i = 0; i < system.planets().size(); ++i) {
        const auto& p = system.planets()[i];
        ImGui::PushID(static_cast<int>(i));
        const auto pos = ImGui::GetCursorScreenPos();
        const float rowHeight = size.y < 760 ? 27.0f : 43.0f;
        const float rowWidth = ImGui::GetContentRegionAvail().x;
        if (ImGui::Selectable("##planet", selected == static_cast<int>(i), 0, {0, rowHeight}))
            actions.select = static_cast<int>(i);
        const bool hovered = ImGui::IsItemHovered();
        destinationGlow_[i] = ease(destinationGlow_[i], selected == static_cast<int>(i) ? 1.0f : hovered ? .45f : 0.0f,
                                   12, motion_);
        auto* dl = ImGui::GetWindowDrawList();
        const float glow = destinationGlow_[i];
        if (glow > .01f) {
            dl->AddRectFilled(pos, {pos.x + rowWidth, pos.y + rowHeight}, color(p.data().color, int(26 * glow)), 9);
            dl->AddRect(pos, {pos.x + rowWidth, pos.y + rowHeight}, color(p.data().color, int(110 * glow)), 9);
        }
        planetDisc(dl, {pos.x + 15, pos.y + rowHeight * .5f}, rowHeight < 30 ? 7.0f : 10.0f,
                   p.data().color, i == 5);
        dl->AddText({pos.x + 37, pos.y + (rowHeight - 17) * 0.5f}, IM_COL32(234, 242, 252, 255),
                    p.data().name.c_str());
        char index[4];
        std::snprintf(index, sizeof(index), "%d", static_cast<int>(i) + 1);
        dl->AddText({pos.x + rowWidth - 14, pos.y + (rowHeight - 17) * 0.5f}, IM_COL32(137, 159, 184, 255),
                    index);
        ImGui::PopID();
    }
    if (size.y >= 760) {
        rule();
        ImGui::TextColored(system.isModified() ? gold : muted,
                           system.isModified() ? "Experiment modified" : "8 planets around one star");
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(muted, "Distances and sizes are scaled for exploration.");
        ImGui::PopTextWrapPos();
    }
    ImGui::PopStyleVar();
    }
    ImGui::End();

    panel("Object data", {size.x - right - 16, top + 16}, {right, size.y - top - bottom - 32});
    if (lab.view == LabView::Orbit) {
        orbitPanel(system, settings, selected, actions);
    } else if (lab.view == LabView::Eclipse) {
        eclipseInfo(lab.eclipse, settings);
    } else if (lab.view == LabView::Workshop) {
        missionControls(system, settings, lab, selected);
    } else if (selected >= 0) {
        const auto& data = system.planets()[static_cast<size_t>(selected)].data();
        eyebrow(data.category.c_str());
        ImGui::Dummy({0, 6});
        ImGui::PushFont(hero_);
        ImGui::TextUnformatted(data.name.c_str());
        ImGui::PopFont();
        ImGui::TextColored(cyan, camera.followed() == selected ? "Following its journey" : "Ready to explore");
        rule();
        char value[96];
        std::snprintf(value, sizeof(value), "%.0f km", data.diameterKm);
        fact("Mean diameter", value);
        if (data.distanceMillionKm < 1000)
            std::snprintf(value, sizeof(value), "%.1f million km", data.distanceMillionKm);
        else
            std::snprintf(value, sizeof(value), "%.2f billion km", data.distanceMillionKm / 1000);
        fact(system.isModified() ? "Reference distance from Sun" : "Distance from Sun", value);
        if (data.orbitalDays < 1000)
            std::snprintf(value, sizeof(value), "%.1f Earth days", data.orbitalDays);
        else
            std::snprintf(value, sizeof(value), "%.2f Earth years", data.orbitalDays / 365.256);
        fact("Orbital period", value);
        std::snprintf(value, sizeof(value), "%.2f hours%s", std::abs(data.rotationDays) * 24,
                      data.rotationDays < 0 || selected == 6 ? " / retrograde" : "");
        fact("Sidereal rotation", value);
        rule();
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(muted, "%s", data.description.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Dummy({0, 10});
        if (ImGui::Button("F  Refocus", {-1, 34}))
            actions.refocus = true;
    } else {
        eyebrow("Welcome to your observatory");
        ImGui::Dummy({0, 9});
        ImGui::PushFont(hero_);
        ImGui::TextUnformatted("Space,\na little\ncloser.");
        ImGui::PopFont();
        rule();
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(muted, "Travel from the rocky inner planets to the distant ice giants. Choose a "
                                  "destination and follow its journey around our Sun.");
        ImGui::PopTextWrapPos();
        ImGui::Dummy({0, 18});
        fact("Our solar system", "4.6 billion years in motion");
        fact("Sunlight reaches Earth", "In 8 minutes 20 seconds");
        rule();
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(gold, "Start with Earth");
        ImGui::TextColored(muted, "Explore the atmosphere, night lights and our orbiting Moon.");
        ImGui::PopTextWrapPos();
        ImGui::Dummy({0, 12});
        if (softButton("Visit Earth", {-1, 38}, motion_))
            actions.select = 2;
    }
    if (lab.view != LabView::Workshop && lab.missions.active >= 0) {
        rule();
        missionControls(system, settings, lab, selected);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    panel("Flight controls", {16, size.y - bottom}, {size.x - 32, bottom - 16}, false);
    if (lab.view == LabView::Eclipse) {
        if (softButton(lab.eclipse.animate ? "Pause" : "Animate", {86, 32}, motion_))
            lab.eclipse.animate = !lab.eclipse.animate;
        ImGui::SameLine(0, 16);
        ImGui::SetNextItemWidth(180);
        ImGui::SliderFloat("deg / sec", &lab.eclipse.angularSpeed, 0, 30, "%.1f",
                          ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine(0, 20);
        ImGui::Checkbox("Observer", &lab.eclipse.observerView);
        ImGui::SameLine(0, 16);
        ImGui::Checkbox("Shadow cones", &lab.eclipse.showCones);
        ImGui::TextColored(muted, "Space to pause   /   Explore the alignment with phase and inclination");
    } else if (lab.view == LabView::Workshop) {
        if (softButton(lab.workshop.animate ? "Pause" : "Animate", {86, 32}, motion_))
            lab.workshop.animate = !lab.workshop.animate;
        ImGui::SameLine(0, 16);
        ImGui::SetNextItemWidth(size.x < 1200 ? 165.0f : 235.0f);
        ImGui::SliderFloat("Rotation", &lab.workshop.rotationSpeed, -45, 45, "%.1f deg/s", ImGuiSliderFlags_AlwaysClamp);
        ImGui::SameLine(0, 18);
        if (softButton("Reset camera", {115, 32}, motion_))
            actions.reset = true;
        ImGui::SameLine(0, 18);
        ImGui::SetNextItemWidth(130);
        ImGui::SliderFloat("Exposure", &settings.exposure, .2f, 3, "%.2f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::TextColored(muted, "Drag to orbit   /   Scroll to zoom   /   Space to pause   /   Choose a challenge on the right");
    } else {
    if (softButton(system.paused() ? "Resume" : "Pause", {86, 32}, motion_))
        system.setPaused(!system.paused());
    ImGui::SameLine(0, 16);
    ImGui::SetNextItemWidth(size.x < 1200 ? 165.0f : 235.0f);
    float speed = system.speed();
    if (ImGui::SliderFloat("##speed", &speed, 0.05f, 100, "%.2fx", ImGuiSliderFlags_Logarithmic))
        system.setSpeed(speed);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("1x = 6 simulated days per second\n+ / - changes the time scale");
    ImGui::SameLine(0, 18);
    if (softButton("Reset camera", {115, 32}, motion_))
        actions.reset = true;
    ImGui::SameLine(0, 20);
    ImGui::Checkbox("Orbits", &settings.orbits);
    ImGui::SameLine(0, 12);
    ImGui::Checkbox("Labels", &settings.labels);
    ImGui::SameLine(0, 12);
    ImGui::Checkbox("Bloom", &settings.bloom);
    ImGui::SameLine(0, 18);
    ImGui::TextColored(cyan, "Day %07.1f", system.days());
    ImGui::TextColored(
        muted,
        "Drag to orbit   /   Scroll to zoom   /   WASD to pan   /   1-8 to explore   /   Space to pause");
    }
    ImGui::End();
    const float sceneLeft = left + 54, sceneWidth = size.x - left - right - 108;
    if (lab.view != LabView::Eclipse && sceneWidth > 100) {
        auto* dl = ImGui::GetBackgroundDrawList();
        const char* sceneTitle = lab.view == LabView::Workshop ? "Planet Workshop" :
                                 lab.view == LabView::Orbit ? "An orbit, reimagined" :
                                 selected >= 0 ? system.planets()[selected].data().name.c_str() : "The solar neighborhood";
        dl->AddText(heading_, 23, {sceneLeft, 139}, IM_COL32(234, 242, 252, int(255 * (.6f + .4f * reveal_))), sceneTitle);
        const char* sceneHint = lab.view == LabView::Workshop ? "Build a world. Make it your own." :
                                lab.view == LabView::Orbit ? "Change the orbit. See the mathematics move." :
                                "Eight worlds. Endless perspectives.";
        if (sceneWidth > 300)
            dl->AddText({sceneLeft, 169}, IM_COL32(156, 175, 200, 230), sceneHint);
    }
    helpWindow();
    labMessage();
    return actions;
}
void UIManager::helpWindow() {
    if (!help_)
        return;
    auto size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({size.x * 0.5f, size.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({530, 0});
    if (ImGui::Begin("Flight manual", &help_,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::PushFont(heading_);
        ImGui::TextUnformatted("A universe at your fingertips");
        ImGui::PopFont();
        rule();
        ImGui::TextWrapped("Orbit Lab: edit trajectories, seek time, and inspect wireframe or world normals. "
                           "Eclipse Lab: compare geometry and observer views. Workshop: create procedural worlds and complete challenges. "
                           "Save / Load stores the full experiment. Turn Motion off in the header to stop decorative animation.");
        rule();
        if (ImGui::BeginTable("Shortcuts", 2, ImGuiTableFlags_RowBg)) {
            const char* controls[][2] = {{"Left mouse drag", "Orbit the camera"},
                                         {"Mouse wheel", "Zoom in / out"},
                                         {"W A S D", "Pan the camera's center"},
                                         {"1 - 8", "Select and follow a planet"},
                                         {"F", "Smoothly refocus the selection"},
                                         {"0 / R", "Return to the full-system overview"},
                                         {"Space", "Pause / resume simulation"},
                                         {"+ / -", "Increase / decrease simulation speed"},
                                         {"H", "Toggle this help"},
                                         {"Escape", "Exit"}};
            for (const auto& row : controls) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextColored(cyan, "%s", row[0]);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(row[1]);
            }
            ImGui::EndTable();
        }
        rule();
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(muted, "Educational scale: radii and distances are exaggerated. Orbital periods "
                                  "keep real relative ratios; axial rotation is slowed 60x. Initial "
                                  "positions are illustrative, not today's ephemeris.");
        ImGui::PopTextWrapPos();
        if (ImGui::Button("Continue exploring", {-1, 36}))
            help_ = false;
    }
    ImGui::End();
}
void UIManager::labels(const SolarSystem& system, const Camera& camera, int selected) const {
    const auto size = ImGui::GetIO().DisplaySize;
    if (size.x <= 0 || size.y <= 0)
        return;
    const auto vp = camera.projection(size.x / size.y) * camera.view();
    auto* dl = ImGui::GetBackgroundDrawList();
    const float left = size.x < 1200 ? 232.0f : 252.0f, right = size.x - (size.x < 1200 ? 280.0f : 316.0f);
    struct Rect {
        float x, y, w, h;
    };
    std::vector<Rect> occupied;
    auto add = [&](glm::vec3 position, const char* name, glm::vec3 tint, bool highlight, float radius) {
        const auto clip = vp * glm::vec4(position, 1);
        if (clip.w <= 0)
            return;
        const auto ndc = glm::vec3(clip) / clip.w;
        if (ndc.z < -1 || ndc.z > 1)
            return;
        ImVec2 p((ndc.x * 0.5f + 0.5f) * size.x, (0.5f - ndc.y * 0.5f) * size.y);
        float pixelRadius = radius / clip.w * size.y * 1.21f;
        const auto text = ImGui::CalcTextSize(name);
        Rect box{p.x + pixelRadius + 12, p.y - 10, text.x + 14, 24};
        const bool labelLeft = box.x + box.w > right;
        if (labelLeft)
            box.x = p.x - pixelRadius - 12 - box.w;
        if (box.x < left || box.x + box.w > right || box.y < 196 || box.y + box.h > size.y - 110)
            return;
        for (const auto& other : occupied)
            if (box.x < other.x + other.w && box.x + box.w > other.x && box.y < other.y + other.h &&
                box.y + box.h > other.y)
                return;
        occupied.push_back(box);
        dl->AddLine({p.x + (labelLeft ? -pixelRadius - 2 : pixelRadius + 2), p.y},
                    {labelLeft ? box.x + box.w + 3 : box.x - 3, box.y + 12}, color(tint, 130));
        dl->AddRectFilled({box.x, box.y}, {box.x + box.w, box.y + box.h}, IM_COL32(4, 10, 19, 185), 4);
        dl->AddText({box.x + 7, box.y + 3}, highlight ? IM_COL32(244, 248, 255, 255) : color(tint, 220),
                    name);
    };
    if (selected >= 0) {
        const auto& p = system.planets()[selected];
        add(p.position(system.days()), p.data().name.c_str(), p.data().color, true, p.data().radius);
    }
    for (size_t i = 0; i < system.planets().size(); ++i)
        if (static_cast<int>(i) != selected) {
            const auto& p = system.planets()[i];
            add(p.position(system.days()), p.data().name.c_str(), p.data().color, false, p.data().radius);
        }
    add({0, 0, 0}, "Sun", {1, 0.75f, 0.4f}, false, SolarSystem::sunRadius);
    if (camera.followed() == 2)
        add(system.moonPosition(system.days()), "Moon", {0.75f, 0.79f, 0.86f}, false,
            SolarSystem::moonRadius);
}
void UIManager::finish(bool visible) {
    ImGui::Render();
    if (visible)
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
} // namespace sse
