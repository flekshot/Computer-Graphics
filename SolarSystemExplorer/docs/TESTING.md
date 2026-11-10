# Verification report

## Interface and Workshop update - 2026-11-10

The current **1.2.0** source was built with Visual Studio 2026 / MSVC 19.51, Release, in `build`. The complete suite passes **13 / 13 CTest tests**:

```powershell
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

- CPU: Core 10,492 assertions; orbit/persistence 875; eclipse 62; workshop geometry 7,971; mission predicates 67.
- OpenGL: context, original graphics smoke, laboratory smoke, workshop smoke, missing textures, bloom disabled, invalid CLI, and resource recovery all pass.
- New regressions cover continuous cloud rotation, upright portrait eclipse projection, repeated Orbit Lab navigation, and overview selection preservation.
- Workshop smoke exercises actual ImGui navigation, independent time, keyboard pause, terrain rebuilds, gas and diagnostic modes, persistence/challenge progress, compact/portrait resize, renderer switching, and the real Motion checkbox. It checks the rendered emblem vertices remain stationary with decorative motion disabled.
- Captures of Explore, Earth, Saturn, Orbit Lab, normals, solar/lunar eclipses, rocky/gas Workshop and compact layouts were rendered by the application and inspected. All capture processes exited successfully with zero GL errors. At 1000x640 all eight destination entries fit; long inspector/workshop panels scroll.
- Both added version 2 example files were loaded and rendered. Existing version 1 examples remain covered by loading and persistence tests. An independent read-only review found no Critical/Important issues; its minor duplicate mission predicate evaluation was removed.
- The reference-driven HUD follow-up was rebuilt and again passed **13/13**. Native effects add angular frames, scanning rings, signal strips and flowing title colors. Actual title vertex colors change with Motion on and remain fixed with Motion off.
- The two bundled internet GIFs decode completely (radar: 121 frames; Earth: 44 frames). Workshop smoke reads back GPU pixels to verify frame advancement, frozen playback and caller texture-state preservation. Resource recovery verifies both GIFs after relocation, a corrupt-GIF fallback, and safe rejection of a valid three-frame disposal-method-3 fixture before it reaches the bundled decoder's unsupported path. A focused independent review confirmed the guard and found no remaining Critical/Important issues.

This is a Release verification of the current workstation. It does not add new cross-GPU, long-duration memory, native file-picker or OS minimize/restore claims. The older reports below retain their original scope and dates.

## Laboratory update — 2026-11-04

Version **1.1.0** was built in the integrated workspace using **Visual Studio 2026 / MSVC 19.51**, Release configuration. The fresh integrated run passed **10 / 10 CTest tests**: the original seven tests plus three laboratory tests.

```powershell
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

These commands refer to the workspace's `build` directory. With a named CMake preset, use its matching build and test presets instead.

| Test group | Recorded result / coverage |
| --- | --- |
| Original seven tests | All passed: `core_behaviors`, `opengl_context`, `graphics_smoke`, `missing_textures`, `no_bloom`, `invalid_cli`, and `resource_recovery`. Core coverage remains 10,481 assertions. |
| `orbit_and_persistence` | 345 assertions passed, covering elliptical position and velocity, orbital bounds/reset, experiment roundtrips, malformed data, and state preservation on failed loading. |
| `eclipse_geometry` | 62 assertions passed, including alignment/occlusion cases and comparison of angular-circle overlap against numerical spherical-cap integration. |
| `laboratory_smoke` | Completed the 240-frame integrated scenario through orbit editing, graphics modes, persistence, and eclipse views. |

The default circular eight-planet behavior remains covered by the original suite. The new orbit tests check Keplerian ellipses and analytic velocity; the eclipse tests validate the illustrative occlusion model, not real-calendar eclipse predictions. Invalid experiment input is checked before it can replace live model/settings state.

This update records the integrated **Release** build and automated checks. It does not claim a new Debug run, benchmark/FPS measurement, memory soak, or completed manual interaction with every native file-dialog path. Save/load model behavior and application integration are covered by the tests above; native picker interaction is a separate manual check. The older environment, native-window observations, benchmark, and memory measurements below apply to their original date and must not be read as new laboratory measurements.

## Historical report — 2026-10-21

Date: **2026-10-21**. These are results of actual local builds and executions, not predicted outcomes.

### Environment

- Windows 11 Pro x64, build 26200; installed Visual Studio Enterprise 2026, MSVC 19.51.36256.
- CMake 4.4.3; Windows SDK 10.0.26100.0.
- NVIDIA GeForce RTX 4050 Laptop GPU.
- Created OpenGL **3.3 Core** context; driver reports `3.3.0 NVIDIA 610.47`.
- Source and executable resources are local. No runtime network access is used.

VS 2022 presets and CMake 3.24-compatible project configuration are supplied. The installed compiler is VS 2026; a VS 2022 build was not independently executed here. Windows 10, other GPUs, other drivers and remote desktop sessions were not separately tested.

### Automated tests

Run:

