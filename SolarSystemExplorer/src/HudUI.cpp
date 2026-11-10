#include "UIManager.h"
#include "AnimatedTexture.h"
#include "UIEffects.h"
#include <imgui.h>
#include <algorithm>

namespace sse {
void UIManager::hudOverlay() {
    const auto display = ImGui::GetIO().DisplaySize;
    const float left = (display.x < 1200 ? 202.0f : 222.0f) + 48;
    const float right = display.x - (display.x < 1200 ? 248.0f : 284.0f) - 48;
    const float top = 204, bottom = display.y - 120;
    const float width = right - left, height = bottom - top;
    if (width <= 120 || height <= 120)
        return;
    auto* draw = ImGui::GetBackgroundDrawList();
    const float phase = ambientPhase_ * 2.2f;
    const float alpha = .65f + .35f * reveal_;
    draw->PushClipRect({left, top}, {right, bottom}, true);

    // The feeds are labelled reference/interface loops, not live astronomical data.
    if (display.y >= 760 && width >= 560) {
        const float feedWidth = 184, feedHeight = 96, feedTop = bottom - feedHeight;
        auto feed = [&](float x, AnimatedTexture* animation, const char* title, const char* caption, bool red) {
            const ImVec2 minimum(x, feedTop), maximum(x + feedWidth, bottom);
            draw->AddRectFilled(minimum, maximum, IM_COL32(4, 6, 9, 220));
            ui::technicalFrame(draw, minimum, maximum, phase, motion_, alpha * .8f);
            draw->AddText(body_, 12, {x + 10, feedTop + 8}, IM_COL32(215, 222, 232, 235), title);
            if (animation && animation->ready())
                draw->AddImage(static_cast<ImTextureID>(animation->id()), {x + 9, feedTop + 28},
                               {x + 69, feedTop + 88}, {0, 0}, {1, 1},
                               red ? IM_COL32(255, 108, 118, 240) : IM_COL32(220, 233, 248, 240));
            else
                ui::trackingReticle(draw, {x + 39, feedTop + 58}, 27, phase, motion_, .8f);
            draw->AddText(body_, 12, {x + 80, feedTop + 32}, IM_COL32(168, 179, 195, 255), caption);
            ui::signalStrip(draw, {x + 80, feedTop + 55}, {x + feedWidth - 12, feedTop + 80}, phase,
                            motion_, .75f * alpha);
        };
        feed(left + 6, radarAnimation_.get(), "RADAR DISPLAY", "Sweep loop", true);
        feed(right - feedWidth - 6, globeAnimation_.get(), "EARTH ROTATION", "Reference loop", false);
    }
    draw->PopClipRect();

    // A thin signal band keeps the scene readable at the compact window size.
    const float stripLeft = left + std::max(0.0f, width * .5f - 110);
    ui::signalStrip(draw, {stripLeft, display.y - 117}, {stripLeft + std::min(220.0f, width), display.y - 105},
                    phase, motion_, .42f);
}
} // namespace sse
