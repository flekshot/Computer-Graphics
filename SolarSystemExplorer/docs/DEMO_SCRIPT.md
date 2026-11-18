# Five minute defence and live demo

Launch the Release executable from its portable folder. Use a 1440 × 900 window if the display permits. Start paused, with High quality, Shaded mode, and Bloom enabled. Rehearse the Sun framing and the Workshop challenge from a fresh session. Keep the backup video and its share link available.

## 0:00 to 0:15 | Show Bloom immediately

Start Explore, press **R** for overview, then scroll forward over empty space until the central Sun occupies about one quarter of the viewport height. Hold that view at the start and point to the glow outside the disk. Press **F6** to turn Bloom off, then press it again to restore the halo. Point to the on-screen Bloom state.

> Solar System Explorer is our project for Topic 2, Solar System. The Sun writes color above display white into an HDR render target. Bloom spreads that light beyond the disk.

## 0:15 to 1:10 | Show the passes

Press **F7** to split the same camera view. The left half omits Bloom and the right half includes it. Point out the rim and nearby space. Press **F7** again, then use the pipeline selector in the **View** menu or Orbit Lab's **Graphics** tab. Choose **Scene without Bloom**, **Extracted highlights**, **Blurred highlights** and **Final composite** in succession.

> The geometry is shaded into a floating-point target and the multisampled image is resolved. The post-process keeps values above 1 in each color channel, blurs the result along one axis per pass, and adds it back before tone mapping. The comparison holds camera, exposure, and geometry fixed.

Return to the final image. Show the High and Low quality choices. Open **Project & team** from the **View** menu to identify the team and the on-screen help.

## 1:10 to 2:00 | Explore the scene

Press **3** for Earth. Point to the surface, clouds, atmosphere and Moon. Press **Space** to resume and pause. Press **6** to show Saturn's rings. Press **R** for overview, and point to the asteroid belt between Mars and Jupiter.

> The Moon follows Earth's translation and has its own orbital frame. Rings use transparent geometry and analytic shadows. The asteroid belt reuses geometry and draws many objects with one instanced draw call.

## 2:00 to 3:25 | Complete a Workshop challenge

Open **Workshop**, select **Read the normals** in **Challenges**, and click **Start challenge**. While the world is shaded, open **Surface** and expand **Rotation** to stop **Animate world**, then expand **Lighting** and change **Azimuth** or **Elevation**. The terminator should move across the stationary world. Click **Check challenge** while still shaded to show feedback. Select **World normals**, then click **Check challenge** again. Show the completed mark and changed count.

> The Workshop challenge has a visible goal, progress, retry feedback, and a reset. The direction controls move the light relative to the planet. World normals encode direction in RGB, so moving the light cannot change these colors.

## 3:25 to 4:25 | Edit an orbit and compare eclipses

Open **Orbit Lab**, choose Mars, and click **Try elliptical orbit**. Point out the periapsis, apoapsis, and velocity markers. Resume briefly and pause again. Open **Eclipse Lab**, choose **Solar eclipse**, and compare **Total alignment** and **Partial alignment**.

> The orbit uses Kepler's equation, so the planet moves faster near periapsis. Eclipse Lab estimates visible sunlight by comparing apparent disks. The displayed percentage samples the target's Sun-facing surface center.

## 4:25 to 5:00 | Close and leave the graphic on screen

Show where **Save...** and **Load...** preserve an experiment. Return to the Sun in Explore and restore the final composite with Bloom enabled. Show the measured 1280 × 720 Bloom on/off results in the report.

> The application runs locally with C++17, OpenGL 3.3 Core, bundled libraries and GLSL files. Its geometry scales are illustrative. The report documents the shaders, data sources and measured performance.

## Two minute question preparation

Every teammate should be able to trace the code from the floating-point target in `Renderer.cpp`, through `fullscreen.vert` and `blur.frag`, to `composite.frag`. Be ready to explain why an LDR target would lose above-white values, why separable blur uses two axes, why the glow is added before tone mapping, and why World normals bypass the artistic display transform. Rehearse adjusting the extraction threshold and predicting the before/after image.

For the other features, use `Planet.cpp` for the orbit solve, `WorkshopModel.cpp` and `workshop.frag` for the Workshop light, and `Missions.cpp` for the challenge conditions. See [Project explanation](PROJECT_EXPLANATION.md) for the equations and testable claims.

## Sixty second backup video

The guided tour runs the same renderer and challenge checks at fixed 30 FPS steps. It displays captions and does not require narration. Run the tour from a portable Release folder:

```powershell
.\SolarSystemExplorer.exe --demo-tour --width 1280 --height 720
```

Add `--record-frames <empty-folder>` to record the actual rendered frames for encoding.

| Time | What appears in the recording |
| --- | --- |
| 00–04 s | Sun with Bloom enabled. |
| 04–09 s | Bloom off, then on. |
| 09–16 s | Split comparison with the left half off and right half on. |
| 16–24 s | Scene without Bloom, extraction, blurred highlights, then final composite. |
| 24–34 s | Earth, Saturn and the asteroid belt from overview. |
| 34–54 s | Workshop challenge, changing the light on a paused world, World normals, and completion. |
| 54–60 s | Explore with the team and topic card visible. |

Upload the backup to YouTube or Google Drive and verify that an instructor can open the link without requesting access. Add the verified link to the report.
