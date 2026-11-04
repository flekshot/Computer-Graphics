# Demonstration script — approximately 4 minutes

Before the presentation, build Release and run the tests. Use the portable folder on the presentation computer and check its OpenGL driver. Start with:

```powershell
.\SolarSystemExplorer.exe --paused
```

Keep the README and `PROJECT_EXPLANATION.md` available for questions. Use the default 1440×900 window if the display supports it. The narration below is deliberately simple English.

## 0:00–0:35 — Introduce the system

Show the overview. Briefly drag in empty space, then press **R** to restore the presentation view.

> This is Solar System Explorer, my final project for Computer Graphics Fundamentals. It is written in C++17 with OpenGL 3.3 Core. It contains the Sun, all eight planets, Earth's Moon and Saturn's rings. The sizes and distances are scaled for clear exploration.

Point to the planet list and information panel.

> The scene uses generated geometry, texture mapping, custom shaders and an interactive orbit camera. All resources are stored locally, so the program works offline.

## 0:35–1:10 — Show real-time simulation

Press **Space**. Increase the speed using **+** several times or move the slider to about **10×**. Watch Mercury and Venus move. Return speed to about **1×**.

> Every orbit is calculated from its period and elapsed simulation time. Movement uses delta time, so it does not depend on frame rate. The planets continue moving even when the camera follows a different object. Their orbital periods keep realistic relative ratios.

Press **Space** once to pause.

> Pausing stops orbital and axial motion while the camera remains interactive.

## 1:10–2:00 — Earth, lighting and hierarchy

Press **3** and allow the smooth transition to finish. Drag slowly across the Earth to show the day/night boundary. Use the wheel a little; avoid extreme zooming during the presentation. Turn **Orbits** off briefly if the lines distract from the surface.

> Selecting Earth starts a smooth camera flight. The camera then follows its current position. The planet has a surface texture, cloud layer, atmospheric rim and night lights. A point light at the Sun creates diffuse and specular lighting, with a dark opposite hemisphere.

Point to the Moon. Turn orbits on, resume briefly, then pause again.

> The Moon is the hierarchical transformation example. It inherits Earth's translation, then applies its own orbit and scale. It does not inherit Earth's surface rotation. That keeps the lunar orbit attached correctly.

If Earth faces away from the Sun after time has advanced, press **F** to refocus from the illuminated side.

## 2:00–2:40 — Saturn and transparency

Press **6**, let the camera settle, and make a short orbit drag.

> Saturn's rings are a textured annulus with transparency. The opaque planet writes depth first; the rings blend afterward without writing depth. The ring is visible from both sides. The shaders also calculate the planet's shadow on the rings and the ring shadow on the planet.

Point to the ring gap and shadow. Briefly toggle **Labels** to show that the labels are a separate projected overlay.

## 2:40–3:20 — Rendering pipeline

Press **0**, then toggle **Bloom** off and on while looking at the Sun.

> The Sun uses an emissive shader. I render to a floating-point HDR framebuffer, resolve multisample anti-aliasing, blur bright pixels at half resolution, and combine them with the scene. Tone mapping brings the HDR result into the display's range. The interface is rendered last, so its text stays sharp.

> The background combines a local Milky Way texture with 4,500 generated stars. The application uses VAOs, VBOs, an index buffer and GLSL shaders throughout.

## 3:20–4:00 — Explain design and close

Show **H** for the control summary, then close help. Resize the window moderately using its border and show that planets remain round. Return to overview.

> The code separates simulation, camera, rendering and interface responsibilities. Resource-owning classes release GPU objects before the OpenGL context is destroyed. Missing textures receive a procedural fallback, and shader errors include compiler diagnostics.

> The automated tests check orbital closure, pause and speed, the Earth-Moon distance, mesh normals and winding, camera transitions, resource failures and real rendering. The project brings the major course topics together in one interactive application.

Finish with the scene visible for questions. Press Escape only when the demonstration is over.

## If the examiner asks for code

Open `Planet.cpp` for `T × R × R × S`, `SolarSystem.cpp` for the Moon hierarchy, `planet.vert` for the normal matrix, `planet.frag` for Blinn-Phong lighting, and `Camera.cpp` for frame-independent interpolation. The longer explanation and suggested answers are in `PROJECT_EXPLANATION.md`.
