#include "UIEffects.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace sse::ui {
namespace {
constexpr float pi = 3.14159265358979323846f;

float unit(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}

float cycle(float phase, float speed, float offset = 0.0f) {
    const double value = (std::isfinite(phase) ? static_cast<double>(phase) : 0.0) * speed + offset;
    const float result = static_cast<float>(value - std::floor(value));
    return result < 1.0f ? result : 0.0f;
}

bool validRect(ImVec2 minimum, ImVec2 maximum) {
    return std::isfinite(minimum.x) && std::isfinite(minimum.y) && std::isfinite(maximum.x) &&
           std::isfinite(maximum.y) && maximum.x > minimum.x && maximum.y > minimum.y &&
           std::isfinite(maximum.x - minimum.x) && std::isfinite(maximum.y - minimum.y);
}

ImVec4 spectrum(float position) {
    // Luminous red/coral and cool silver match the technical HUD without losing legibility.
    constexpr ImVec4 stops[] = {{0.961f, 0.310f, 0.341f, 1.0f}, {1.0f, 0.60f, 0.55f, 1.0f},
                                {0.843f, 0.871f, 0.902f, 1.0f}, {0.69f, 0.75f, 0.82f, 1.0f}};
    const float wrapped = cycle(position, 1.0f) * 4.0f;
    const int index = static_cast<int>(wrapped);
    float blend = wrapped - index;
    blend = blend * blend * (3.0f - 2.0f * blend);
    const auto& a = stops[index];
    const auto& b = stops[(index + 1) % 4];
    return {a.x + (b.x - a.x) * blend, a.y + (b.y - a.y) * blend,
            a.z + (b.z - a.z) * blend, 1.0f};
}

unsigned channel(float value) {
    return static_cast<unsigned>(unit(value) * 255.0f + 0.5f);
}

ImU32 red(float alpha) {
    return IM_COL32(245, 79, 87, static_cast<int>(unit(alpha) * 255.0f));
}

ImU32 white(float alpha) {
    return IM_COL32(215, 222, 230, static_cast<int>(unit(alpha) * 255.0f));
}

ImU32 gray(float alpha) {
    return IM_COL32(110, 122, 134, static_cast<int>(unit(alpha) * 255.0f));
}

ImVec2 polar(ImVec2 center, float radius, float angle) {
    return {center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)};
}

void arc(ImDrawList* draw, ImVec2 center, float radius, float start, float end, ImU32 tint,
         float thickness = 1.0f) {
    draw->PathArcTo(center, radius, start, end, 12);
    draw->PathStroke(tint, ImDrawFlags_None, thickness);
}
} // namespace

void spectrumText(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 position, const char* text,
                  float phase, bool motion, float alpha) {
    alpha = unit(alpha);
    if (!draw || !text || !*text || alpha <= 0.0f || !std::isfinite(fontSize) || fontSize <= 0.0f ||
        !std::isfinite(position.x) || !std::isfinite(position.y))
        return;
    if (!font) {
        if (!ImGui::GetCurrentContext())
            return;
        font = ImGui::GetFont();
    }
    const ImVec2 extent = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
    if (!std::isfinite(extent.x) || !std::isfinite(extent.y) || extent.x <= 0.0f || extent.y <= 0.0f)
        return;
    const ImVec2 clipMin = draw->GetClipRectMin(), clipMax = draw->GetClipRectMax();
    if (position.x + extent.x + 1.0f < clipMin.x || position.y + extent.y + 1.0f < clipMin.y ||
        position.x - 1.0f > clipMax.x || position.y - 1.0f > clipMax.y)
        return;

    // A one-pixel stationary halo adds separation without displacing the main glyphs.
    const ImU32 halo = IM_COL32(245, 111, 114, static_cast<int>(alpha * 12.0f));
    constexpr ImVec2 offsets[] = {{-0.75f, 0.0f}, {0.75f, 0.0f}, {0.0f, -0.75f}, {0.0f, 0.75f}};
    for (const auto& offset : offsets)
        draw->AddText(font, fontSize, {position.x + offset.x, position.y + offset.y}, halo, text);

    const int firstVertex = draw->VtxBuffer.Size;
    draw->AddText(font, fontSize, position, IM_COL32(255, 255, 255, static_cast<int>(alpha * 255.0f)), text);
    const float gradientWidth = std::max(extent.x, fontSize * 6.0f);
    const float drift = motion ? cycle(phase, 0.19f) : 0.0f;
    const float sheenRadius = std::max(9.0f, fontSize * 0.8f);
    const float sheenCenter = -sheenRadius * 2.0f +
                              cycle(phase, 0.24f) * (extent.x + sheenRadius * 4.0f + extent.y * 0.23f);
    for (int i = firstVertex; i < draw->VtxBuffer.Size; ++i) {
        auto& vertex = draw->VtxBuffer[i];
        const float x = vertex.pos.x - position.x;
        const float y = vertex.pos.y - position.y;
        ImVec4 tint = spectrum(x / gradientWidth + y / std::max(extent.y, fontSize) * 0.10f - drift);
        if (motion) {
            float sheen = unit(1.0f - std::abs(x + y * 0.23f - sheenCenter) / sheenRadius);
            sheen = sheen * sheen * (3.0f - 2.0f * sheen) * 0.88f;
            tint.x += (1.0f - tint.x) * sheen;
            tint.y += (1.0f - tint.y) * sheen;
            tint.z += (1.0f - tint.z) * sheen;
        }
        // Keep ImGui's original coverage/alpha; only replace RGB on this text's vertices.
        vertex.col = (vertex.col & IM_COL32_A_MASK) | (channel(tint.x) << IM_COL32_R_SHIFT) |
                     (channel(tint.y) << IM_COL32_G_SHIFT) | (channel(tint.z) << IM_COL32_B_SHIFT);
    }
}

