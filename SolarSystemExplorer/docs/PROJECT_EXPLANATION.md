# Project explanation: Solar System Explorer

## 1. Purpose

This project demonstrates how a modern real-time graphics application is built from geometry, matrices, shaders and interactive input. The user can explore a complete educational Solar System, select any of eight planets, follow its movement, control time and inspect the Earth-Moon hierarchy and Saturn's rings.

Version 1.1.0 adds an orbit editor, graphics diagnostics, experiment save/load, and a separate eclipse laboratory. The top navigation selects Explore, Orbit Lab, or Eclipse Lab. Explore and Orbit Lab operate on the same system; Eclipse Lab has its own Sun–Earth–Moon geometry and animation clock.

The objective is to connect the concepts from Computer Graphics Fundamentals in one working application. It is not a scientifically accurate orbit prediction tool. Distances and sizes are compressed so all planets can be explored comfortably. Default orbital periods preserve real relative ratios; axial rotation is slowed for readability. Edited periods are independent of ellipse size, and eclipse geometry uses a separate illustrative scale.

## 2. Technology choices

| Technology | Role in the application |
|---|---|
| C++17 | Classes, resource ownership, standard containers and filesystem paths |
| OpenGL 3.3 Core | Programmable rendering pipeline and GPU resources |
| GLFW | Native window, OpenGL context, keyboard, mouse and events |
| GLEW | Loads OpenGL function entry points after the context is created |
| GLM | Vectors, matrix products, transforms, perspective and lookAt |
| GLSL 330 | Custom vertex/fragment shaders executed by the GPU |
| stb_image | Local JPEG/PNG texture decoding |
| stb_image_write | Diagnostic framebuffer screenshots |
| Dear ImGui | Interactive controls, facts, help and labels |
| CMake | Reproducible offline compilation of application and dependencies |

There are no `glBegin`, `glEnd`, matrix-stack calls or fixed-function lights. The application supplies explicit vertex attributes, matrices, textures and shader uniforms.

## 3. Architecture and frame lifecycle

`Application` owns the GLFW lifetime. In its `run()` method it constructs a `SolarSystem`, `Camera`, `Renderer` and `UIManager`, and creates `EclipseRenderer` when first needed. They are destroyed before the application's destructor destroys the OpenGL context.

`Planet` and `SolarSystem` contain data and mathematical behavior, with no OpenGL dependency. `Geometry` produces CPU arrays. `Mesh`, `Shader`, `Texture` and `GLObject` own GPU resources. `Renderer` draws the scene. `UIManager` presents widgets without becoming responsible for orbital mechanics.

`LabState.h` holds shared view, graphics, and eclipse settings. `LabUI.cpp` implements the laboratory panels. `EclipseModel` computes apparent-disk occlusion on the CPU, while `EclipseRenderer` draws it. `ExperimentIO` validates and serializes model/settings state independently of OpenGL; `ExperimentDialogs` supplies the Windows file picker. `Application` routes the resulting save, load, reset, and navigation actions.

Every Explore or Orbit Lab frame follows these steps:

1. Poll GLFW events and measure elapsed real time.
2. Skip rendering if the framebuffer is zero-sized; wait briefly for new events.
3. Resize render targets if needed and begin an ImGui frame.
4. Build UI controls and process input, respecting active UI widgets.
5. Advance simulated time, update camera interpolation and avoid solid bodies.
6. Draw the sky, stars, Sun, planets, Moon, orbits, rings, clouds and atmosphere.
7. Resolve MSAA, apply bloom and tone mapping, then draw the interface.
8. Check OpenGL errors and swap the front/back buffers.

All planets use the same day counter regardless of which one is selected or visible. Camera selection never stops another planet's simulation.

In Eclipse Lab, the original system clock is held and the independent alignment phase advances only when eclipse animation is enabled. `EclipseRenderer` draws two stacked viewports into the default framebuffer, restores the graphics state, and the interface is drawn afterward. Returning to Explore or Orbit Lab resumes using the retained system state.

