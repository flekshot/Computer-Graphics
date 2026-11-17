# Solar System Lab

The user approved the previously proposed orbit editor, graphics learning modes, experiment persistence, and the final proposed eclipse laboratory. Implement them in the existing native C++17 / OpenGL 3.3 application, retaining the original exploration mode and four-person project credits.

## User experience

- A top navigation row selects Explore, Orbit Lab, or Eclipse Lab.
- Orbit Lab edits the selected planet's semi-major axis, eccentricity, inclination, period, and phase. An ellipse uses Kepler's equation so speed changes along the orbit. Periapsis must stay outside the Sun. Scene distances remain illustrative and periods are independently editable.
- The orbit view supports a recent trajectory, velocity arrow, and periapsis/apoapsis markers. Users can pause, seek simulation time, and reset the original system.
- Graphics controls select shaded, wireframe, or world-normal visualization and independently scale ambient, diffuse, and specular illumination. Normals mode also illustrates normal directions on the selected sphere.
- Save/Load uses native Windows file dialogs and a versioned .sselab file. Persist all eight orbital configurations, simulation day/speed/pause, selected planet, graphics settings, active laboratory, and eclipse settings. Invalid files must not partially change the live scene. Report useful errors.
- Eclipse Lab is a separate coherent illustrative Sun-Earth-Moon scene. Solar and lunar modes, phase/inclination controls, animation, total/partial/clear presets, shadow/penumbra cones, and two simultaneous views demonstrate the phenomenon. The detail view can switch to an observer viewpoint. No real-calendar prediction is claimed.
- Keep controls usable at 1000x640 and 1440x900; compact layouts may scroll.

## Architecture and shared contracts

Keep CPU mathematics and persistence independent of OpenGL. LabState.h owns POD settings shared by model, renderer, UI, and file I/O. Existing Renderer consumes expanded RenderSettings. EclipseModel and EclipseRenderer are separate modules. UIManager delegates lab panels to LabUI.cpp; Application owns input routing, dialogs, scene selection, and integration.

OrbitParameters uses double fields: semiMajorAxis, eccentricity, inclinationDegrees, periodDays, phaseDegrees.
Planet exposes orbitParameters(), velocity(double days), periapsis(), apoapsis(); existing position/orbitMatrix/modelMatrix remain compatible.
SolarSystem exposes bool setOrbit(size_t, const OrbitParameters&, std::string& error), resetOrbits(), and bool isModified() const.
saveExperiment(path, const SolarSystem&, const RenderSettings&, const LabState&, int selected, std::string& error) returns bool.
loadExperiment(path, SolarSystem&, RenderSettings&, LabState&, int& selected, std::string& error) returns bool and applies only fully validated data.
EclipseRenderer(root) and render(const EclipseSettings&, const RenderSettings&, const EclipseViewport&, int framebufferWidth, int framebufferHeight) render into the default framebuffer and restore GL state. EclipseViewport has int x, y, width, height in framebuffer pixels with bottom-left origin.

## Constraints

Windows x64, C++17, OpenGL 3.3 Core, existing vendored dependencies only. No edits to third_party or third-party licenses. Existing default eight-planet behavior and original CTest suite remain supported. No committing, pushing, or publishing is required for this implementation. Work occurs on feat/solar-system-lab in the user's current workspace.

## Verification

CPU tests cover elliptical motion, velocity, bounds, orbit reset, persistence roundtrip and malformed input, and eclipse occlusion geometry. GPU integration exercises all new rendering modes and lab switching, plus screenshots at default and minimum sizes. Existing seven CTest tests remain regression coverage. Actual test results and limits will be recorded.
