# Review and implementation decisions

An independent, read-only review examined the actual C++/GLSL files, requirements, plan and tests. It found no Critical issues and two Important issues. Both were fixed and covered by regression checks.

| Finding | Resolution | Evidence |
|---|---|---|
| Collision avoidance stored an escape correction in permanent pan | Store it in the exponentially decaying transition offset | Mars→Venus and Uranus→Mercury focus tests failed before correction and pass afterward; 100× Mercury tracking also passes |
| Escape from the speed slider text editor also closed the application | Preserve UI input ownership before widgets process Escape | Real ImGui input injection; temporarily disabling the fix reproduces premature exit, restoring it passes |
| Validation did not drive actual OS minimize/restore | Kept as an explicit verification limit | Renderer zero-size guard is tested; OS transition is not claimed |

The reviewer additionally checked GPU destruction and constructor unwinding, resource discovery, shader failures, texture fallback, framebuffer resizing, model math, geometry, offline CMake and input capture. Visual quality and performance measurements were inspected/performed by the implementer, not independently certified by the reviewer.

Other bugs found by direct testing were initial ImGui focus swallowing destination shortcuts and Windows Unicode path conversion. Both have regression coverage.

## Decisions

- Implement directly from the user's detailed specification and instruction to begin immediately. No extra design-approval pause was introduced.
- Keep the self-contained project and deliverables in `outputs/`; use `work/` for builds and scratch scripts. This avoids cluttering the source distribution with machine-specific build files.
- Vendor fixed dependency sources rather than require vcpkg or an online first build. This costs some archive size and makes a university presentation easier to reproduce offline.
- Use compressed radii/distances, circular prescribed orbits and slowed axial rotation. This improves visibility but is unsuitable for scientific predictions; the limitations are visible in help and documentation.
- Use the installed VS 2026 compiler for actual verification and supply a VS 2022 preset as requested. A separate VS 2022 run remains outside the verified environment.
- Preserve a renderer zero-dimension guard and disclose that native minimize/restore automation was not completed after external input interrupted window automation. This leaves that OS transition as a manual verification item, not an unimplemented feature.

There are no known unresolved Important/Critical findings. No feature is implemented as a placeholder.