## 4. Geometry and buffers

`Geometry.cpp` generates one unit UV sphere, reused by the Sun, all planets, the Moon, clouds and atmospheric shells. With latitude angle θ and longitude angle φ:

```text
x = sin(θ) cos(φ)
y = cos(θ)
z = sin(θ) sin(φ)
θ = vπ,  φ = u2π
```

The normal of the unit sphere is the normalized position. Texture coordinates are `(u, v)`. Longitude vertices are duplicated at the seam so both `u=0` and `u=1` can exist at the same position. Degenerate triangles at the poles are omitted.

A `Vertex` contains position, normal and UV. A VBO stores these vertices; an EBO stores triangle indices. A VAO remembers attribute layout and buffer associations. `Mesh::draw()` binds its VAO and calls `glDrawElements`. Counterclockwise winding and back-face culling reduce unnecessary rasterization.

The ring is an indexed annulus with inner and outer vertices at each angular sample. Orbits use a reusable unit circle drawn with `GL_LINE_LOOP`, transformed by ellipse-axis scaling, the focus offset, and orbital-plane inclination. Stars are generated with a fixed random seed on a distant sphere and drawn as point sprites in one call.

## 5. Model, view and projection matrices

The fundamental vertex transformation is:

```glsl
gl_Position = projection * view * model * vec4(localPosition, 1.0);
```

GLM uses column vectors. Therefore, the transform on the right acts first. A planet's surface uses:

```text
M = T(position) × R(axial tilt) × R(surface spin) × S(radius)
```

In the actual implementation, `equatorialFrame(days)` returns the translation and tilt. `Planet::modelMatrix()` then applies spin and scale:

```cpp
return equatorialFrame(days)
     * glm::rotate(glm::mat4(1), spin, glm::vec3(0, 1, 0))
     * glm::scale(glm::mat4(1), glm::vec3(data_.radius));
```

The view matrix comes from `glm::lookAt(cameraPosition, cameraCenter, worldUp)`. The perspective matrix uses a 45-degree vertical field of view, framebuffer aspect ratio, a 0.025 near plane and an 1800 far plane. A small off-axis projection shift centers the subject in the space between unequal side panels. The overview distance also adjusts to available scene width.

Changing the window size updates the framebuffer viewport and aspect ratio. Logical ImGui coordinates and physical framebuffer pixels are kept separate, so scaling does not stretch the 3D scene.

## 6. Orbit mathematics and time

`PlanetData` stores the body, material, rotation, and reference facts. Editable `OrbitParameters` hold double-precision semi-major axis `a`, eccentricity `e`, inclination, period `P`, and initial phase. The initial phase is mean anomaly at day zero. Each planet starts with `e = 0`, preserving the original circular system.

For simulation day `t`, the implementation reduces the mean anomaly before solving Kepler's equation:

```text
M(t) = initialPhase + 2π × remainder(t / P, 1), reduced modulo 2π
E − e sin(E) = M
x = a(cos(E) − e)
z = −a √(1 − e²) sin(E)
position(t) = Rz(inclination) × (x, 0, z)
```

Angles in these equations are radians; the editor presents degrees. Newton iteration solves for eccentric anomaly `E` in double precision. Eccentricity is limited to 0.85, and `SolarSystem::setOrbit()` validates all parameters and requires periapsis to clear the Sun plus the planet radius and a margin. A full period closes the orbit. When `e = 0`, the equations reduce to the original `x = a cos(M)`, `z = −a sin(M)` circle.

Velocity is the analytic derivative of position, with the same inclination transform:

```text
dE/dt = (2π / P) / (1 − e cos(E))
vx = −a sin(E) × dE/dt
vz = −a √(1 − e²) cos(E) × dE/dt
periapsis = a(1 − e)
apoapsis  = a(1 + e)
```

This produces faster motion near periapsis and slower motion near apoapsis. It is prescribed Keplerian motion, with no force integration. Independently editable `a` and `P` do not enforce Kepler's third-law relation for one shared gravitational parameter. Edits can cross other planetary trajectories; no mutual gravitational or collision response is simulated.

