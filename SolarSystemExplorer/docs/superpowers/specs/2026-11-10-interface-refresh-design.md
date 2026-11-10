# Solar System Explorer interface refresh

The requested work is to repair the audited failures and give the existing native C++ application a fresh, animated interface. Keep the offline OpenGL/ImGui architecture and all existing laboratory functions.

Visual direction: an observatory console with an open view of space, floating translucent navy panels, a compact animated mode selector, planet-colored destination cards and a clear time-control deck. Use deep space `#080D19`, panel `#111D32`, readable secondary `#9CAFC8`, foreground `#EAF1FB`, aqua `#66DAD1`, and solar amber `#FFB36B`. Use the bundled Roboto with Cyrillic glyphs, a large object name, sentence-case labels and quiet supporting text. Avoid new runtime dependencies.

Motion answers actions: a sliding mode indicator, eased destination selection/hover, short content reveals and smooth notification entrance. A single subtle orbital emblem supplies ambient life. Provide a Motion toggle that stops decorative animation and makes transitions immediate. Keep hit targets fixed while animating decoration, preserve keyboard focus, and fit 1000x640 through 1440x900 and tall windows.

Repair incomplete Workshop integration rather than remove the already-declared mode and tests. Implement its existing design, version 2 experiment persistence with version 1 compatibility, CLI overrides, independent preview camera and challenge checks. Repair cloud rotation continuity, eclipse projection for portrait windows, and inconsistent overview navigation. An accidental duplicate GLEW global declaration must be removed for compilation; leave all other bundled dependency code intact.

Verification: run the complete Release CTest suite, test actual ImGui navigation/animation/input ownership through the integration scenarios, inspect framebuffer captures for all four modes at normal and compact sizes, and obtain an independent final review. No publishing, commits or unrelated new features are part of this task.