void lightSweep(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                float strength) {
    strength = unit(strength);
    if (!draw || !motion || strength <= 0.0f || !validRect(minimum, maximum))
        return;
    const float width = maximum.x - minimum.x;
    const float radius = std::min(14.0f, std::max(4.0f, width * 0.07f));
    const float center = minimum.x - radius + cycle(phase, 0.24f) * (width + radius * 2.0f);
    const ImU32 edge = IM_COL32(215, 222, 230, 0);
    const ImU32 middle = IM_COL32(255, 176, 178, static_cast<int>(strength * 34.0f));
    draw->PushClipRect(minimum, maximum, true);
    draw->AddRectFilledMultiColor({center - radius, minimum.y}, {center, maximum.y}, edge, middle, middle,
                                 edge);
    draw->AddRectFilledMultiColor({center, minimum.y}, {center + radius, maximum.y}, middle, edge, edge,
                                 middle);
    draw->PopClipRect();
}

void orbitSparkles(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                   float strength) {
    strength = unit(strength);
    if (!draw || !motion || strength <= 0.0f || !validRect(minimum, maximum))
        return;
    const float width = maximum.x - minimum.x, height = maximum.y - minimum.y;
    if (width < 16.0f || height < 16.0f)
        return;
    // Fixed seeds give each card a sparse, repeatable constellation with no retained state.
    constexpr ImVec2 locations[] = {{0.16f, 0.23f}, {0.77f, 0.18f}, {0.89f, 0.67f},
                                    {0.38f, 0.81f}, {0.59f, 0.39f}};
    draw->PushClipRect(minimum, maximum, true);
    for (int i = 0; i < 5; ++i) {
        const float life = cycle(phase, 0.21f, static_cast<float>(i) * 0.213f);
        if (life >= 0.19f)
            continue;
        const float envelope = std::sin(life / 0.19f * pi);
        const float brightness = envelope * envelope * strength;
        const auto& location = locations[i];
        const ImVec2 center(minimum.x + 5.0f + (width - 10.0f) * location.x,
                            minimum.y + 5.0f + (height - 10.0f) * location.y);
        const float radius = 1.6f + brightness * 2.1f;
        const ImU32 tint = IM_COL32(215, 222, 230, static_cast<int>(brightness * 115.0f));
        draw->AddCircleFilled(center, 1.65f, red(brightness * 0.09f), 8);
        draw->AddLine({center.x - radius, center.y}, {center.x + radius, center.y}, tint, 0.8f);
        draw->AddLine({center.x, center.y - radius * 0.65f}, {center.x, center.y + radius * 0.65f}, tint,
                      0.8f);
    }
    draw->PopClipRect();
}