Simulation time advances by:

```text
simulatedDays += realDeltaSeconds × 6 × speedMultiplier
```

Time is stored as a double. Pausing prevents this addition. The model rejects negative or non-finite deltas. In interactive mode, a single unusually large delta is limited to 0.1 seconds to prevent a large jump after an operating-system stall. Automated capture mode uses a fixed 1/60-second step for reproducibility.

Axial spin uses the physical sidereal rotation period multiplied by 60. Venus is represented by a small tilt and a negative spin period. Uranus uses a tilt greater than 90 degrees, which produces retrograde rotation relative to the ecliptic without reversing it a second time.

## 7. Earth-Moon hierarchy

The Moon must travel with Earth, but it should not inherit Earth's surface spin or scale. Its model matrix is:

```text
Mmoon = T(EarthPosition)
      × Rz(lunar inclination)
      × Ry(lunar orbital angle)
      × T(lunar orbit radius, 0, 0)
      × S(Moon radius)
```

The Earth's translated frame is the parent transform. The lunar transform is evaluated relative to that frame. Because the Moon keeps the orbital rotation in its orientation, the same local hemisphere stays facing Earth: an educational demonstration of synchronous rotation.

This is a real hierarchy. If the Earth moves, the Moon's entire local orbit moves with it. A unit test checks that the Earth-Moon separation stays at the configured scene distance at many different times.

## 8. Lighting and materials

In the fragment shader, `vWorld` is the surface's world position. The Sun is at the origin, so the light direction is:

```glsl
vec3 N = normalize(vNormal);
vec3 L = normalize(-vWorld);
vec3 V = normalize(uEye - vWorld);
vec3 H = normalize(L + V);
```

Ambient illumination preserves a little detail on the dark side. Diffuse illumination is proportional to `max(dot(N,L),0)`. Blinn-Phong specular intensity is based on `pow(max(dot(N,H),0), shininess)`. A texture supplies the surface color. Earth's shader uses a simple blue-channel heuristic for stronger water highlights; this is an approximation rather than a physically based ocean model.

Normals use the inverse transpose of the model's upper-left 3×3 matrix:

```glsl
vNormal = transpose(inverse(mat3(uModel))) * aNormal;
```

This preserves perpendicularity under scaling. Even though the current planets use uniform scale, the correct transform demonstrates the general technique.

The Sun uses a separate emissive fragment shader, so it remains bright without receiving light from itself. Earth adds a night texture on the unlit hemisphere and a slightly larger rotating cloud sphere. An atmospheric shell uses a view-angle rim term and the light direction.

The Graphics tab multiplies the ambient, diffuse, and specular terms independently, with 1 as the default and a 0–3 control range. These controls are enabled in Shaded mode so their effect can be observed directly.

Saturn's ring fragment shader tests a ray toward the Sun against Saturn's sphere to form its shadow. The planet shader intersects a light ray with the ring plane and samples ring alpha to darken areas under the rings. These are analytic shadows; there is no general-purpose shadow map or full interplanetary eclipse solver.

## 9. Depth, transparency and post-processing

Opaque bodies write to the depth buffer. Depth testing hides back surfaces and resolves planet overlaps. Transparent surfaces are drawn afterward with depth testing still enabled but depth writing disabled. Rings disable face culling so they can be viewed from above or below. Standard alpha blending is used for rings and clouds; additive blending is used for atmospheric rims and stars.

The main scene renders into a floating-point HDR framebuffer. High quality uses up to four MSAA samples per pixel and resolves by blit into a single-sample texture. Low quality uses one sample per pixel and blits the HDR color into the same single-sample scene texture. The main scene retains its full resolution in both modes.

The first fullscreen pass samples linear HDR color `C` and writes `max(C - 1, 0)` per channel into the smaller Bloom buffer. `blur.frag` then applies a five-sample weighted filter using linear interpolation at fractional texel offsets from Daniel Rákos's [Gaussian blur reference](https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/). `I` is the current input and `a` is the one-texel vector along the pass axis:

