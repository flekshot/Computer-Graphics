# Demonstration script — approximately 5 minutes

Before presenting, build Release and run the tests. Check the presentation computer's OpenGL driver and rehearse Save/Load with a file location you can find easily. Use the default 1440×900 window if possible; smaller windows have scrollable side panels. Start from the portable folder with:

```powershell
.\SolarSystemExplorer.exe --paused
```

Keep the README and `PROJECT_EXPLANATION.md` available for questions. Ready-made experiments are included in `examples/`: `elliptical-mars.sselab`, `solar-eclipse.sselab`, and `lunar-eclipse.sselab`. They can be opened through **Load...** if you want a rehearsed starting state.

## 0:00–0:35 — Introduce the system

Show the overview. Drag briefly in empty space, then press **R**.

> This is Solar System Explorer, our team's Computer Graphics Fundamentals project. It uses C++17 and OpenGL 3.3 Core. The default scene contains the Sun, eight planets, Earth's Moon, and Saturn's rings. All resources are local, so it runs offline.

Point to **Explore**, **Orbit Lab**, and **Eclipse Lab**.

> Explore presents the system. Orbit Lab lets us change motion and inspect the graphics. Eclipse Lab uses a separate scene to explain how bodies block sunlight. The scales are illustrative, not predictions of today's positions.

## 0:35–1:15 — Explore lighting and hierarchy

Press **3**, allow the camera transition to finish, and make a short drag. Point to Earth's surface, clouds, atmospheric rim, and Moon. Resume with **Space** briefly, then pause.

> A point light at the Sun produces diffuse and specular illumination. Earth adds clouds, night lights, and an atmospheric rim. The Moon inherits Earth's translation but not its surface spin, which keeps its own orbit attached correctly.

Press **6** and show Saturn's rings.

> Saturn's rings use transparent geometry drawn after the opaque planet. The shaders calculate shadows between the rings and the planet.

## 1:15–2:10 — Change an orbit

Click **Orbit Lab**, select **Mars**, and choose **Try elliptical orbit** in the right panel's **Orbit** tab. The example starts paused at day zero in overview. Point to the ellipse and its markers. If necessary, scroll the right panel to see the overlays.

> The semi-major axis controls size, eccentricity controls shape, and inclination tilts the orbital plane. Period and initial phase are editable too. Validation keeps the closest point outside the Sun.

Resume, increase the main speed to about **20×**, and observe the trajectory. Pause again.

> Kepler's equation makes motion faster near periapsis and slower near apoapsis. Green marks the closest point and orange the farthest. The arrow shows analytic velocity, with its length scaled for display. The recent trail is rebuilt from the current orbit.

Enter **0** in **Simulation time** and press Enter.

> Seeking a day also pauses the system. These are prescribed elliptical paths; changing size and period independently is useful for experiments, but it is not an N-body gravity simulation.

## 2:10–2:50 — Inspect the graphics

Press **F** to focus Mars. Open the **Graphics** tab. Switch to **Wireframe**, then **World normals**.

> Wireframe reveals the actual indexed triangle mesh. World normals encodes the transformed X, Y, and Z components as color. The short lines show the surface normal directions used in lighting.

Return to **Shaded**. Set **Diffuse** to zero, then restore it with **Reset lighting**. Briefly adjust **Specular** or **Ambient**.

> The lighting terms can be changed independently. This makes the difference between ambient visibility, diffuse illumination, and specular highlights observable. Bloom and exposure are separate rendering controls.

## 2:50–4:10 — Compare solar and lunar eclipses

Open **Eclipse Lab**. Select **Solar eclipse**, **Shaded**, and **Total alignment**. Point first to the upper geometry view, then to the larger lower Earth detail. Turn on **Shadow cones** if needed.

> Here the Moon blocks sunlight reaching Earth. The upper view explains the alignment; the lower view shows the target. Purple marks the umbra boundary, where sunlight is fully blocked, and amber marks the penumbra boundary, where it is partly blocked.

Choose **Partial alignment**, then **Clear of shadow**. Point to the visible-sunlight percentage. Return to **Total alignment** and change inclination a little.

> The percentage samples Earth's Sun-facing surface center. Other places can see a different amount of the Sun. Changing phase or inclination moves the shadow, which explains why alignment matters.

Switch to **Lunar eclipse** and choose **Total alignment**. Compare **Partial alignment**, then toggle **Observer viewpoint** for the lower view. Briefly press **Animate**, then **Pause**.

> In a lunar eclipse, Earth blocks sunlight reaching the Moon. This laboratory has its own angular animation speed. It estimates visible sunlight from apparent disk overlap on a consistent illustrative scale; it does not predict eclipse dates or atmospheric effects.

## 4:10–5:00 — Save, restore, and close

Click **Save...**, choose a location, and save a `.sselab` file. Change the alignment or click **Reset experiment**, then use **Load...** to reopen the saved file.

> An experiment stores all eight orbits, time, selection, graphics controls, the active laboratory, and eclipse settings. Loading validates the complete file before changing the scene. It restores the selected target, but not the exact camera pose.

Return to **Explore**. If a clean closing scene is wanted, click **Reset experiment**, which pauses the original system.

> The code separates simulation mathematics, rendering, the interface, and persistence. Automated checks cover the original simulation, elliptical motion, save/load failures, eclipse geometry, and real OpenGL rendering. These laboratories bring transformations, shading, animation, and geometric reasoning together in one application.

Leave the scene visible for questions. Press Escape only when the demonstration is over.

## If the examiner asks for code

Open `Planet.cpp` for Kepler's equation and analytic velocity, `SolarSystem.cpp` for validation and the Moon hierarchy, `LabUI.cpp` for the editors, and `Renderer.cpp` plus `debug_surface.frag` for graphics diagnostics. `EclipseModel.cpp` and `eclipse.frag` show sunlight occlusion; `ExperimentIO.cpp` shows staged validation and persistence. The detailed explanation and suggested answers are in `PROJECT_EXPLANATION.md`.
