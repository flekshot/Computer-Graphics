# Solar System Explorer

Final project report

Astana IT University  
Computer Graphics Fundamentals · Academic year 2026–2027

| Assignment | Team |
| --- | --- |
| Topic 2 · Solar System | Abai Erik · Nurbolat Zhumadulla |
| Group · {{GROUP}} | Nurasyl Adildabek · Ulnaz Seitzhan |

## Project overview

Solar System Explorer is a native, interactive OpenGL 3.3 Core application. It brings the Sun, eight planets, Earth's Moon, Saturn's rings and an asteroid belt into one scene. Explore mode presents the system. Orbit Lab edits trajectories and exposes render diagnostics. Eclipse Lab compares solar and lunar alignments. Workshop generates planets and tracks five challenges.

This report focuses on HDR Bloom, the rendering technique demonstrated at the start of the defence. The application can display intermediate buffers and compare the result with and without Bloom using the same view. Distances and sizes are compressed for exploration. The orbit and eclipse models use illustrative scales and simplified dynamics.

## Common requirement checklist

Section 6 was absent from the supplied assignment document. The checklist reflects Sections 2–5. The topic-specific checklist is pending access to Section 6.

| Requirement | Status and evidence |
| --- | --- |
| Native programmable graphics | Done. C++17, OpenGL 3.3 Core, file-based GLSL, and local assets. |
| Interactive camera and controls | Done. Orbit, pan and zoom, resize-aware projection, H help, Space pause and F6 Bloom. |
| Depth and culling | Done. Depth-tested opaque geometry and scoped exceptions for transparent or two-sided passes. |
| Adjustable light | Done. Workshop azimuth and elevation change the preview's directional light. |
| Visible errors | Done. Shader failures, incomplete targets and texture fallbacks report to the console. |
| Repeatable interaction | Done. Five Workshop challenges with conditions, feedback, completion count and reset. |
| Build and performance | {{BUILD_STATUS}} |
| Team attribution and backup link | {{SUBMISSION_STATUS}} |

::: page

## HDR Bloom pipeline

The main renderer uses the programmable vertex and fragment stages for geometry. Opaque and transparent passes produce a linear HDR scene with depth testing. A framebuffer blit transfers the HDR color into a single-sample texture for fullscreen passes. The blit resolves multisampling at High quality. A separable filter spreads above-white energy into adjacent pixels. The final shader adds that energy before exposure, tone mapping and display encoding. See de Vries (n.d.) for the HDR Bloom workflow.

::: pipeline

Figure 1. The main scene's HDR post-processing. ImGui is drawn after the scene display transform.

## Equations used in the shaders

For an RGB scene sample C, extraction works per channel. A subscript p denotes a sample centered on a pixel. The vector a is one input texel along the horizontal or vertical axis. The fractional offsets and weights follow Rákos's (2010) linear-sampling Gaussian filter.

`E(C) = max(C - 1, 0)`

`B(p) = 0.227027 I(p) + 0.316216 [I(p ± 1.384615 a)]`  
`       + 0.070270 [I(p ± 3.230769 a)]`

Each ± bracket denotes the sum of the two positive and negative offset samples. The horizontal and vertical passes reuse the kernel with different axes. High quality uses ten passes at half width and height, with up to four scene samples per pixel. Low quality uses four passes at quarter width and height, with one scene sample per pixel. The fullscreen vertex shader generates the geometry for both extraction and blur.

`x = exposure × (C + 0.75 B)`  
`T(x) = clamp(x(2.51x + 0.03) / (x(2.43x + 0.59) + 0.14), 0, 1)`  
`display = T(x)^(1/2.2) × [1 - 0.22 ||uv - 0.5||²]`

With Bloom off, the composite omits the 0.75 B term. The tone curve uses the coefficients of Narkowicz's (2016) ACES fit. The comparison and stage views display the same camera and tone curve. In comparison, the glow term applies only where the screen coordinate u is at least 0.5. The inspector tone-maps the intermediate buffers for display. World normals map the unit normal N to 0.5N + 0.5 and bypass the artistic display transform.

::: page

## Scene rendering and interaction

The planet vertex shader evaluates clip position as P × V × M × p and transforms normals by the inverse transpose of the model's upper-left 3 × 3 matrix. The planet fragment shader uses textured ambient, diffuse and specular lighting. It computes diffuse as max(N·L, 0) and Blinn–Phong specular from max(N·H, 0) raised to a shininess exponent, with H = normalize(L + V) (Blinn, 1977). The main scene's light direction points toward the Sun. Workshop provides an independent adjustable direction to demonstrate the same geometric relationship on a stationary preview.

Orbit Lab solves E − e sin(E) = M for eccentric anomaly E by Newton iteration. The focal-plane position is x = a(cos E − e), z = −a√(1 − e²) sin E. The code tilts this position into the orbital plane and uses the analytic velocity for the arrow. Earth's Moon inherits Earth's translation and retains its own orbital rotation.

Saturn's translucent rings use an annulus and analytic ray intersections to produce planet and ring shadows. The asteroid belt reuses one mesh and supplies per-instance data to one indexed draw for 640 rocks.

## Controls and shot setup