```text
B(p) = 0.227027 I(p)
     + 0.316216 [I(p + 1.384615 a) + I(p - 1.384615 a)]
     + 0.070270 [I(p + 3.230769 a) + I(p - 3.230769 a)]
```

High quality uses half resolution in each dimension and ten alternating blur passes. Low uses quarter resolution and four passes. Two ping-pong textures allow each pass to sample the preceding result while writing to a different texture. The composite adds `0.75 × B` before applying exposure, Narkowicz's ACES-style tone curve, a 1/2.2 display power, and a light vignette. ImGui draws after the composite. The World normals mode skips the artistic transforms and displays direct normal RGB.

In Explore or Orbit Lab, **F6** toggles Bloom and returns to the final view. **F7** enables a split of one frame, with the left without Bloom and the right with Bloom. Only the final compositing expression differs across the split. The pipeline inspector can show the scene without Bloom, the extracted values, and the blurred highlights. Intermediate HDR images pass through the same display transform for inspection. The asteroid belt uses one instanced indexed draw for 640 deterministic rocks with per-instance attributes.

## 10. Camera behavior

An orbit camera is described by a center, distance, yaw and pitch. Its position is:

```text
eye = center + distance ×
      (cos(pitch) sin(yaw), sin(pitch), cos(pitch) cos(yaw))
```

Pitch is clamped below ±90 degrees to avoid a singular look-at direction. Mouse drag changes target angles. The mouse wheel changes target distance exponentially. WASD adds a deliberate world-plane pan offset.

For frame-independent smoothing, `alpha = 1 - exp(-5.5 × deltaTime)`. Angles approach their targets with this factor; zoom interpolates in log distance. During focus, a relative transition offset decays toward zero while the selected planet's **current** position is used every frame. This avoids the constant tracking lag that would occur if the moving target position itself were smoothed.

Collision corrections are added to the decaying transition offset, not the permanent user pan. The eye is kept outside the Sun, planets and Moon. This fixes a subtle case where flying between two destinations passes near another body and would otherwise leave the destination off center.

## 11. Robustness and performance

GPU buffers, arrays, textures, framebuffers and renderbuffers are owned by move-only RAII wrappers. Programs and shaders are released on normal destruction and on construction failures. Resource destruction occurs before GLFW termination. Shader compilation/linking logs contain the failing file. Texture failure triggers an in-memory procedural fallback; missing fonts use ImGui's built-in font.

The main renderer reuses one high-quality sphere mesh for celestial bodies. Static geometry and images are uploaded at initialization; the laboratory's trajectory, velocity, and normal-line buffers update as needed. The separate eclipse renderer owns its own reusable sphere. Uniform locations are cached, the star field uses one draw call, and bloom uses reduced-resolution targets. The scene has too few planets to justify a complex frustum-culling system.

The project checks OpenGL errors every rendered frame. CPU tests verify orbital closure, elliptical position/velocity, frame partition independence, pause/speed behavior, the lunar hierarchy, unit normals, triangle winding, camera tracking, experiment roundtrips and rejection, and eclipse visibility. Integration tests exercise the actual OpenGL renderers, laboratory transitions, and failure paths. The measured results and limits are in `TESTING.md`.

## 12. Orbit and graphics laboratory

The right panel in Orbit Lab has **Orbit** and **Graphics** tabs. Orbit changes apply immediately to the selected planet through `SolarSystem::setOrbit()`. The planet information API reflects its edited period; real-world distance facts remain references. Entering a simulation day and pressing Enter seeks and pauses. **Reset experiment** restores the original model and graphics settings, pauses, and keeps the current laboratory open.

The recent trajectory is reconstructed from the current orbit over the preceding 22% of its period rather than accumulating a history buffer. A seek, load, or edit therefore cannot leave points from an earlier experiment. The velocity vector uses the analytic derivative, with a display scale and length limits. Green and orange markers identify periapsis and apoapsis.

