# Review fixes and visual polish

> **For agentic workers:** Use superpowers:subagent-driven-development or superpowers:executing-plans. The user approved fixing the reviewed errors first, then improving the visual presentation. Work in the current shared checkout; do not commit or close the user's application.

**Goal:** Correct the four demonstrated interaction/rendering defects, verify them, then improve the existing observatory interface.

**Architecture:** Retain C++17, OpenGL 3.3 and Dear ImGui. Keep scene state separate from session-only presentation preferences. Preserve the graphite/red HUD, local GIF assets and reduced-motion setting; do not restore large targeting rings.

**Spec:** The review and user approval in this conversation, following the existing interface-refresh design.

## Constraints and review focus

- Escape from a popup must consume that press, including when ImGui closes the popup during NewFrame; ordinary Escape outside UI still exits.
- Workshop geometry edits preserve the chosen viewing angle/pan and safe relative zoom; explicit Reset camera still focuses.
- Orbit-slider endpoints remain inside model validation limits for Mercury and Mars.
- Normal diagnostic colors do not depend on exposure, bloom or vignette.
- Compact controls remain reachable at 1000 x 640; decorative motion can be stopped; clean view has an obvious way back.

## Phase 1: correctness

- [x] Add regressions for popup/modal Escape, camera-preserving radius updates and eccentricity endpoints; observe failure before implementing the fixes.
- [x] Fix input routing in Application, camera safety updates in Camera, and slider rounding in LabUI.
- [x] Add a real framebuffer regression for normal diagnostics, then bypass artistic processing in that mode.
- [x] Build Release and run the complete CTest suite before beginning visual changes.

## Phase 2: visual presentation

- [x] Split Workshop tools into compact Shape, Surface and Atmosphere tabs; put frequently used controls near the top and retain all existing settings.
- [x] Make scrollbars and planet labels easier to see; keep colored markers and use bright label text.
- [x] Add a reversible clean-view presentation and optional reference-feed cards; keep the existing animation identity.
- [x] Fade unrelated orbit lines as the camera approaches a selected planet while retaining educational overlays in Orbit Lab.
- [x] Inspect the feasibility of shared antialiasing/linear composition for the laboratory renderers and interface scaling; implement coherent changes within the existing renderer/layout boundaries.
- [x] Capture and inspect Explore, Workshop and Eclipse at compact and normal sizes; run relevant regression checks and the complete suite after integration.
- [x] Update user documentation/screenshots, build a portable folder, and leave any running portable executable untouched.

## Validation

Use cmake --build SolarSystemExplorer/build --config Release --parallel, then ctest --test-dir SolarSystemExplorer/build -C Release --output-on-failure. Use hidden deterministic captures for visual comparisons. Do not infer interactive smoothness from uncapped hidden-window FPS.

## Completion evidence

Release CTest: 13/13 passed. Portable Workshop and Eclipse both initialized with two HUD animations and zero OpenGL errors. Captures were refreshed and visually inspected at 1440 x 900 and 1000 x 640; the UI integration scenario verifies 150% scaling and scaled mouse navigation. Both portable folders are updated. ZIP archives were not regenerated.