```powershell
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Both **Release and Debug** configurations were built and each passes **7 / 7 CTest tests**:

| Test | Real behavior exercised |
|---|---|
| `core_behaviors` | 10,481 assertions: all eight planets, orbital closure and half-period positions, stable radius, pause, speed, delta-time partition independence, invalid deltas, lunar separation, unit/outward normals, UV range, CCW winding, invalid tessellation, smooth focus, exact follow, zoom safety, pitch limits, overview, collision-crossing focus, 100× Mercury tracking, and overview framing at two window sizes |
| `opengl_context` | Initializes GLFW/GLEW, compiles and links every shader, loads maps/fonts, allocates GPU resources and renders three real frames |
| `graphics_smoke` | Runs 360 frames through the real application; injects a planet key while ImGui has focus, opens the actual speed slider's text editor and cancels it with Escape, exercises pause and 100× speed, focuses all planets, changes window dimensions, checks zero-size render-target guard, orbit/zoom/pan, feature toggles and overview |
| `missing_textures` | Forces all 14 maps to procedural fallbacks and renders successfully |
| `no_bloom` | Exercises the render path with bloom disabled |
| `invalid_cli` | Rejects a non-finite speed argument with a nonzero exit code |
| `resource_recovery` | Copies executable/resources into `relocated test Жоба 星`, launches from a different directory with zero fallbacks, corrupts Earth's map and observes exactly one fallback, corrupts a shader and checks compiler diagnostics, and checks a missing-shader error |

The application drains and checks `glGetError()` after every rendered frame. The final verified render runs report **0 OpenGL errors**. GLEW's context-initialization probing errors are cleared immediately after `glewInit`, before scene setup.

### Regression evidence

Three important problems were reproduced and fixed during development:

1. **Initial UI focus swallowed destination keys.** The added integration assertion failed with `startup UI swallowed the planet keyboard shortcut`. Adjusting ImGui navigation capture and preserving active-widget ownership made it pass.
2. **A camera collision permanently changed tracking center.** A Mars-to-Venus flight failed the new settling assertion. Collision correction now enters the decaying transition offset instead of permanent user pan. The test passes, including Uranus-to-Mercury and high-speed Mercury tracking.
3. **Escape from slider editing exited the app.** With the fix deliberately disabled, the integration test fails with `Smoke scenario exited before all assertions ran`. Restoring input ownership checks makes the complete scenario pass.

Unicode relocation also failed before its fix with a Windows character-conversion exception. Paths now use the wide Windows entry point and UTF-8-aware resource APIs. The relocation test passes with all original textures loaded.

### Visual and native-window checks

Actual framebuffer captures were inspected for overview, Earth/Moon and Saturn. These are stored in `screenshots/`; they are outputs of the application, not concept renders. They show textured surfaces, visible illumination, solar glow, orbit lines, star field, labels, cloud/atmospheric layers and Saturn's ring transparency/shadow.

A 1000×640 capture was also inspected. The navigation rows adapt so all eight planets remain selectable, and the information panel scrolls when its content is taller than the window. The overview camera adjusts to keep the destination centers between side panels.

The visible native application was launched and observed around **144 FPS** with VSync on this 144 Hz display. Planet selection with **3**, **Space** pause and **Escape** exit were exercised. The initial selection issue was observed through native input before being fixed and verified again.

Further native automation was interrupted by external user input. Remaining checks therefore used the separate hidden integration build. A complete manual exercise of every mouse gesture and every keyboard shortcut, and an actual OS minimize/restore cycle, were not claimed. The zero-framebuffer guard is exercised directly in the renderer; the application's minimize branch was reviewed but not independently driven by the OS test.

### Stress run and memory observations

Command:

```powershell
.\SolarSystemExplorer.exe --hidden --benchmark --frames 12000 --speed 100
```

Result on the recorded machine, at the default 1440×900 size:

```text
Completed 12000 frames in 22.3607 s; average 536.656 FPS; GL errors: 0
```

The hidden benchmark disables VSync and uses fixed simulation steps. This measures this workload on this machine; it does not predict the FPS of another laptop. Loading is outside the reported frame-loop duration; final GPU completion is included.

Process memory was sampled every approximately 200 ms. Private memory stabilized at about **333.5 MiB** in the final samples after an approximately **442.4 MiB** initialization peak. No unbounded growth was observed in this short run. This is not a formal CPU/GPU leak proof, a long soak test or a driver allocation audit. GPU ownership and constructor-unwinding paths were also checked in an independent read-only code review.

### Acceptance mapping

| Required result | Implementation / evidence |
|---|---|
| Application starts | Executed Release binary and `opengl_context` |
| Sun and all eight planets | `SolarSystem`, `Renderer`, inspected overview |
| Orbit and axial motion | Period tests, time tests and planet model matrices |
| Camera, selection and smooth approach | Core tests, native key check and full integration scenario |
| Lighting and textures/materials | GLSL programs, original map runs and fallback run |
| Stars, Saturn's rings, Moon | Rendered captures and real geometry/model implementation |
| Pause and time-speed control | CPU tests, native Space and UI integration tests |
| Resize and aspect ratio | Integration resize, framebuffer allocation and compact capture |
| Build instructions | Root README, CMake presets and BuildAndRun.bat |
| Defense documentation | PROJECT_EXPLANATION.md and DEMO_SCRIPT.md |
| Class-based architecture | Separate model, camera, resource, renderer, UI and application modules |
| No intentionally nonfunctional methods | Authored source scan, compile/link, core/integration tests and independent review |

All mandatory requested features are implemented. Circular prescribed orbits, exaggerated scale, slowed axial spin and simplified lighting are deliberate educational approximations, documented in the UI and README. There are no remaining known Important/Critical review findings.

Detailed build, regression, stress and memory artifacts are local verification output and are excluded from the source repository. This report summarizes the measured results; raw artifacts may contain local workspace paths.

The `cmake --install` portable folder was launched from a different working directory and completed the entire 360-frame smoke scenario with zero texture fallbacks and zero OpenGL errors. `dumpbin /dependents` lists only Windows system DLLs (`OPENGL32`, `KERNEL32`, `USER32`, `GDI32`, `SHELL32`, `IMM32`); no separate GLFW/GLEW/ImGui or Visual C++ runtime DLL is needed.