Wireframe uses `glPolygonMode(..., GL_LINE)` on the actual indexed geometry. World normals transforms each normal into world space and encodes it as `0.5 × normal + 0.5` in RGB. Short glyphs on the selected sphere illustrate the same direction using the inverse-transpose normal matrix. Diagnostic modes omit overlapping surface shells that would obscure the geometry. These views explain mesh topology and normal transformation; they do not represent physical surface colors.

## 13. Eclipse laboratory

The eclipse model is separate from the compressed eight-planet scene. It places a finite spherical Sun, Earth, and Moon on one coherent illustrative scale. Solar mode puts the Moon between the Sun and Earth; lunar mode puts Earth between the Sun and Moon. Phase and inclination move the Moon, and a separate degrees-per-second control animates phase. Changing inclination at alignment demonstrates why an orbital tilt can prevent an eclipse.

Two views share the same geometry: a top overview occupies about 38% of the available height, and a lower detail view occupies about 62%. The lower view can switch to an observer viewpoint. Purple lines show the umbra boundary and amber lines the penumbra boundary. These lines illustrate the shadow volumes; the shaded target uses per-point occlusion.

For a surface point, the CPU and shader compute the apparent angular radii of the Sun and occluder and their angular separation. If a body of radius `r` is distance `d` away, its apparent angular radius is `asin(r / d)`. A circle-overlap formula then estimates the fraction of the solar disk that remains visible. Disjoint disks give full sunlight; a larger enclosing occluder gives zero; partial overlap gives a fraction between them. The occluder must be between the surface point and the source.

The circle overlap is exact for circles in a plane of angular coordinates, but is a small-angle approximation to spherical-cap overlap. The CPU tests compare it against numerical integration on the sphere. The right-panel percentage and status are measured at the target's Sun-facing surface center; they do not classify the entire Earth or Moon. Different surface points can lie in full sunlight, penumbra, or umbra at the same time.

Total alignment uses phase 0° in both modes. Partial alignment uses 8° for solar and 20° for lunar; clear-of-shadow presets use 30° and 35°, respectively. Presets set inclination to zero and stop animation. The model explains occlusion and shadow geometry without predicting calendar dates, atmospheric scattering/refraction, or real eclipse paths.

## 14. Experiment persistence

Native Windows Save/Load dialogs use `.sselab` files. `ExperimentIO` writes a versioned `SSELAB 4` text record containing all eight orbital configurations, simulation day/speed/pause, selected planet, graphics/overlay settings, active laboratory, eclipse and workshop settings, challenge progress, and a `techniques` record (comet, Bézier control polygon, shading model, low-poly spheres, projection, stencil outline). Version 4 adds the Workshop light direction, asteroid visibility, and render quality. Versions 1–3 still load with defaults for fields their formats omit. Bloom comparison and pipeline inspection are session controls. Numeric serialization uses a fixed locale and sufficient precision for roundtrips.

Loading parses into temporary model and settings objects. The parser checks the version, record order/counts, finite values, supported ranges, and orbital clearance before replacing live state. Invalid or truncated files report an error without partially modifying the experiment. Saving validates state and writes through a temporary file before replacing the destination.

Camera pose is deliberately absent from the format. After loading, `Application` focuses the restored selection or returns to overview. The model and persistence tests do not need a graphics context; the laboratory smoke scenario additionally exercises save/load inside the application.

## 15. Workshop light and challenges

Workshop's Surface tab exposes azimuth and elevation under **Lighting**. In world coordinates the unit light vector is `(cos(e) sin(a), sin(e), cos(e) cos(a))`, where `a` is azimuth and `e` is elevation in radians. The model converts the displayed degrees and validates the supported ranges. The same vector drives terrain and atmospheric lighting. **Reset lighting** restores the default direction and term multipliers. Changing light direction moves the terminator of a paused world. World normals bypasses lighting and encodes surface direction directly.

