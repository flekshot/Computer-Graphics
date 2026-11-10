# Fixes and Interface Refresh Implementation Plan

> **For agentic workers:** Execute the independent file ownership tasks in parallel, then integrate and review the whole change.

**Goal:** Restore a buildable application and deliver the requested fresh, animated native interface.

**Architecture:** Preserve C++17/OpenGL 3.3 and Dear ImGui. Keep workshop geometry/missions and persistence in CPU-only modules; integrate a separate workshop renderer/camera. Session-only UI motion does not change the saved experiment schema.

**Tech Stack:** C++17, OpenGL 3.3, bundled GLFW/GLM/GLEW/Dear ImGui, CMake/MSVC.

**Spec:** `docs/superpowers/specs/2026-11-10-interface-refresh-design.md` and the existing `2026-11-04-planet-workshop-design.md`.

## Global Constraints

- Remain offline with no additional dependencies; preserve existing Explore/Orbit/Eclipse behavior.
- Fit 1000x640 and 1440x900; keep hit targets stable during transitions.
- Save SSELAB 2, read SSELAB 1 and 2; validate fully before mutation and preserve atomic writes.
- Do not commit or publish; leave changes reviewable in the user's existing feature branch.

## Review Focus

- Invalid files must preserve every live field; cover with LabTests.
- Repeated mode selection and every overview action must preserve intended selection/settings; cover with real UI smoke clicks.
- Compact and portrait windows must keep controls reachable and projections finite; cover with smoke resize and captured layouts.
- Animation disabled must preserve interaction and stop decorative motion; inspect through UI smoke and captures.
- Switching renderers must restore GL state and keep independent clocks/cameras; cover workshop/laboratory smoke.

## Tasks

- [x] CPU worker: implement `WorkshopModel.h/.cpp`, `Missions.h/.cpp` and meaningful deterministic geometry/predicate tests, red then green. Produce APIs agreed in worker brief.
- [x] Persistence worker: make existing LabTests pass with strict SSELAB 2 and legacy compatibility; run the isolated test before and after the fix.
- [x] View worker: implement `WorkshopRenderer.h/.cpp`, workshop shaders and `WorkshopUI.cpp`; consume the agreed model/mission APIs and root-owned UIManager declarations.
- [x] Root: add regression coverage for cloud continuity, portrait projection and overview/repeated-mode interaction. Repair the duplicate GLEW declaration, integrate workshop lifecycle/CLI and implement the UI design and motion control.
- [x] Root: build Release, run the whole CTest suite, inspect screenshots and fix demonstrated failures. Run one independent review, resolve material findings, update concise usage/testing evidence.

## Progress

- Baseline: full CMake generation fails for missing Workshop sources. Isolated CPU tests: CoreTests and EclipseTests pass; LabTests fails at assertion 87 saving Workshop.
- Work occurs in the existing `feat/planet-workshop` checkout so the user's active IDE sees the results. The only pre-existing edit duplicates a GLEW global declaration; the task requires removing that duplicate to compile.

- Final verification: Release build successful without new compiler warnings; CTest 13/13 passed. Reviewed all four modes and compact layouts. Independent review clean after removing duplicate mission evaluation.

