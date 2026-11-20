#include "PlanetComparison.h"
#include "ComparisonLayout.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace sse {
namespace {
// Preserve every GL state changed by this offscreen pass. In particular, never
// let an ImGui scissor or the main scene's wireframe/depth state leak into it.
class ComparisonGLState {
  public:
    ComparisonGLState() {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_);
        glGetIntegerv(GL_VIEWPORT, viewport_);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program_);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao_);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer_);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture_);
        glGetIntegerv(GL_POLYGON_MODE, polygon_);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc_);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask_);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask_);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_);
        glGetDoublev(GL_DEPTH_CLEAR_VALUE, &clearDepth_);
        for (int i = 0; i < 6; ++i) enabled_[i] = glIsEnabled(caps_[i]);
    }
    ~ComparisonGLState() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(draw_));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(read_));
        glViewport(viewport_[0], viewport_[1], viewport_[2], viewport_[3]);
        glUseProgram(GLuint(program_));
        glBindVertexArray(GLuint(vao_));
        glBindRenderbuffer(GL_RENDERBUFFER, GLuint(renderbuffer_));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GLuint(texture_));
        glActiveTexture(GLenum(active_));
        glPolygonMode(GL_FRONT_AND_BACK, GLenum(polygon_[0]));
        glDepthFunc(GLenum(depthFunc_));
        glDepthMask(depthMask_);
        glColorMask(colorMask_[0], colorMask_[1], colorMask_[2], colorMask_[3]);
        glClearColor(clear_[0], clear_[1], clear_[2], clear_[3]);
        glClearDepth(clearDepth_);
        for (int i = 0; i < 6; ++i)
            if (enabled_[i]) glEnable(caps_[i]); else glDisable(caps_[i]);
    }
  private:
    const GLenum caps_[6] = {GL_DEPTH_TEST, GL_BLEND, GL_SCISSOR_TEST, GL_CULL_FACE,
                            GL_FRAMEBUFFER_SRGB, GL_STENCIL_TEST};
    GLboolean enabled_[6]{}, depthMask_{}, colorMask_[4]{};
    GLint draw_{}, read_{}, viewport_[4]{}, program_{}, vao_{}, renderbuffer_{};
    GLint active_{}, texture_{}, polygon_[2]{}, depthFunc_{};
    GLfloat clear_[4]{};
    GLdouble clearDepth_{};
};
}

PlanetComparison::PlanetComparison(const std::filesystem::path& root)
    : root_(root), shader_(root / "shaders/comparison.vert", root / "shaders/comparison.frag"),
      sphere_(makeSphere()) {}

void PlanetComparison::resize(int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_.id());
    if (width == width_ && height == height_) return;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_.id());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_.id(), 0);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_.id());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_.id());
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Planet comparison framebuffer is incomplete");
    width_ = width;
    height_ = height;
}

void PlanetComparison::render(const SolarSystem& system, int width, int height) {
    const ComparisonGLState restore;
    resize(width, height);
    const auto& planets = system.planets();
    textures_.resize(planets.size());
    for (int index : {first_, second_}) {
        if (!textures_[index]) {
            const auto& data = planets[index].data();
            textures_[index] = std::make_unique<Texture>(root_ / "assets/textures" / data.texture,
                                                         data.name, data.color);
        }
    }
    glViewport(0, 0, width, height);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(.018f, .025f, .045f, 1);
    glClearDepth(1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);
    const auto layout = makeComparisonLayout(planets[first_].data().diameterKm,
        planets[second_].data().diameterKm, double(width) / height, zoom_);
    const auto vp = glm::ortho(float(-layout.halfWidth), float(layout.halfWidth),
        float(-layout.halfHeight), float(layout.halfHeight), 0.1f, 20.0f) *
        glm::lookAt(glm::vec3(0, 0, 6), glm::vec3(0), glm::vec3(0, 1, 0));
    shader_.use();
    shader_.set("uVP", vp);
    shader_.set("uSurface", 0);
    for (int slot = 0; slot < 2; ++slot) {
        const int index = slot == 0 ? first_ : second_;
        const float radius = float(slot == 0 ? layout.radiusA : layout.radiusB);
        const float x = float(slot == 0 ? layout.centerA : layout.centerB);
        const auto model = glm::translate(glm::mat4(1), glm::vec3(x, 0, 0)) *
            glm::rotate(glm::mat4(1), glm::radians(pitch_), glm::vec3(1, 0, 0)) *
            glm::rotate(glm::mat4(1), glm::radians(yaw_), glm::vec3(0, 1, 0)) *
            glm::scale(glm::mat4(1), glm::vec3(radius));
        shader_.set("uModel", model);
        textures_[index]->bind();
        sphere_.draw();
    }
}

