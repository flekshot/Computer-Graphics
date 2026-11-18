# Final assignment completion plan

**Goal:** Complete the agreed assignment gaps and graphics additions for Topic 2, Solar System.
**Architecture:** Extend the native C++17 / OpenGL 3.3 application from current main. Independent workers own lighting/persistence, main rendering, and submission documents; the coordinator owns application input, shared UI, capture/benchmark integration, build and verification.
**Authorization:** The user requested all previously proposed items, in sequence, with parallel subagents. Proceed without another design approval. Preserve the existing renderer and user preference against large targeting rings.

## Global constraints
- Keep OpenGL 3.3, offline resources, existing laboratories and SSELAB 1–3 compatibility.
- Do not invent team contributions, group, missing Section 6 requirements, performance results, or upload links.
- Keep the current working directory; a dedicated feature branch preserves the previous branch and uses current origin/main.
- No shared-branch push, publication, process termination, or replacing a running portable executable.
- Scope workers to distinct files. Coordinator alone performs integrated builds and test runs to avoid shared build races.

## Tasks
- [x] 1. Reconcile current main and obtain a working build toolchain. Original cache could not build; integrated verification used the recovered compiler rather than claiming an untouched baseline run.
- [x] 2. Workshop movable light with direction controls, reset, normal-mode invariance and validated SSELAB 4 persistence.
- [x] 3. Main rendering: Bloom split comparison, pipeline stages, Low/High quality and instanced asteroid belt.
- [x] 4. F6 Bloom hotkey, visible effect state, project/team card, graphics controls and contextual help.
- [x] 5. Reproducible 1280x720 performance table, graphics/input regression checks, updated screenshots and portable build.
- [x] 6. Honest six-page PDF report with pipeline diagram, equations, references, authorship disclosure, pending factual contribution fields, 5-minute script and verified 60-second real application video. External metadata and hosted link remain pending and explicitly marked.
- [x] 7. Independent review and final verification complete; source/portable packaging excludes toolchain and raw historical logs. Externally blocked submission items are documented.

## Shared interfaces
RenderSettings: `bool bloomCompare=false, asteroids=true; int pipelineStage=0, quality=1;`.
Pipeline stages: 0 final composite, 1 scene before bloom, 2 extracted highlights, 3 blurred highlights. Comparison uses one camera and scene; left without bloom, right with bloom. Diagnostic normals bypass all artistic processing. Quality 0 low, 1 high. Comparison/stage are session inspection tools; asteroids/quality persist.
Workshop light fields are owned by the lighting worker; its direction helper must also feed the shader. SSELAB 4 adds validated workshop light and presentation records while preserving older defaults.

## Review focus
Keyboard capture while editing widgets; normal RGB unchanged by effects; corrupt/new-version experiment loading remains atomic; resize and compact UI; recording reflects actual rendering and performance uses measured workload.

## Progress ledger
- 2026-11-18: Created feat/final-requirements from origin/main fd658f5. Existing build cache points to missing D:/studio; investigating installed compiler before baseline.
- Recovered a verified portable GCC16.2 toolchain without global installation. Initial integrated tests 17/17 passed; final suite 18/18 passed after independent review identified and verified the demo+capture duration correction.
- Four real visible-window 1280x720 measurements and a 60-second H.264 video completed, all zero GL errors. Source reviewers found no outstanding Critical/Important issues.
- Ruling: group/contribution facts, missing assignment Section 6 and a hosted video URL cannot be invented. Local report/video will be packaged with those fields explicitly pending. Google Drive connection suggested; no connection is confirmed.