The five challenges use predicates over the live experiment. **Read the normals** succeeds when Workshop is active, its world is paused, and World normals is selected. **Check challenge** validates these conditions and updates progress. Players can change settings and retry, reset challenge progress, or save and reload their state.

## 16. Likely defense questions

**Why use indexed geometry?** Reusing vertices reduces duplicated data and allows the GPU's post-transform cache to avoid repeated vertex work.

**What is the difference between a VBO and a VAO?** A VBO stores vertex data. A VAO records how vertex attributes are read, including associated buffers. The EBO stores indices.

**Why does transform order matter?** Matrix multiplication is not commutative. Rotating a translated object can move it around the origin, while translating a rotated object moves its already-rotated local frame.

**Why is the Moon not multiplied by Earth's full model matrix?** That would incorrectly apply Earth's size and surface spin to the Moon's orbit. It inherits only the parent's translation.

**Why normalize directions in the fragment shader?** Interpolated normals may lose unit length. Dot products represent cosines only when the vectors are normalized.

**Why use delta time?** Movement should depend on elapsed time rather than the number of frames. A 120 FPS computer should not simulate twice as fast as a 60 FPS computer.

**Why use a double for time but float matrices?** Time accumulates over many frames and benefits from extra precision. The GPU uses float transforms efficiently, after periodic angles are reduced to small values.

**What makes the Sun glow?** Its shader emits values above display white into an HDR target. Bloom spreads those bright values before tone mapping.

**Why draw transparent geometry after opaque geometry?** Opaque surfaces first establish the depth buffer. Transparency can then blend with the already-rendered scene without incorrectly blocking later transparent fragments.

**Is this a physically exact Solar System?** No. It is a graphics project with illustrative geometry scales and simplified lighting. The default circular system preserves meaningful relative orbital periods. Edited ellipses follow Kepler's equation, but their size and period can be changed independently and no N-body forces are integrated.

**Why does speed vary on an ellipse?** Mean anomaly advances uniformly, but eccentric anomaly solves `E − e sin(E) = M`. The derivative contains `1 / (1 − e cos(E))`, giving faster motion near periapsis. The velocity arrow uses that analytic derivative.

**Does the eclipse percentage describe the whole planet?** No. It samples the Sun-facing surface center. The renderer evaluates visibility at other surface points, which can be in different shadow regions.

**What does saving reproduce?** Orbits, time, selection, rendering controls, active laboratory, and eclipse settings. It does not reproduce the exact camera pose.

**How would you extend it?** Add ephemeris data, physically linked orbital size and period, perturbations, and a general shadowing solution for the whole system. These are future extensions beyond the current illustrative laboratories.

**Can you prove there are no memory leaks?** RAII and context-aware destruction reduce the risk, and the available checks are reported honestly. A short successful run is not a formal proof of zero leaks across all GPU drivers.

## 17. Rendering techniques laboratory

These features were added to cover the course rubric items on shading models, projections, buffers and parametric curves. All of them are reachable from Orbit Lab's **Graphics** tab and from keyboard shortcuts in Explore.

### 17.1 Gouraud, Phong and Blinn-Phong

`planet.vert` and `planet.frag` share a `uShadingModel` uniform:

| Model | Where lighting is evaluated | Specular term |
| --- | --- | --- |
| Blinn-Phong (default) | Fragment shader, per pixel | `pow(max(dot(N, H), 0), s)` with `H = normalize(L + V)` |
| Phong | Fragment shader, per pixel | `pow(max(dot(reflect(-L, N), V), 0), s / 4)` |
| Gouraud | Vertex shader, per vertex | Blinn-Phong at each vertex, then interpolated |

In Gouraud mode the vertex shader outputs `vNdl` and `vSpec`; the rasterizer interpolates those scalars and the fragment shader only multiplies them with the texture colour. A highlight that falls inside a large triangle is therefore lost or smeared. The **Low-poly spheres** option swaps the 128 × 64 sphere for a 16 × 8 one to make this visible. Phong uses a quarter of the Blinn exponent because the angle between `R` and `V` is roughly twice the angle between `N` and `H`; this keeps the highlight size comparable so that only its shape differs.

