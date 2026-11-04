# Solar System Explorer

**Interactive 3D Solar System Simulation — Computer Graphics Fundamentals final project**

A native C++17 / OpenGL 3.3 application for Windows. Explore all eight planets, follow Earth and its Moon, inspect Saturn's rings, and control time from a compact Dear ImGui interface. The entire application runs offline. Library source code, licenses, fonts and all 14 texture maps are included.

![System overview](docs/screenshots/overview.png)

## Start here

If you have the portable Windows distribution, extract the **whole ZIP** and open `SolarSystemExplorer.exe`. Keep the `assets`, `shaders` and `licenses` folders beside it. Nothing needs to be downloaded at runtime. The native C/C++ runtime and project libraries are linked statically; Windows system graphics libraries and a working OpenGL driver are still required.

If you have the source distribution, open this folder in Visual Studio or VS Code, or follow the commands below. `BuildAndRun.bat` configures, builds, tests and launches using the locally available Visual Studio toolchain.

## Build from source

Requirements:

- Windows 10/11, x64.
- Visual Studio 2022 with **Desktop development with C++** and a Windows SDK. VS 2026 is also supported; it requires a CMake version supporting that generator.
- CMake 3.24 or newer for VS 2022.
- A GPU driver providing OpenGL 3.3 Core.

No vcpkg installation, Python, Node.js, package manager, Git checkout, network connection or environment-specific library path is required. The dependency versions are fixed in `third_party/MANIFEST.json` and their unmodified source is bundled.

From this directory in PowerShell:

```powershell
cmake --preset vs2022
cmake --build --preset release --parallel
ctest --preset release
.\build\vs2022\Release\SolarSystemExplorer.exe
```

