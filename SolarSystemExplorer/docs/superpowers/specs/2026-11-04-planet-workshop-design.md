# Planet Workshop

The user approved a procedural planet workshop and interactive challenges for a more distinctive graphics coursework demonstration. Extend the existing C++17/OpenGL application without downloads or changes to the fixed eight-planet simulation.

Add a fourth navigation mode with a separate camera and a seed-based planet preview. Rocky worlds have displaced terrain, adjustable ocean level, polar ice, surface/ocean colors, clouds and atmosphere. Gas giants have adjustable bands, turbulence and colors. Rotation can be paused and adjusted. Four presets provide a useful starting point. The same seed and settings reproduce the same world. Wireframe and normal visualization expose the actual generated geometry.

Save and load the whole experiment, including workshop settings and completed challenges. Version 2 files must remain strictly validated and atomically written; version 1 files must still load with default workshop/progress values. Explicit CLI options override files. Existing astronomy, orbit, eclipse and graphics behavior remains available.

Five challenges require an explicit check: a rocky ocean world with estimated coverage 45–75% and relief at least 0.10; a gas giant with at least 12 bands and turbulence at least 0.50; a paused workshop world in normal mode; a selected orbit with eccentricity 0.50–0.70 and absolute inclination at least 15 degrees; an eclipse observer with 20–80% of the Sun visible at the target center. Starting a challenge selects its mode but does not create the answer. Completed progress is saved; resetting a world preserves progress, with a separate deliberate progress reset.

Implementation choices made by the builder: normalized three-dimensional seeded terrain avoids longitude seams; oceans flatten submerged terrain; CPU sampling estimates ocean area; a dedicated renderer displays clouds and an illustrative atmosphere. No physical atmosphere simulation or integration of custom worlds into the orbital model is implied. English UI matches the application. Navigation and panels must fit 1000×640 and 1440×900.

Verification covers deterministic geometry, finite normals, parameter boundaries, old/new persistence, challenge predicates, all rendering modes, scene navigation, resize and state restoration. Produce two example worlds, screenshots, updated documentation and a refreshed portable Windows package. Preserve the four existing team credits.
