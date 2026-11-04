# Solar System Explorer

**An interactive desktop planetarium built with C++17 and OpenGL 3.3.**

Explore the Sun and all eight planets, follow Earth and its Moon, and take a closer look at Saturn's rings. Adjust the simulation speed, move the camera freely, and learn about each world through planet information panels and an in-app flight manual.

**Computer Graphics Fundamentals** · Windows x64 · Offline desktop application

[Team](#project-team) · [Features](#features) · [Gallery](#gallery) · [Build and run](#get-started) · [Controls](#controls) · [Documentation](#project-guide)

![Solar System Explorer overview](SolarSystemExplorer/docs/screenshots/overview.png)

## Project team

Solar System Explorer was developed as a Computer Graphics Fundamentals project through the active contributions of four team members:

- **Abai Erik**
- **Ulnaz Seitzhan**
- **Nurbolat Zhumadulla**
- **Nurasyl Adildabek**

Together, the team invested time, effort, and collaboration in bringing this interactive Solar System to life.

## About the project

The project brings the main topics of a real-time computer graphics course into one scene: procedural geometry, coordinate transformations, texture mapping, lighting, hierarchical animation, transparency, and post-processing. Planetary motion and camera interaction make these concepts visible and easy to explore.

The application runs locally without an internet connection. Dependency source code, 14 texture maps, shaders, fonts, and license notices are included, so the project can be built offline once the required development tools are installed.

## Features

### Worlds and motion

- **The complete planetary system:** the Sun, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, and Neptune, with individual textures and orbital and rotation parameters.
- **Earth and the Moon:** a Moon that travels with Earth while keeping its own orbit independent of Earth's surface rotation.
- **Continuous animation:** inclined circular orbits and axial rotation evaluated from a shared simulation clock.
- **Time controls:** pause and resume, or change speed from 0.05× to 100×. At 1×, one real second advances six simulated days.

### Lighting and visual effects

- **Sun-based lighting:** ambient, diffuse, and specular shading with the Sun as the point light source.
- **Earth's surface and atmosphere:** day and night maps, ocean highlights, moving clouds, and atmospheric glow.
- **Saturn's rings:** transparent, two-sided geometry with shadows between the rings and the planet.
- **A glowing Sun:** emissive rendering, a corona, HDR bloom, tone mapping, and gamma correction.
- **A detailed background:** a Milky Way panorama and 4,500 stars, with orbit lines and planet labels.
- **Smoother edges:** up to 4× MSAA, subject to graphics hardware support.

### Camera and interface

- Orbit, pan, and zoom with smooth camera transitions and limits that keep the camera outside celestial bodies.
- Select any planet, approach it, and follow its movement around the Sun.
- Read planet facts, adjust time, and toggle orbits, labels, and bloom through a Dear ImGui interface.
- Return to the system overview or open the flight manual at any point.

## Gallery

These screenshots were captured from the application.

| Earth and Moon | Saturn |
| --- | --- |
| ![Earth and Moon in Solar System Explorer](SolarSystemExplorer/docs/screenshots/earth.png) | ![Saturn and its rings in Solar System Explorer](SolarSystemExplorer/docs/screenshots/saturn.png) |
| Surface detail, cloud layers, atmospheric glow, and the lunar companion. | Textured rings, transparency, and sunlight interacting with the planet. |

## Technology stack

| Technology | Purpose |
| --- | --- |
| C++17 | Application logic, simulation, filesystem access, and resource ownership |
| OpenGL 3.3 Core / GLSL 330 | GPU rendering and custom vertex and fragment shaders |
| GLFW 3.4 | Window creation, OpenGL context, keyboard, and mouse input |
| GLEW 2.2.0 | OpenGL function loading |
| GLM 1.0.1 | Vectors, matrices, camera transforms, and projection |
| Dear ImGui 1.91.9b | Navigation panels, controls, labels, and help |
| stb_image / stb_image_write | Texture loading and framebuffer captures |
| CMake | Building the application, bundled dependencies, and tests |

The bundled versions and their sources are recorded in the [dependency manifest](SolarSystemExplorer/third_party/MANIFEST.json).

## Get started

### Run the Windows build

For a prebuilt version, check [GitHub Releases](https://github.com/flekshot/Computer-Graphics/releases). Extract the entire portable archive and launch `SolarSystemExplorer.exe`; keep the `assets`, `shaders`, and `licenses` folders beside it.

### Build from source

Requirements:

- Windows 10/11, x64.
- Visual Studio 2022 with **Desktop development with C++** and a Windows SDK.
- CMake 3.24 or newer for the Visual Studio 2022 preset.
- A GPU driver providing OpenGL 3.3 Core.

From the repository root, run these commands in PowerShell:

```powershell
cd SolarSystemExplorer
cmake --preset vs2022
cmake --build --preset release --parallel
ctest --preset release
.\build\vs2022\Release\SolarSystemExplorer.exe
```

The project bundles its dependency sources, so no package manager or network access is needed during the build. Visual Studio 2026 presets are also available; see the [build guide](SolarSystemExplorer/README.md#build-from-source) for the matching toolchain, IDE setup, and portable packaging instructions.

Alternatively, run `BuildAndRun.bat` inside `SolarSystemExplorer/` to configure, build, test, and launch with the locally available Visual Studio toolchain. The graphics tests require a working OpenGL driver.

## Controls

| Input | Action |
| --- | --- |
| Drag with the left mouse button | Orbit the camera |
| Mouse wheel | Zoom in or out |
| W / A / S / D | Pan the camera |
| 1–8 | Select a planet |
| F | Refocus the selected planet |
| Space | Pause or resume time |
| + / − | Change simulation speed |
| 0 or R | Return to the system overview |
| Orbits / Labels / Bloom | Toggle the corresponding visual feature |
| H | Show or hide the flight manual |
| Esc | Exit; while editing a numeric field, cancel that edit first |

Camera gestures apply over the scene. UI interaction is handled separately, and Tab and arrow keys can navigate interface controls.

## Project structure

The application source lives in `SolarSystemExplorer/`:

```text
SolarSystemExplorer/
├── src/                 Application, simulation, camera, and rendering
├── include/             Class interfaces and resource wrappers
├── shaders/             GLSL vertex and fragment shaders
├── assets/              Texture maps, fonts, and asset provenance
├── tests/               CPU, rendering, and resource recovery checks
├── third_party/         Bundled dependency sources and version manifest
├── docs/                Technical explanation, demo, testing notes, screenshots
├── licenses/            Third-party license texts
├── CMakeLists.txt       Build configuration
├── CMakePresets.json    Visual Studio build and test presets
└── BuildAndRun.bat      Windows build and launch helper
```

### Main components

| Component | Responsibility |
| --- | --- |
| `Application` | Creates the window, routes input, and coordinates the frame loop |
| `Planet` / `SolarSystem` | Store planet data and calculate time, orbits, rotation, and the Earth-Moon hierarchy |
| `Camera` | Handles orbiting, panning, zooming, smooth focus, and planet tracking |
| `Geometry` | Generates spheres, rings, orbit lines, and star positions |
| `Mesh` / `Shader` / `Texture` / `GLObject` | Manage geometry buffers, shader programs, textures, and GPU resource lifetime |
| `Renderer` | Draws opaque and transparent objects, resolves MSAA, and applies bloom and tone mapping |
| `UIManager` | Builds the controls, information panels, help, and projected labels |

Each frame processes input, updates the simulation and camera, renders the scene, applies post-processing, and draws the interface. A shared sphere mesh is reused for celestial bodies, and static geometry and textures are uploaded during initialization.

## Command-line examples

After building with the `vs2022` preset, run these commands from `SolarSystemExplorer/`:

```powershell
# Open a paused view of Earth
.\build\vs2022\Release\SolarSystemExplorer.exe --planet earth --paused

# Capture Saturn after the camera settles
.\build\vs2022\Release\SolarSystemExplorer.exe --planet saturn --paused --capture .\build\vs2022\saturn.png

# Run a fixed-length benchmark with VSync disabled
.\build\vs2022\Release\SolarSystemExplorer.exe --benchmark --frames 1200

# Show all available options
.\build\vs2022\Release\SolarSystemExplorer.exe --help
```

Captures default to 180 frames to allow the camera to settle. Benchmark results depend on the GPU, driver, and window size; the [full usage guide](SolarSystemExplorer/README.md#testing-and-screenshots) explains the timing and capture options.

## Educational scope

The simulation uses illustrative scales so the whole system remains practical to explore. Planet sizes and orbital distances are compressed separately, orbits are circular with small inclinations, and initial positions are illustrative. Relative orbital periods are preserved, while axial spin is slowed for readability.

Lighting is also simplified: the Sun provides a point light, and ambient illumination keeps dark surfaces visible. Gravitational interactions, orbital eccentricity, and precision ephemeris prediction are outside the current model. These choices support the project's focus on graphics, transformations, animation, and interaction.

## Testing and troubleshooting

The repository includes checks for orbital motion, pause and speed controls, the Earth-Moon hierarchy, geometry generation, camera behavior, OpenGL rendering, and resource recovery. From `SolarSystemExplorer/`, after a Release build:

```powershell
ctest --preset release
```

Graphics checks need a real OpenGL context even when the window is hidden. The [testing report](SolarSystemExplorer/docs/TESTING.md) records the environment, observed results, and limits of the original verification.

- **OpenGL context cannot be created:** install a GPU driver with OpenGL 3.3 support. Generic display drivers and some remote sessions may be insufficient.
- **Resources are missing:** keep the executable with its complete `assets/` and `shaders/` folders, or use `--assets PATH` to select a resource root.
- **The executable is locked during a rebuild:** close the running application before building again.
- **Rendering is slow:** reduce the window size or disable bloom.

## Project guide

| Document | What it covers |
| --- | --- |
| [Application guide](SolarSystemExplorer/README.md) | Detailed setup, controls, command-line options, and troubleshooting |
| [Project explanation](SolarSystemExplorer/docs/PROJECT_EXPLANATION.md) | Geometry, matrices, orbit mathematics, lighting, camera behavior, and rendering |
| [Demo script](SolarSystemExplorer/docs/DEMO_SCRIPT.md) | A guided 3–5 minute presentation of the application |
| [Testing notes](SolarSystemExplorer/docs/TESTING.md) | Recorded build and runtime checks, environment, and verification limits |
| [Third-party acknowledgements](SolarSystemExplorer/THIRD_PARTY_NOTICES.md) | Library, texture, and font attribution |

## Assets and acknowledgements

Planetary and environment textures are credited to Solar System Scope / INOVE under CC BY 4.0. The [texture source manifest](SolarSystemExplorer/assets/textures/SOURCES.json) records their original URLs and SHA-256 hashes. Roboto supplies the interface font; its license and the bundled library notices are included with the project.

## License

Original project code is available under the [MIT License](SolarSystemExplorer/LICENSE). Bundled libraries, fonts, and planetary textures retain their own licenses; see the [license texts](SolarSystemExplorer/licenses/) and [third-party notices](SolarSystemExplorer/THIRD_PARTY_NOTICES.md).