### 17.2 Perspective and orthographic projection

`Camera::orthographicProjection` builds `glm::ortho` with a half-height of `distance * tan(fov / 2)`. At the focus distance this is exactly the height the perspective frustum shows, so both projections map the focus plane identically and switching does not change the selected planet's size. For the same reason a linear blend of the two matrices also preserves the focus plane, which the camera uses to animate the switch. In the orthographic matrix `w` is always 1, so there is no perspective divide and no foreshortening. The off-centre panel offset becomes a translation (`p[3][0]`) instead of a shear (`p[2][0]`). The sky and stars represent directions at infinity and always use the perspective matrix.

### 17.3 Stencil-buffer outline

The multisampled scene framebuffer uses a packed `GL_DEPTH24_STENCIL8` renderbuffer. While the selected planet is drawn, `glStencilFunc(GL_ALWAYS, 1)` with `GL_REPLACE` marks its visible pixels. Next, a copy scaled by about three pixels is drawn in a flat HDR colour with `glStencilFunc(GL_NOTEQUAL, 1)` and stencil writes disabled. Only the ring outside the original silhouette survives, and bloom turns it into a glow. The scale is computed from the projection (`2w / (P[1][1] * height)` world units per pixel), so the width stays constant in pixels for both projections.

### 17.4 Bézier comet and De Casteljau's algorithm

`deCasteljau` evaluates a Bézier curve of any degree by repeated linear interpolation:

```cpp
for (level = n - 1; level > 0; --level)
    for (i = 0; i < level; ++i)
        p[i] = mix(p[i], p[i + 1], t);
return p[0];
```

`BezierLoop` joins cubic segments into a closed path. Each anchor stores a handle vector `h`. The outgoing control point is `anchor + h`, and the incoming one is `anchor - h`. Because the two handles are collinear and equal in length, the tangent is continuous at every joint (C1). The derivative is evaluated as the quadratic **hodograph** with control points `3(P1-P0), 3(P2-P1), 3(P3-P2)`.

The comet path uses four anchors at the ends of an ellipse's axes. The handle ratio 0.5523 approximates a quarter ellipse, and the perihelion handle is shortened so the curve bends tighter around the Sun. A uniform Bézier parameter does not mean uniform speed. The constructor therefore integrates `dt = |dP/du| du / v(r)` with `v ~ sqrt(2/r - 1/a)` (vis-viva) and inverts the resulting table at run time. The comet is about nine times faster at perihelion than at aphelion.

The tails are point sprites with additive blending. The ion tail is a straight line away from the Sun. Each dust particle is placed where the comet was `age` days ago and is pushed outward by `age * push`, so the dust tail curves behind the comet. Brightness scales with `(12 / r)^1.6`.

`tests/BezierTests.cpp` checks that De Casteljau agrees with the Bernstein form and that endpoints are interpolated. It also tests C0 and C1 continuity at every joint, periodicity, monotonic motion and perihelion/aphelion speeds. `CoreTests` checks the projection invariants.

### 17.5 Defense questions for these techniques

**Why does Gouraud shading lose highlights?** Lighting is computed only at vertices. If a specular peak falls between vertices, the interpolation never reaches it.

**Why is Blinn-Phong the default?** The half vector is cheaper than a reflection and behaves better at grazing angles; it is also what fixed-function OpenGL used.

**When would you use an orthographic projection?** For CAD, maps, 2D UI and technical views where sizes must be comparable regardless of depth.

**What is the stencil buffer?** It is an integer per-sample buffer that is tested and updated alongside depth. It is commonly used for outlines, mirrors, portals and shadow volumes.

**Why use De Casteljau instead of the Bernstein polynomial?** It uses only convex combinations, so it is numerically stable. It also works for any degree, and its intermediate points split the curve into two Bézier curves.

**What continuity does the comet path have?** It has C1 continuity: positions and first derivatives match at joints because the handles are collinear and equal in length.
