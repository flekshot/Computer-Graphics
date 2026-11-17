# Solar System Lab Implementation Plan

> For agentic workers: use focused parallel tasks with explicit file ownership, integration checks, and a fresh final review.

**Goal:** Implement the user-approved interactive orbit and graphics laboratories, persistent experiments, and eclipse observatory.

**Architecture:** CPU orbit/eclipsing models and file persistence are independent of OpenGL. Separate renderers consume shared settings; the existing application and ImGui UI coordinate modes.

**Tech Stack:** C++17, OpenGL 3.3, GLSL 330, GLFW, GLM, Dear ImGui, CMake, Windows native dialogs.

**Spec:** docs/superpowers/specs/2026-11-04-solar-system-lab-design.md

## Global constraints

- Windows x64, C++17, OpenGL 3.3 Core, existing vendored dependencies only.
- Preserve default exploration and four-person project credits.
- Respect explicit file ownership; leave third-party sources and licenses untouched.
- Current workspace on feature branch; leave changes reviewable without an automatic commit/push.

## Review focus

- Invalid/non-finite orbit parameters and Sun intersections must leave the model unchanged.
- Malformed/truncated/version-mismatched experiment files must not partially apply settings or overwrite valid saves on error.
- GL polygon mode, scissor, viewport, depth, and blend state must survive mode switching and resizing.
- UI keyboard/mouse capture and compact layouts must not break existing controls.
- Eclipse shadow direction, angular disk overlap, and observer placement must work for total, partial, and clear cases.

## Task 1 — Orbit model and persistence

Owner: orbit_model agent. Files: Planet.h/.cpp, SolarSystem.h/.cpp, ExperimentIO.h/.cpp, tests/LabTests.cpp.

- [x] Add tests for circular compatibility, periapsis/apoapsis, period closure, faster periapsis motion, velocity, validation, reset.
- [x] Implement OrbitParameters, Kepler solving, analytical position/velocity, safe editing.
- [x] Add versioned persistence and roundtrip/corruption/atomic-load tests; include shared LabState.h settings.
- [x] Run LabTests and CoreTests after integration into CMake.

Interfaces are exactly those in the spec. Supported editing ranges: semiMajorAxis 6..180 scene units, eccentricity 0..0.85, inclination -60..60 degrees, period 5..200000 days, finite phase within -3600..3600 degrees. Periapsis must exceed Sun radius + planet radius + 0.4. Existing default orbits need not be re-derived.

## Task 2 — Graphics laboratory

Owner: graphics_lab agent. Files: Renderer.h/.cpp, Mesh.h/.cpp if required, existing shaders and new debug shaders.

- [x] Preserve default rendering; implement elliptical orbit paths consuming OrbitParameters.
- [x] Implement shaded/wireframe/world-normal modes and ambient/diffuse/specular multipliers.
- [x] Draw selected-planet trail, velocity arrow, apsis markers, and normal vectors without per-frame GPU object creation.
- [x] Provide meaningful rendering checks and identify capture scenarios for integration.

Consumes LabState.h RenderSettings and Planet interfaces. Does not modify Application, UI, CMake, or model files.

## Task 3 — Eclipse laboratory

Owner: eclipse_lab agent. Files: EclipseModel.h/.cpp, EclipseRenderer.h/.cpp, shaders/eclipse* and tests/EclipseTests.cpp.

- [x] Test eclipse disk-overlap math and total/partial/clear solar/lunar arrangements.
- [x] Implement a consistent illustrative scene and mathematical visibility calculation.
- [x] Render overview plus target/observer views, shadow and penumbra cones; preserve GL state.
- [x] Verify CPU tests and supply presets/labels for the main UI.

Consumes LabState.h and existing Mesh/Shader/Texture helpers. EclipseRenderer interface is fixed in the spec.

## Task 4 — UI and integration

Owner: main agent. Files: LabState.h, UIManager.h/.cpp, LabUI.cpp, Application.h/.cpp, main.cpp, ExperimentDialogs.h/.cpp, Camera if required, CMakeLists.txt.

- [x] Provide shared settings before parallel implementations.
- [x] Integrate top mode navigation, orbit/graphics/eclipse panels, time seeking, reset, and status messages.
- [x] Integrate Unicode native .sselab file dialogs and atomic load/apply with camera refocus.
- [x] Route lab rendering and input, add deterministic CLI capture options and automated lab smoke scenarios.
- [x] Build and run regression/feature tests; visually inspect 1440x900 and 1000x640 captures.

## Task 5 — Review and delivery

- [x] Fresh independent review of the complete diff; fix substantive findings and rerun relevant checks.
- [x] Update README, application guide, demo script, testing notes, and current screenshots.
- [x] Build an updated portable folder and archive with matching sources; keep generated packages ignored.
- [ ] Report implemented controls and observed verification results.

## Progress

- Initial repository clean; feature branch feat/solar-system-lab created in the current workspace.
- Visual Studio 2026 / MSVC 19.51 Release build succeeded; baseline seven tests and final expanded ten-test suite passed.

## Final verification - 2026-11-04

- Final Release build succeeded after explicit CLI planet-selection precedence fix.
- All 10 CTests passed in 6.14 seconds, including protected temporary-file and Orbit/Graphics tab regressions.
- Reviewer approved the two original findings and final CLI selection change; no material findings remain.
- Orbit, normals, solar and lunar captures inspected at 1440x900 and 1000x640; side panels scroll at compact size. Solar/lunar observer captures also inspected. All capture runs reported zero OpenGL errors.
- Native manual-click and Windows file-dialog checks remain unverified: the computer-use native pipe was unavailable; the alternate app-control attempt did not complete. Integration tests exercise ImGui mode buttons.
- Portable installation passed the 240-frame laboratory smoke test with zero OpenGL errors and zero missing-texture fallbacks.
- Four lab screenshots and three example experiments ship in both updated packages. Archive verification matched every entry byte-for-byte against current intended input files: 674 source entries and 68 portable entries; build products and old verification logs excluded.
- Root SHA256SUMS.txt records both new archive hashes. Generated packages remain ignored; no commits or pushes performed.
