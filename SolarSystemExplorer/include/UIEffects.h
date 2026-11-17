#pragma once

#include <imgui.h>

namespace sse::ui {

// Draw-only effects: callers retain text layout, interaction, and clipping.
// Phase is a continuously increasing time value; text positions never animate.
void spectrumText(ImDrawList* draw, ImFont* font, float fontSize, ImVec2 position, const char* text,
                  float phase, bool motion, float alpha = 1.0f);
void lightSweep(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                float strength = 1.0f);
void orbitSparkles(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                   float strength = 1.0f);
void technicalFrame(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                    float alpha = 1.0f);
void trackingReticle(ImDrawList* draw, ImVec2 center, float radius, float phase, bool motion,
                     float alpha = 1.0f);
void signalStrip(ImDrawList* draw, ImVec2 minimum, ImVec2 maximum, float phase, bool motion,
                 float alpha = 1.0f);

} // namespace sse::ui