void technicalFrame(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                    float alpha) {
    alpha = unit(alpha);
    if (!draw || alpha <= 0.0f || !validRect(minimum, maximum))
        return;
    const float width = maximum.x - minimum.x, height = maximum.y - minimum.y;
    if (width < 18.0f || height < 18.0f)
        return;
    phase = motion && std::isfinite(phase) ? phase : 0.0f;
    draw->PushClipRect(minimum, maximum, true);
    const float x0 = minimum.x + 1.0f, x1 = maximum.x - 1.0f;
    const float y0 = minimum.y + 1.0f, y1 = maximum.y - 1.0f;
    const float cut = std::min(13.0f, std::min(width, height) * 0.14f);
    const ImVec2 outline[] = {{x0 + cut, y0}, {x1 - cut, y0}, {x1, y0 + cut}, {x1, y1 - cut},
                              {x1 - cut, y1}, {x0 + cut, y1}, {x0, y1 - cut}, {x0, y0 + cut}};
    draw->AddPolyline(outline, 8, gray(alpha * 0.49f), ImDrawFlags_Closed, 1.0f);

    // Corner brackets remain at the edge, leaving the panel's content unobstructed.
    const float bracket = std::min(34.0f, width * 0.17f);
    draw->AddLine({x0 + cut, y0}, {x0 + cut + bracket, y0}, red(alpha * 0.86f), 1.6f);
    draw->AddLine({x0, y0 + cut}, {x0, y0 + cut + std::min(22.0f, height * 0.13f)},
                  red(alpha * 0.63f), 1.6f);
    draw->AddLine({x1 - cut - bracket, y1}, {x1 - cut, y1}, white(alpha * 0.69f), 1.3f);
    draw->AddLine({x1, y1 - cut - std::min(18.0f, height * 0.12f)}, {x1, y1 - cut},
                  red(alpha * 0.7f), 1.6f);

    if (width > 90.0f) {
        const float linkX = x1 - cut - 13.0f, linkY = y0 + 6.0f;
        draw->AddLine({linkX - 30.0f, linkY}, {linkX - 9.0f, linkY}, gray(alpha * 0.6f));
        draw->AddLine({linkX - 9.0f, linkY}, {linkX - 4.0f, linkY - 3.0f}, gray(alpha * 0.6f));
        draw->AddCircleFilled({linkX, linkY - 3.0f}, 1.7f, red(alpha * 0.88f), 8);
        for (int i = 0; i < 4; ++i) {
            const float notchX = x0 + cut + bracket + 7.0f + static_cast<float>(i) * 5.0f;
            if (notchX < linkX - 36.0f)
                draw->AddLine({notchX, y0}, {notchX + 2.0f, y0 + 3.0f}, white(alpha * 0.32f));
        }
    }

    // An intentionally faint fixed-count point field reads as instrumentation behind content.
    if (width > 100.0f && height > 90.0f) {
        for (int row = 0; row < 6; ++row) {
            for (int column = 0; column < 12; ++column) {
                const float x = minimum.x + 12.0f + (width - 24.0f) * (static_cast<float>(column) / 11.0f);
                const float y = minimum.y + 15.0f + (height - 30.0f) * (static_cast<float>(row) / 5.0f);
                draw->AddRectFilled({x, y}, {x + 1.0f, y + 1.0f}, gray(alpha * 0.15f));
            }
        }
        const float scanY = minimum.y + 10.0f + cycle(phase, 0.11f, 0.27f) * (height - 20.0f);
        draw->AddLine({minimum.x + 8.0f, scanY}, {maximum.x - 8.0f, scanY}, white(alpha * 0.045f));
    }
    draw->PopClipRect();
}

