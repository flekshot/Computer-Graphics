# Project explanation: Solar System Explorer

## 1. Purpose

This project demonstrates how a modern real-time graphics application is built from geometry, matrices, shaders and interactive input. The user can explore a complete educational Solar System, select any of eight planets, follow its movement, control time and inspect the Earth-Moon hierarchy and Saturn's rings.

The objective is to connect the concepts from Computer Graphics Fundamentals in one working application. It is not a scientifically accurate orbit prediction tool. Distances and sizes are compressed so all planets can be explored comfortably. Orbital periods preserve real relative ratios; axial rotation is slowed for readability.

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

`Application` owns the GLFW lifetime. In its `run()` method it constructs a `SolarSystem`, `Camera`, `Renderer` and `UIManager`. They are local objects, so they are destroyed before the application's destructor destroys the OpenGL context.

`Planet` and `SolarSystem` contain data and mathematical behavior, with no OpenGL dependency. `Geometry` produces CPU arrays. `Mesh`, `Shader`, `Texture` and `GLObject` own GPU resources. `Renderer` draws the scene. `UIManager` presents widgets without becoming responsible for orbital mechanics.

Every normal frame follows these steps:

1. Poll GLFW events and measure elapsed real time.
2. Skip rendering if the framebuffer is zero-sized; wait briefly for new events.
3. Resize render targets if needed and begin an ImGui frame.
4. Build UI controls and process input, respecting active UI widgets.
5. Advance simulated time, update camera interpolation and avoid solid bodies.
6. Draw the sky, stars, Sun, planets, Moon, orbits, rings, clouds and atmosphere.
7. Resolve MSAA, apply bloom and tone mapping, then draw the interface.
8. Check OpenGL errors and swap the front/back buffers.

All planets use the same day counter regardless of which one is selected or visible. Camera selection never stops another planet's simulation.

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

The ring is an indexed annulus with inner and outer vertices at each angular sample. Orbits use a reusable unit circle drawn with `GL_LINE_LOOP`. Stars are generated with a fixed random seed on a distant sphere and drawn as point sprites in one call.

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

Each `PlanetData` entry stores radius, orbit radius, orbital period, rotation period, tilt, inclination, initial phase, texture, display color and facts.

For orbital period P in simulated days and day counter t:

```text
angle(t) = initialPhase + 2π × remainder(t / P, 1)
O(t) = Rz(inclination) × Ry(angle(t)) × T(orbitRadius, 0, 0)
position(t) = O(t) × (0, 0, 0, 1)
```

With GLM's positive Y rotation, the uninclined coordinates are `x = r cos(angle)` and `z = -r sin(angle)`. A full period returns to the same position. Reducing angles before converting to float protects rendering precision over long runs.

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

Saturn's ring fragment shader tests a ray toward the Sun against Saturn's sphere to form its shadow. The planet shader intersects a light ray with the ring plane and samples ring alpha to darken areas under the rings. These are analytic shadows; there is no general-purpose shadow map or full interplanetary eclipse solver.

## 9. Depth, transparency and post-processing

Opaque bodies write to the depth buffer. Depth testing hides back surfaces and resolves planet overlaps. Transparent surfaces are drawn afterward with depth testing still enabled but depth writing disabled. Rings disable face culling so they can be viewed from above or below. Standard alpha blending is used for rings and clouds; additive blending is used for atmospheric rims and stars.

The scene renders into a floating-point HDR multisample framebuffer. Up to four samples per pixel smooth sphere edges and orbit lines. A blit resolves it into a single-sample texture. Bright pixels are extracted and blurred in alternating horizontal and vertical passes using two half-resolution framebuffers. The final shader adds bloom, applies exposure and an ACES-style tone curve, then converts linear color for display. The ImGui interface is drawn after this stage and remains crisp.

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

Only one high-quality sphere mesh is stored on the GPU. Geometry and image uploads happen at initialization. Uniform locations are cached. The star field uses one draw call. Bloom uses reduced-resolution targets. The scene has too few planets to justify a complex frustum-culling system.

The project checks OpenGL errors every rendered frame. CPU tests verify orbital closure, frame partition independence, pause/speed behavior, the lunar hierarchy, unit normals, triangle winding and camera tracking. Integration tests exercise the actual OpenGL renderer and failure paths. The measured results and limits are in `TESTING.md`.

## 12. Likely defense questions

**Why use indexed geometry?** Reusing vertices reduces duplicated data and allows the GPU's post-transform cache to avoid repeated vertex work.

**What is the difference between a VBO and a VAO?** A VBO stores vertex data. A VAO records how vertex attributes are read, including associated buffers. The EBO stores indices.

**Why does transform order matter?** Matrix multiplication is not commutative. Rotating a translated object can move it around the origin, while translating a rotated object moves its already-rotated local frame.

**Why is the Moon not multiplied by Earth's full model matrix?** That would incorrectly apply Earth's size and surface spin to the Moon's orbit. It inherits only the parent's translation.

**Why normalize directions in the fragment shader?** Interpolated normals may lose unit length. Dot products represent cosines only when the vectors are normalized.

**Why use delta time?** Movement should depend on elapsed time rather than the number of frames. A 120 FPS computer should not simulate twice as fast as a 60 FPS computer.

**Why use a double for time but float matrices?** Time accumulates over many frames and benefits from extra precision. The GPU uses float transforms efficiently, after periodic angles are reduced to small values.

**What makes the Sun glow?** Its shader emits values above display white into an HDR target. Bloom spreads those bright values before tone mapping.

**Why draw transparent geometry after opaque geometry?** Opaque surfaces first establish the depth buffer. Transparency can then blend with the already-rendered scene without incorrectly blocking later transparent fragments.

**Is this a physically exact Solar System?** No. It is a graphics project with illustrative geometry scales, circular prescribed orbits and simplified lighting. It preserves meaningful relative orbital periods and demonstrates the required graphics concepts.

**How would you extend it?** Replace circular trajectories with Keplerian orbital elements, add an ephemeris dataset and a general shadowing solution. These are future extensions, not hidden unfinished requirements of the delivered application.

**Can you prove there are no memory leaks?** RAII and context-aware destruction reduce the risk, and the available checks are reported honestly. A short successful run is not a formal proof of zero leaks across all GPU drivers.