| Input | Action |
| --- | --- |
| F6 · F7 · Space · H | Toggle Bloom, split the final image for comparison, pause or resume, and open help. |
| Left drag · wheel · WASD | Orbit the camera, zoom, and pan. |
| 1–8 · F · R | Select a planet, refocus the selection, and return to overview. |
| Graphics controls | Compare Bloom, inspect its stages, or select High or Low quality. |
| Workshop | Adjust light direction, switch to World normals, or complete a challenge. |
| Save / Load | Reopen a validated experiment file. |

For the Bloom comparison, start paused in Explore in Shaded mode. Press R and zoom toward the central Sun until its disk occupies roughly one quarter of the viewport height. Keep the camera still and turn on comparison. Inspect the region just outside the solar disk. Return to the final image with Bloom enabled for presentation. For a second shot, press 6 for Saturn and drag slightly above the ring plane. Use F10 to hide the side and bottom panels after framing. Open **Project & team** from the **View** menu.

::: page

## Build verification and performance

The Windows Release build used CMake with GCC 16.2.0 (w64devkit 2.11). The {{TEST_COUNT}} CTest checks passed, including OpenGL checks for the Bloom split, pipeline stages, normal colors under changing light direction and all 1,800 frames of the guided tour. The README gives Windows build and launch commands.

{{BENCH_METHOD}}

| Quality | Bloom | Mean FPS | Mean frame (ms) |
| --- | --- | --- | --- |
| High | On | {{HIGH_ON_FPS}} | {{HIGH_ON_MS}} |
| High | Off | {{HIGH_OFF_FPS}} | {{HIGH_OFF_MS}} |
| Low | On | {{LOW_ON_FPS}} | {{LOW_ON_MS}} |
| Low | Off | {{LOW_OFF_FPS}} | {{LOW_OFF_MS}} |

{{BENCH_INTERPRETATION}}

## Challenge walkthrough

Start **Read the normals** from the Workshop challenge list. In Shaded mode, pause the world and move the light to demonstrate the changing terminator. Checking the challenge while shaded gives retry feedback. Select **World normals** and check again. The completion mark and count show the result. Changing the light leaves the normal colors unchanged. Challenge progress can be reset or stored with the experiment. The Workshop light is a unit vector L = (cos(e) sin(a), sin(e), cos(e) cos(a)), where a and e are azimuth and elevation in radians.

## Individual contributions

| Team member | Named module and contribution |
| --- | --- |
| Abai Erik | {{ABAI}} |
| Nurbolat Zhumadulla | {{NURBOLAT}} |
| Nurasyl Adildabek | {{NURASYL}} |
| Ulnaz Seitzhan | {{ULNAZ}} |

Backup video. {{VIDEO_LINK}}

::: evidence

## References

Blinn, J. F. (1977). Models of light reflection for computer synthesized pictures. In *Proceedings of the 4th annual conference on computer graphics and interactive techniques* (pp. 192–198). ACM.  
https://doi.org/10.1145/563858.563893

de Vries, J. (n.d.). *Bloom*. LearnOpenGL.  
https://learnopengl.com/Advanced-Lighting/Bloom

Flyingidiot. (2007, February 19). *Radar2* [Animated GIF]. Wikimedia Commons.  
https://commons.wikimedia.org/wiki/File:Radar2.gif

INOVE. (n.d.). *Solar textures* [Texture maps]. Solar System Scope.  
https://www.solarsystemscope.com/textures/

Marvel. (n.d.). *Rotating earth (large)* [Animated GIF]. Wikimedia Commons.  
https://commons.wikimedia.org/wiki/File:Rotating_earth_(large).gif

Narkowicz, K. (2016, January 6). *ACES filmic tone mapping curve*.  
https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/

National Aeronautics and Space Administration. (n.d.). *About the planets*. NASA Science.  
https://science.nasa.gov/solar-system/planets/

OpenAI. (2026). *Codex* [AI coding assistant].  
https://openai.com/codex/

Rákos, D. (2010, September). *Efficient Gaussian blur with linear sampling*. RasterGrid.  
https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

Segal, M., & Akeley, K. (2010, March 11). *The OpenGL graphics system: A specification* (Version 3.3, core profile). Khronos Group.  
https://registry.khronos.org/OpenGL/specs/gl/glspec33.core.pdf

## Asset and library notices

Planetary maps, ring opacity and the Milky Way panorama are from INOVE's Solar System Scope under Creative Commons Attribution 4.0 International. The files are unchanged and the renderer adjusts their appearance. Wikimedia's radar animation is in the public domain. Marvel's rotating Earth and its monochrome display adaptation are licensed under CC BY-SA 3.0. The manifests in `assets/textures/SOURCES.json` and `assets/hud/SOURCES.json` list individual URLs and checksums.

The bundled helper libraries are GLFW (zlib/libpng), GLEW (BSD-style and bundled notices), GLM (MIT option), Dear ImGui (MIT) and stb_image/stb_image_write (MIT option). The Roboto Medium font is from Google under Apache 2.0. The project keeps versions, source URLs and complete license notices in `THIRD_PARTY_NOTICES.md` and `licenses/`.

## AI assistance disclosure

OpenAI Codex (OpenAI, 2026) provided substantial assistance with implementation and documentation, including C++ integration, GLSL post-processing and instanced rendering, Workshop lighting and persistence, tests, and report drafting. The relevant implementation is in `Renderer.cpp`, the Bloom and asteroid shaders, the Workshop renderer and model, and the experiment I/O and associated tests. The named contribution table is reserved for verified individual work. The defence covers reading, explaining and modifying the shipped shader code.
