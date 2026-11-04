# Solar System Explorer Implementation Plan

> Execution: superpowers:executing-plans, continuously in the current workspace as requested.

**Goal:** A complete, offline-running C++17 OpenGL 3.3 university project for Windows, with eight planets, the Earth-Moon hierarchy, an orbit camera, a readable ImGui interface and defense documentation.

**Architecture:** CPU-only Planet/SolarSystem/Camera/geometry modules are independent of OpenGL. RAII Shader/Texture/Mesh/Renderer modules own GPU resources. Application owns the GLFW lifetime and routes input; UIManager presents controls and astronomical facts.

**Tech Stack:** C++17, CMake, GLFW 3.4, GLEW 2.2.0, GLM 1.0.1, Dear ImGui 1.91.9b, stb_image/stb_image_write, GLSL 330.

**Spec:** `docs/USER_REQUIREMENTS.txt` (the original supplied request).

## Global constraints and design decisions

- Modern OpenGL only: VAO/VBO/EBO, GLSL, no fixed-function calls.
- Work in this new project directory; no existing repository or branch is being changed.
- All eight planets use real relative orbital periods with compressed distances and exaggerated radii. Orbits are circular, slightly inclined, and phases are illustrative, not an ephemeris.
- Simulation time is double precision, in days; the default is 6 days per real second. Axial rotation is deliberately slowed for readability and documented.
- Local CC BY 4.0 surface maps, with deterministic procedural fallback for missing maps; bundled dependencies make the delivered source buildable offline.
- Visual direction: near-black space, warm solar gold, ice-blue interaction accents, quiet translucent panels, crisp typography, HDR bloom and atmospheric rims.
- Selection immediately starts a smooth focus; F refocuses, WASD pans, 0/R restore overview. UI input must not leak into camera input.
- GLFW context outlives every GPU resource. Missing essential shaders fail with a readable error.

## Review focus

1. Minimized/zero framebuffer dimensions must skip rendering without invalid projection or framebuffer allocation.
2. Missing/corrupt textures must produce a usable fallback; relocated builds must find resources beside the executable.
3. Following fast inner planets at high time scales must not lag into a planet; zoom limits must account for the selected body and Saturn's rings.
4. UI mouse/keyboard capture must prevent camera motion and accidental global shortcuts.
5. Failure during shader/FBO initialization must release already-created resources before GLFW termination.

## Tasks

### 1. Build and context
- [x] Vendor pinned dependencies with licenses; create CMake, presets, launch scripts and a GLFW core-context application.
- [x] Verify configure/build and run a short context test that reports the GL renderer/version.

### 2. CPU model, geometry and camera
Files: `include/{Planet,SolarSystem,Camera,Geometry}.h`, corresponding `src/*.cpp`, `tests/CoreTests.cpp`.
Interfaces: `Planet::position(double days)`, `Planet::modelMatrix(double days)`, `SolarSystem::update(double seconds)`, `Camera::update(float dt, glm::vec3 followedPosition)`, `makeSphere(unsigned segments, unsigned rings)`.
- [x] Add tests for period closure, pause/speed, frame partition independence, moon relative distance, finite camera matrices and mesh normals/winding.
- [x] Define the behavior tests before implementation; implement the CPU model, compile and run the complete core suite. The initial baseline could not execute before the modules existed; later bug regressions were verified with observed failing/passing runs.

### 3. Rendered scene
Files: `Shader`, `Texture`, `Mesh`, `Renderer`, GLSL programs and assets.
- [x] Build indexed UV spheres, orbit lines, a two-sided annulus, and a deterministic star field.
- [x] Add point-source Blinn-Phong lighting, emissive Sun, Earth clouds/night lights, atmosphere and Saturn/ring mutual shadows.
- [x] Add HDR scene framebuffer, downsampled ping-pong bloom and tone mapping.
- [x] Run the application and inspect an actual framebuffer capture; check all GL errors.

### 4. Interaction and interface
Files: `Application`, `UIManager`, CLI entry point.
- [x] Route all required shortcuts, drag/zoom/pan, focus tracking, pause/speed and visibility controls.
- [x] Add top bar, planet navigation, facts panel, timeline controls, projected labels and help.
- [x] Add reproducible capture and smoke-test modes exercising resize, all selections, follow, pause, speed and rendering switches.
- [x] Verify screenshots at overview/Earth/Saturn, input behavior, missing textures and resource relocation.

### 5. Delivery
- [x] Create README, project explanation, 3–5 minute demo script, asset credits and testing report.
- [x] Perform a fresh independent code review and resolve significant findings.
- [x] Build/test Release and Debug, package portable Windows application and source archive, save verification evidence.

## Execution ledger

- 2026-10-21: Empty workspace confirmed. Visual Studio 2026 MSVC 14.51, CMake 4.4.3 and NVIDIA RTX 4050 available. The source will also target VS 2022; only the installed toolchain can be verified here.
- Ruling: Proceed directly under the supplied detailed specification and explicit instruction to plan and immediately implement; no additional design-approval gate.
- Ruling: Keep the project and deliverables under `outputs/SolarSystemExplorer`; build directories and acquisition scripts under `work/`.
- Tasks 1–4: complete. Context checkpoint passed; full scene, input, camera, rendering and UI subsequently built and tested.
- Final review: independent read-only reviewer found two Important bugs. Collision tracking and Escape ownership were fixed, with regression evidence in `docs/TESTING.md`.
- Validation: Release 7/7, Debug 7/7; 10,481 core assertions; 12,000-frame stress run with zero OpenGL errors. Unicode relocation and corrupt resources pass.
- Visual QA: overview/Earth/Saturn and compact layout inspected. Overview was adjusted to keep Neptune visible; MSAA added; compact navigation fits all eight destinations.
- Task 5: complete. README, explanation, demo, licenses, source inventory, logs and portable/source archives delivered. Actual OS minimize/restore and VS 2022 remain explicitly unverified; see TESTING.md.