void PlanetComparison::draw(const SolarSystem& system, bool& open) {
    if (!open) return;
    constexpr const char* title = "Planet size comparison";
    if (!ImGui::IsPopupOpen(title)) ImGui::OpenPopup(title);
    const auto display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({display.x * .5f, display.y * .5f}, ImGuiCond_Appearing, {.5f, .5f});
    ImGui::SetNextWindowSize({display.x * .9f, display.y * .88f}, ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints({std::min(620.0f, display.x), std::min(450.0f, display.y)}, display);
    if (!ImGui::BeginPopupModal(title, &open, ImGuiWindowFlags_NoSavedSettings)) return;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !ImGui::GetIO().WantTextInput) {
        open = false;
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }
    const auto& planets = system.planets();
    if (planets.empty()) {
        ImGui::TextUnformatted("No planets available.");
        ImGui::EndPopup();
        return;
    }
    first_ = std::clamp(first_, 0, int(planets.size()) - 1);
    second_ = std::clamp(second_, 0, int(planets.size()) - 1);
    auto select = [&](const char* label, int& index) {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo(label, planets[index].data().name.c_str())) {
            for (int i = 0; i < int(planets.size()); ++i) {
                if (ImGui::Selectable(planets[i].data().name.c_str(), index == i)) {
                    index = i;
                    zoom_ = 1;
                }
                if (index == i) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    };
    ImGui::TextWrapped("Real mean diameters. One shared scale, orthographic projection. Rings are excluded.");
    if (ImGui::BeginTable("planet choices", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn(); ImGui::TextUnformatted("A / LEFT"); select("##first", first_);
        ImGui::TableNextColumn(); ImGui::TextUnformatted("B / RIGHT"); select("##second", second_);
        ImGui::EndTable();
    }
    if (ImGui::Button("Swap A / B")) std::swap(first_, second_);
    if (planets.size() >= 5) {
        ImGui::SameLine();
        if (ImGui::Button("Earth / Jupiter")) { first_ = 2; second_ = 4; zoom_ = 1; }
        ImGui::SameLine();
        if (ImGui::Button("Earth / Mars")) { first_ = 2; second_ = 3; zoom_ = 1; }
    }
    const auto& a = planets[first_].data();
    const auto& b = planets[second_].data();
    if (ImGui::BeginTable("diameters", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn(); ImGui::Text("%s: %.0f km", a.name.c_str(), a.diameterKm);
        ImGui::TableNextColumn(); ImGui::Text("%s: %.0f km", b.name.c_str(), b.diameterKm);
        ImGui::EndTable();
    }
    ImGui::Text("Diameter B / A: %.3fx", b.diameterKm / a.diameterKm);
    ImGui::Checkbox("Rotate", &rotate_); ImGui::SameLine();
    ImGui::Checkbox("Wireframe", &wireframe_); ImGui::SameLine();
    if (ImGui::Button("Fit / reset")) { zoom_ = 1; yaw_ = -25; pitch_ = 0; }
    ImGui::SetNextItemWidth(-90);
    ImGui::SliderFloat("Zoom", &zoom_, .5f, 3.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextDisabled("Drag: rotate surfaces | Wheel: shared zoom | Double-click: fit | Esc: close");
    auto available = ImGui::GetContentRegionAvail();
    // Do not render a zero-sized FBO when the window is collapsed by its parent.
    if (available.x >= 2 && available.y >= 2) {
        const auto origin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("comparison canvas", available);
        const auto& io = ImGui::GetIO();
        if (ImGui::IsItemHovered()) {
            zoom_ = std::clamp(zoom_ * std::exp(io.MouseWheel * .12f), .5f, 3.0f);
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) { zoom_ = 1; yaw_ = -25; pitch_ = 0; }
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            yaw_ = std::remainder(yaw_ + io.MouseDelta.x * .4f, 360.0f);
            pitch_ = std::clamp(pitch_ + io.MouseDelta.y * .4f, -85.0f, 85.0f);
        } else if (rotate_) {
            yaw_ = std::remainder(yaw_ + std::min(io.DeltaTime, .1f) * 12.0f, 360.0f);
        }
        // Use framebuffer pixels on HiDPI displays, capped to bound VRAM usage.
        const float scale = std::min({std::max(io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y),
                                     2048.0f / available.x, 2048.0f / available.y});
        const int width = std::max(2, int(std::round(available.x * scale)));
        const int height = std::max(2, int(std::round(available.y * scale)));
        render(system, width, height);
        auto* draw = ImGui::GetWindowDrawList();
        const ImVec2 end(origin.x + available.x, origin.y + available.y);
        draw->AddImage(static_cast<ImTextureID>(color_.id()), origin, end, {0, 1}, {1, 0});
        draw->PushClipRect(origin, end, true);
        const auto layout = makeComparisonLayout(a.diameterKm, b.diameterKm, double(width) / height, zoom_);
        auto label = [&](double center, const std::string& name) {
            const float x = origin.x + float((center / layout.halfWidth + 1) * .5) * available.x;
            const float textWidth = ImGui::CalcTextSize(name.c_str()).x;
            draw->AddText({x - textWidth * .5f, origin.y + 10}, IM_COL32(230, 236, 248, 255), name.c_str());
        };
        label(layout.centerA, a.name); label(layout.centerB, b.name);
        // A scale bar in physical kilometres, using the same projection as both bodies.
        const float barWidth = available.x * .18f;
        const double km = barWidth / available.x * 2 * layout.halfWidth * layout.kilometresPerUnit;
        const float bx = origin.x + 14, by = end.y - 12;
        draw->AddLine({bx, by}, {bx + barWidth, by}, IM_COL32(150, 205, 240, 255), 2);
        draw->AddLine({bx, by - 4}, {bx, by + 3}, IM_COL32(150, 205, 240, 255));
        draw->AddLine({bx + barWidth, by - 4}, {bx + barWidth, by + 3}, IM_COL32(150, 205, 240, 255));
        char text[48]; std::snprintf(text, sizeof(text), "%.0f km", km);
        draw->AddText({bx, by - ImGui::GetTextLineHeight() - 6}, IM_COL32(190, 210, 230, 255), text);
        draw->PopClipRect();
    }
    ImGui::EndPopup();
}
} // namespace sse
