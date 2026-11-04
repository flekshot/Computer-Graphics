# Solar System Explorer

**An interactive desktop planetarium built with C++17 and OpenGL 3.3.**

Explore the Sun and all eight planets, follow Earth and its Moon, and take a closer look at Saturn's rings. Adjust the simulation speed, move the camera freely, and learn about each world through an in-app flight manual.

![Solar System Explorer overview](SolarSystemExplorer/docs/screenshots/overview.png)

## Highlights

- Explore eight textured planets, the Sun, Earth's Moon, and Saturn's rings.
- Select and follow a planet, pause time, or change the simulation speed.
- See a star field, planetary orbits, Earth clouds, atmospheric glow, and bloom.
- Build and run offline on Windows with the bundled, licensed dependencies.

## Get started

### Run the Windows build

For a prebuilt version, check [GitHub Releases](https://github.com/flekshot/Computer-Graphics/releases). Extract the entire portable archive and launch `SolarSystemExplorer.exe`; keep the `assets`, `shaders`, and `licenses` folders beside it.

### Build from source

Requirements: Windows 10/11 x64, Visual Studio 2022 with **Desktop development with C++**, CMake 3.24 or newer, and a GPU driver with OpenGL 3.3 Core support.

From the repository root, run these commands in PowerShell:

```powershell
cd SolarSystemExplorer
cmake --preset vs2022
cmake --build --preset release --parallel
ctest --preset release
.\build\vs2022\Release\SolarSystemExplorer.exe
```

The project bundles its dependency sources, so no package manager or network access is needed. Visual Studio 2026 presets are also available; see the [build guide](SolarSystemExplorer/README.md#build-from-source).

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
| H | Show or hide the flight manual |
| Esc | Exit the application |

## Project guide

- [Build, full controls, features, and troubleshooting](SolarSystemExplorer/README.md)
- [Project explanation](SolarSystemExplorer/docs/PROJECT_EXPLANATION.md)
- [Demo script](SolarSystemExplorer/docs/DEMO_SCRIPT.md)
- [Testing notes](SolarSystemExplorer/docs/TESTING.md)
- [Third-party acknowledgements](SolarSystemExplorer/THIRD_PARTY_NOTICES.md)

## License

Original project code is available under the [MIT License](SolarSystemExplorer/LICENSE). Bundled libraries, fonts, and planetary textures retain their own licenses; see the `licenses/` folders and [third-party notices](SolarSystemExplorer/THIRD_PARTY_NOTICES.md).
