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
const ImVec4 muted(0.48f, 0.57f, 0.68f, 1), white(0.90f, 0.94f, 0.98f, 1), gold(0.98f, 0.73f, 0.39f, 1),
    cyan(0.45f, 0.80f, 0.94f, 1);
constexpr ImGuiWindowFlags panelFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
void panel(const char* name, ImVec2 pos, ImVec2 size, bool scrollable = true) {
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::Begin(name, nullptr,
                 panelFlags |
                     (scrollable ? 0 : ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse));
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
ImU32 color(glm::vec3 c, int alpha = 255) {
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
            body_ = io.Fonts->AddFontFromFileTTF(font.u8string().c_str(), 17);
            heading_ = io.Fonts->AddFontFromFileTTF(font.u8string().c_str(), 23);
            hero_ = io.Fonts->AddFontFromFileTTF(font.u8string().c_str(), 34);
        }
        if (!body_)
            body_ = io.Fonts->AddFontDefault();
        if (!heading_)
            heading_ = body_;
        if (!hero_)
            hero_ = heading_;
        io.FontDefault = body_;
        auto& style = ImGui::GetStyle();
        style.WindowPadding = {20, 18};
        style.FramePadding = {10, 8};
        style.ItemSpacing = {9, 8};
        style.WindowRounding = 12;
        style.FrameRounding = 7;
        style.GrabRounding = 4;
        style.WindowBorderSize = 1;
        style.FrameBorderSize = 0;
        style.ScrollbarSize = 7;
        style.Colors[ImGuiCol_WindowBg] = {0.025f, 0.043f, 0.069f, 0.90f};
        style.Colors[ImGuiCol_Border] = {0.19f, 0.28f, 0.38f, 0.40f};
        style.Colors[ImGuiCol_Text] = white;
        style.Colors[ImGuiCol_TextDisabled] = muted;
        style.Colors[ImGuiCol_Button] = {0.10f, 0.16f, 0.22f, 0.85f};
        style.Colors[ImGuiCol_ButtonHovered] = {0.16f, 0.29f, 0.38f, 1};
        style.Colors[ImGuiCol_ButtonActive] = {0.19f, 0.36f, 0.46f, 1};
        style.Colors[ImGuiCol_FrameBg] = {0.08f, 0.13f, 0.19f, 1};
        style.Colors[ImGuiCol_FrameBgHovered] = {0.13f, 0.23f, 0.31f, 1};
        style.Colors[ImGuiCol_FrameBgActive] = {0.16f, 0.30f, 0.40f, 1};
        style.Colors[ImGuiCol_Header] = {0.13f, 0.24f, 0.33f, 0.8f};
        style.Colors[ImGuiCol_HeaderHovered] = {0.15f, 0.28f, 0.37f, 1};
        style.Colors[ImGuiCol_HeaderActive] = {0.19f, 0.34f, 0.45f, 1};
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
UIActions UIManager::draw(SolarSystem& system, const Camera& camera, RenderSettings& settings, int selected) {
    UIActions actions;
    const auto size = ImGui::GetIO().DisplaySize;
    const float left = size.x < 1200 ? 202.0f : 222.0f, right = size.x < 1200 ? 248.0f : 284.0f;
    const float top = 76, bottom = 96;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    panel("Top", {0, 0}, {size.x, top}, false);
    auto* draw = ImGui::GetWindowDrawList();
    auto origin = ImGui::GetCursorScreenPos();
    draw->AddCircleFilled({origin.x + 16, origin.y + 16}, 9, IM_COL32(250, 184, 96, 255));
    draw->AddCircle({origin.x + 16, origin.y + 16}, 15, IM_COL32(250, 184, 96, 100), 32, 1.0f);
    ImGui::SetCursorPosX(64);
    ImGui::PushFont(heading_);
    ImGui::TextUnformatted("Solar System Explorer");
    ImGui::PopFont();
    ImGui::SetCursorPosX(64);
    ImGui::TextColored(muted, "AN INTERACTIVE ATLAS  /  COMPUTER GRAPHICS");
    ImGui::SetCursorPos({size.x - 326, 21});
    ImGui::TextColored(system.paused() ? gold : cyan, system.paused() ? "PAUSED" : "SIMULATION LIVE");
    ImGui::SameLine(0, 23);
    ImGui::TextColored(muted, "%.0f FPS", ImGui::GetIO().Framerate);
    ImGui::SameLine(0, 18);
    if (ImGui::SmallButton("H  Help"))
        help_ = !help_;
    ImGui::End();
    ImGui::PopStyleVar();
    panel("Destinations", {16, top + 16}, {left, size.y - top - bottom - 32});
    eyebrow("EXPLORE THE SYSTEM");
    ImGui::Dummy({0, 6});
    if (ImGui::Selectable("00   Overview", selected < 0, 0, {0, 34}))
        actions.select = -1;
    rule();
    for (size_t i = 0; i < system.planets().size(); ++i) {
        const auto& p = system.planets()[i];
        ImGui::PushID(static_cast<int>(i));
        const auto pos = ImGui::GetCursorScreenPos();
        const float rowHeight = size.y < 760 ? 25.0f : 37.0f;
        if (ImGui::Selectable("##planet", selected == static_cast<int>(i), 0, {0, rowHeight}))
            actions.select = static_cast<int>(i);
        auto* dl = ImGui::GetWindowDrawList();
        dl->AddCircleFilled({pos.x + 12, pos.y + rowHeight * 0.5f}, 5, color(p.data().color));
        dl->AddText({pos.x + 29, pos.y + (rowHeight - 17) * 0.5f}, IM_COL32(223, 233, 243, 255),
                    p.data().name.c_str());
        char index[4];
        std::snprintf(index, sizeof(index), "%d", static_cast<int>(i) + 1);
        dl->AddText({pos.x + left - 57, pos.y + (rowHeight - 17) * 0.5f}, IM_COL32(101, 123, 145, 255),
                    index);
        ImGui::PopID();
    }
    if (size.y >= 760) {
        rule();
        ImGui::TextColored(muted, "8 planets   /   1 star");
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(muted, "Distances and sizes are scaled for exploration.");
        ImGui::PopTextWrapPos();
    }
    ImGui::End();

    panel("Object data", {size.x - right - 16, top + 16}, {right, size.y - top - bottom - 32});
    if (selected >= 0) {
        const auto& data = system.planets()[static_cast<size_t>(selected)].data();
        eyebrow(data.category.c_str());
        ImGui::Dummy({0, 6});
        ImGui::PushFont(hero_);
        ImGui::TextUnformatted(data.name.c_str());
        ImGui::PopFont();
        ImGui::TextColored(cyan, camera.followed() == selected ? "TRACKING ORBIT" : "DESTINATION SELECTED");
        rule();
        char value[96];
        std::snprintf(value, sizeof(value), "%.0f km", data.diameterKm);
        fact("MEAN DIAMETER", value);
        if (data.distanceMillionKm < 1000)
            std::snprintf(value, sizeof(value), "%.1f million km", data.distanceMillionKm);
        else
            std::snprintf(value, sizeof(value), "%.2f billion km", data.distanceMillionKm / 1000);
        fact("MEAN DISTANCE FROM SUN", value);
        if (data.orbitalDays < 1000)
            std::snprintf(value, sizeof(value), "%.1f Earth days", data.orbitalDays);
        else
            std::snprintf(value, sizeof(value), "%.2f Earth years", data.orbitalDays / 365.256);
        fact("ORBITAL PERIOD", value);
        std::snprintf(value, sizeof(value), "%.2f hours%s", std::abs(data.rotationDays) * 24,
                      data.rotationDays < 0 || selected == 6 ? " / retrograde" : "");
        fact("SIDEREAL ROTATION", value);
        rule();
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(muted, "%s", data.description.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Dummy({0, 10});
        if (ImGui::Button("F  Refocus", {-1, 34}))
            actions.refocus = true;
    } else {
        eyebrow("THE SOLAR NEIGHBORHOOD");
        ImGui::Dummy({0, 13});
        ImGui::PushFont(hero_);
        ImGui::TextUnformatted("A star.\nEight worlds.\nOne system.");
        ImGui::PopFont();
        rule();
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(muted, "Travel from the rocky inner planets to the distant ice giants. Choose a "
                                  "destination and follow its journey around our Sun.");
        ImGui::PopTextWrapPos();
        ImGui::Dummy({0, 18});
        fact("AGE OF THE SOLAR SYSTEM", "4.6 billion years");
        fact("LIGHT TO EARTH", "About 8 minutes 20 seconds");
        rule();
        ImGui::PushTextWrapPos(0);
        ImGui::TextColored(gold, "Start with Earth");
        ImGui::TextColored(muted, "Explore the atmosphere, night lights and our orbiting Moon.");
        ImGui::PopTextWrapPos();
        ImGui::Dummy({0, 12});
        if (ImGui::Button("03  Visit Earth", {-1, 38}))
            actions.select = 2;
    }
    ImGui::End();

    panel("Flight controls", {16, size.y - bottom}, {size.x - 32, bottom - 16}, false);
    if (ImGui::Button(system.paused() ? "Resume" : "Pause", {86, 32}))
        system.setPaused(!system.paused());
    ImGui::SameLine(0, 16);
    ImGui::SetNextItemWidth(size.x < 1200 ? 165.0f : 235.0f);
    float speed = system.speed();
    if (ImGui::SliderFloat("##speed", &speed, 0.05f, 100, "%.2fx", ImGuiSliderFlags_Logarithmic))
        system.setSpeed(speed);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("1x = 6 simulated days per second\n+ / - changes the time scale");
    ImGui::SameLine(0, 18);
    if (ImGui::Button("Reset camera", {115, 32}))
        actions.reset = true;
    ImGui::SameLine(0, 20);
    ImGui::Checkbox("Orbits", &settings.orbits);
    ImGui::SameLine(0, 12);
    ImGui::Checkbox("Labels", &settings.labels);
    ImGui::SameLine(0, 12);
    ImGui::Checkbox("Bloom", &settings.bloom);
    ImGui::SameLine(0, 18);
    ImGui::TextColored(muted, "DAY %07.1f", system.days());
    ImGui::TextColored(
        muted,
        "DRAG  Orbit     SCROLL  Zoom     WASD  Pan     1-8  Destination     SPACE  Pause     0  Overview");
    ImGui::End();
    helpWindow();
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
        if (box.x < left || box.x + box.w > right || box.y < 100 || box.y + box.h > size.y - 110)
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