void trackingReticle(ImDrawList* draw, ImVec2 center, float radius, float phase, bool motion,
                     float alpha) {
    alpha = unit(alpha);
    if (!draw || alpha <= 0.0f || !std::isfinite(center.x) || !std::isfinite(center.y) ||
        !std::isfinite(radius) || radius < 8.0f)
        return;
    phase = motion && std::isfinite(phase) ? phase : 0.0f;
    draw->PushClipRect({center.x - radius - 2.0f, center.y - radius - 2.0f},
                       {center.x + radius + 2.0f, center.y + radius + 2.0f}, true);
    for (int segment = 0; segment < 8; ++segment) {
        const float start = static_cast<float>(segment) * pi * 0.25f + 0.045f;
        arc(draw, center, radius, start, start + pi * 0.25f - 0.09f, gray(alpha * 0.49f));
    }
    for (int tick = 0; tick < 64; ++tick) {
        const float angle = static_cast<float>(tick) * pi / 32.0f;
        const bool major = tick % 8 == 0;
        const float length = radius * (major ? 0.13f : tick % 4 == 0 ? 0.075f : 0.032f);
        draw->AddLine(polar(center, radius - length, angle), polar(center, radius - 2.5f, angle),
                      major ? white(alpha * 0.73f) : gray(alpha * 0.49f), major ? 1.25f : 1.0f);
    }
    for (int quadrant = 0; quadrant < 4; ++quadrant) {
        const float start = static_cast<float>(quadrant) * pi * 0.5f + 0.20f;
        arc(draw, center, radius * 0.76f, start, start + pi * 0.5f - 0.40f, gray(alpha * 0.32f));
        arc(draw, center, radius * 0.55f, start + 0.12f, start + pi * 0.5f - 0.52f,
            white(alpha * 0.18f));
        const float angle = static_cast<float>(quadrant) * pi * 0.5f;
        draw->AddLine(polar(center, radius * 0.23f, angle), polar(center, radius * 0.29f, angle),
                      white(alpha * 0.36f));
    }

    const float scan = cycle(phase, 0.085f, 0.10f) * pi * 2.0f;
    arc(draw, center, radius * 0.83f, scan, scan + 0.47f, red(alpha * 0.82f), 1.7f);
    arc(draw, center, radius * 0.83f, scan - 0.23f, scan, red(alpha * 0.25f), 1.0f);
    const float blipAngle = cycle(phase, -0.043f, 0.66f) * pi * 2.0f;
    const ImVec2 blip = polar(center, radius * 0.76f, blipAngle);
    draw->AddCircleFilled(blip, 3.3f, red(alpha * 0.16f), 12);
    draw->AddCircleFilled(blip, 1.45f, white(alpha * 0.88f), 8);
    draw->AddLine(polar(center, radius * 0.88f, scan + 0.47f),
                  polar(center, radius * 0.96f, scan + 0.47f), red(alpha * 0.67f), 1.3f);
    draw->PopClipRect();
}

void signalStrip(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                 float alpha) {
    alpha = unit(alpha);
    if (!draw || alpha <= 0.0f || !validRect(minimum, maximum))
        return;
    const float width = maximum.x - minimum.x, height = maximum.y - minimum.y;
    if (width < 18.0f || height < 5.0f)
        return;
    phase = motion && std::isfinite(phase) ? phase : 0.0f;
    // The bounded phase avoids precision loss during long-running observatory sessions.
    const float time = cycle(phase, 0.16f) * pi * 2.0f;
    const float cell = width / 36.0f;
    const float gap = cell * 0.36f;
    const float baseline = maximum.y - 1.0f;
    draw->PushClipRect(minimum, maximum, true);
    draw->AddLine({minimum.x, baseline}, {maximum.x, baseline}, gray(alpha * 0.29f));
    for (int bar = 0; bar < 36; ++bar) {
        const float index = static_cast<float>(bar);
        const float wave = std::sin(index * 0.57f - time) * 0.5f + 0.5f;
        const float detail = std::sin(index * 1.79f + time * 2.0f) * 0.5f + 0.5f;
        const float level = 0.15f + 0.61f * wave + 0.20f * detail;
        const float x = minimum.x + cell * index + gap * 0.5f;
        const float top = baseline - (height - 2.0f) * level;
        const bool accent = bar % 9 == 0 || bar % 9 == 1;
        draw->AddRectFilled({x, top}, {x + cell - gap, baseline},
                            accent ? red(alpha * 0.73f) : gray(alpha * (0.35f + level * 0.35f)));
        draw->AddRectFilled({x, top}, {x + cell - gap, std::min(top + 1.2f, baseline)},
                            accent ? white(alpha * 0.76f) : white(alpha * 0.35f));
    }
    draw->PopClipRect();
}

} // namespace sse::ui