Equivalent explicit commands:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\Release\SolarSystemExplorer.exe
```

For VS 2026 use `--preset vs2026`, then build/test preset `release-vs2026`. The actual validation environment for this delivery is recorded in `docs/TESTING.md`; VS 2022 was not installed on that machine.

In **Visual Studio**, choose *Open a local folder*, select this folder and pick the matching CMake preset. Select `SolarSystemExplorer.exe` as startup target. In **VS Code**, use the Microsoft C/C++ and CMake Tools extensions, select the preset, then configure, build and run the `SolarSystemExplorer` target.

To create a portable folder yourself:

```powershell
cmake --install build --config Release --prefix portable
.\portable\SolarSystemExplorer.exe
```

Resources are copied next to the executable by the build. Shader edits require rebuilding that target to update the copy, or running with `--assets .` from this source directory. The executable resolves its own directory first, so it can be launched from another working directory. Unicode Windows paths are supported.

## Controls

| Input | Action |
|---|---|
| Left mouse drag over space | Orbit around the current camera center |
| Mouse wheel over space | Smooth zoom, with a minimum safe distance |
| W / A / S / D | Pan the camera center |
| 1–8 or a planet in the left panel | Select, smoothly approach and follow a planet |
| F | Refocus the selected planet from its illuminated side |
| 0 / R / Overview / Reset camera | Return to the system overview |
| Space / Pause / Resume | Stop or resume simulation time |
| + / − | Increase or decrease time speed |
| Speed slider | Set 0.05×–100×; 1× is 6 simulated days per second |
| Orbits / Labels / Bloom | Toggle the corresponding render feature |
| H | Show or hide the flight manual |
| Escape | Exit; while editing a numeric UI value, cancel that edit first |

UI interaction is kept separate from camera input. Tab and arrow keys can navigate interface widgets. Panning while following adds a user offset; F clears that offset. Stars twinkle in real time even while planetary time is paused.

## Implemented features

- Sun, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus and Neptune, each with unique texture/material, orbital radius, period, axial spin, tilt and facts.
- Procedurally generated indexed UV spheres with outward normals; reusable VAO/VBO/EBO geometry.
- Circular inclined orbits evaluated mathematically from a double-precision day counter; no frame-specific hardcoded positions.
- A hierarchical, synchronously rotating Moon attached to Earth's translation, independent of Earth's surface spin.
- Camera transitions using exponential smoothing and logarithmic distance interpolation; exact translation tracking after focus; collision correction and zoom limits.
- Blinn-Phong ambient/diffuse/specular shading with a point light at the Sun. Earth has ocean highlights, night illumination, moving clouds and an atmospheric rim.
- Transparent, two-sided Saturn rings with texture alpha, a planet shadow on the rings and a ring shadow on the planet.
- An emissive textured Sun, solar corona, HDR rendering, up to 4× MSAA, separable bloom, ACES-style tone mapping and gamma correction.
- A distant Milky Way panorama and 4,500 deterministic point-sprite stars.
- Screen-space labels with overlap suppression, translucent panels, selection facts, status/FPS display and help.
- Resource-owning classes, cached shader uniforms, shader error logs, texture fallback, resize handling and clean shutdown.

![Earth and Moon](docs/screenshots/earth.png)

## Scientific scope

This is an educational graphics simulation, not a precision ephemeris or an N-body solver. Planet sizes and distances are deliberately compressed independently. The starting phases are illustrative. Orbits are circular with small inclinations; eccentricity, precession and gravitational interaction are not modeled. Orbital periods preserve their real relative ratios. Axial rotation is slowed by 60× relative to simulated days so textures remain readable; the UI shows the physical sidereal rotation period. The Moon uses its orbital period directly so the same hemisphere faces Earth.

Lighting direction is computed from a point source at the origin. Intensity does not follow inverse-square falloff: compressed scene distances would otherwise make the outer planets difficult to see. Ambient illumination is an artistic visibility aid. Surface colors are enhanced educational textures, not calibrated scientific measurements.

## Architecture and files

| File/group | Responsibility |
|---|---|
| `src/main.cpp` | Validated command-line arguments and top-level error reporting |
| `Application` | GLFW lifetime, event loop, input routing and subsystem coordination |
| `Planet`, `SolarSystem` | Planet data, time, orbit/spin math and Earth-Moon hierarchy; no OpenGL |
| `Camera` | Orbit/pan/zoom, smooth focus, tracking and perspective/view matrices |
| `Geometry` | CPU generation of sphere, annulus, orbit and star buffers |
| `GLObject`, `Mesh`, `Shader`, `Texture` | Move-only GPU ownership, uploads, shader compilation, cached uniforms and image loading |
| `Renderer` | Opaque/transparent passes, MSAA resolve, HDR bloom and final image |
| `UIManager` | ImGui styling, widgets, projected labels and contextual facts |
| `shaders/` | 13 GLSL vertex/fragment shader files |
| `assets/` | 14 local surface/environment maps, Roboto font and provenance |
| `tests/` | CPU assertions, OpenGL integration scenario and resource recovery checks |
| `third_party/` | Bundled dependency sources and version manifest |
| `docs/` | Defense explanation, demo script, acceptance/testing evidence and screenshots |

The full authored-file inventory is `docs/FILE_MANIFEST.txt`. The defense documents are in plain English: [project explanation](docs/PROJECT_EXPLANATION.md) and [3–5 minute demonstration](docs/DEMO_SCRIPT.md).

## Testing and screenshots

```powershell
ctest --test-dir build -C Release --output-on-failure
.\build\Release\SolarSystemExplorer.exe --smoke-test --hidden
.\build\Release\SolarSystemExplorer.exe --planet earth --paused --capture earth.png
.\build\Release\SolarSystemExplorer.exe --planet saturn --paused --capture saturn.png
.\build\Release\SolarSystemExplorer.exe --benchmark --frames 1200
.\build\Release\SolarSystemExplorer.exe --fallback-textures
```

`--capture` defaults to 180 frames so focus can settle. `--frames` uses deterministic 1/60-second simulation steps. `--hidden` creates a hidden GLFW window but still requires a real graphics driver; it is not a software/headless renderer. `--benchmark` disables VSync; its reported average includes CPU submission, rendering, presentation and final GPU completion, and is not a guarantee for other hardware. Interactive mode enables VSync.

Use `--help` for all options. `SSE_BUILD_TESTS=OFF` omits the smoke-test entry point and test executables for a smaller production build. The normal app and all user controls remain available.

## Troubleshooting

- **Context creation failed:** install the GPU vendor's driver. Windows' generic display driver, some remote sessions and VMs may not provide OpenGL 3.3.
- **Missing shader:** copy the whole distribution, or specify `--assets PATH`. Shader syntax and linking failures are printed with the offending file and compiler log.
- **Missing/corrupt texture:** a deterministic material is generated, a warning is logged and the scene continues. A missing font falls back to ImGui's built-in font.
- **Build executable is locked:** close the running application before rebuilding on Windows.
- **Slow rendering:** try a smaller window or disable Bloom. The scene intentionally reuses a single sphere mesh and uploads static geometry only once.

No mandatory feature is intentionally left as a stub. Verification limits, including hardware/toolchain coverage and the absence of a formal GPU leak proof, are stated in `docs/TESTING.md`.

## Credits

Planetary and environment maps: [Solar System Scope / INOVE](https://www.solarsystemscope.com/textures/), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/), based in part on NASA imagery. Maps are bundled without image edits; the renderer changes their illumination and appearance. Source URLs and SHA-256 hashes are in `assets/textures/SOURCES.json`.

Astronomical reference: [NASA Solar System Exploration](https://science.nasa.gov/solar-system/planets/) and the [NASA planetary fact sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/). Values are rounded; diameters here are mean diameters.

See `THIRD_PARTY_NOTICES.md` and `licenses/` for library and font licenses. Original project code is provided under the MIT license; bundled assets retain their separate licenses.
