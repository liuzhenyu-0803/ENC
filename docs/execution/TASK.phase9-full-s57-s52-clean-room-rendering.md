# `navscene-sdk` Phase 9 Task

Status: in_progress

## 1. Phase Goal

Phase 9 establishes the new main execution track:

- [ ] implement a clean-room but substantially complete `S-57 -> S-52` portrayal engine for ENC loading, selection, portrayal, and rendering
- [ ] use OpenCPN as behavior and table reference while keeping repository implementation non-GPL and architecture-owned by this project
- [ ] keep the portrayal pipeline backend-agnostic so Vulkan remains the default runtime backend while software validation and future backends consume the same scene output

## 2. Scope

### 2.1 In Scope

- [ ] expand portrayal coverage from the current subset into a broad `S-52` object-class implementation suitable for commercial ENC viewing
- [ ] introduce a proper clean-room instruction and rule execution layer for `AC`, `AP`, `LS`, `SY`, `TX`, `TE`, and `CS` style behavior
- [ ] implement the highest-value conditional symbology procedures needed for real ENC parity, using OpenCPN logic as behavior reference rather than source implementation
- [ ] build reusable symbol, pattern, text, palette, and priority subsystems instead of continuing only with ad-hoc one-off class branches
- [ ] keep the SDK public API `C++` only and free of Qt, Vulkan, GDAL, and OpenCPN-specific types

### 2.2 Out Of Scope

- [ ] `S-100` portrayal implementation
- [ ] raster basemap, satellite, and 3D rendering products
- [ ] UI parity with OpenCPN or plugin parity
- [ ] direct reuse of GPL implementation code

## 3. Architectural Rules For This Phase

- [ ] OpenCPN table values, palette values, lookup values, and standard-derived symbol values may be transcribed when needed
- [ ] OpenCPN code structure may be studied as a behavior reference, but repository implementation must remain clean-room and fit the existing SDK layering
- [ ] new portrayal logic must prefer data-driven execution over adding many more hard-coded branches in `engine.cpp`
- [ ] symbols, patterns, text, priorities, and conditional procedures must stay representable in a backend-neutral portrayal scene

## 4. Work Breakdown

### 4.1 M1 Portrayal Data Model And Execution Core

- [x] define the target clean-room portrayal instruction model for area, line, point, text, and conditional operations
- [x] decide what remains in the static catalog versus what moves into an executable portrayal-rule layer
- [x] refactor the current portrayal engine so rule execution is modular instead of accumulating object-class-specific logic in one file
- [x] preserve compatibility with the current scene and renderer pipeline during migration

### 4.2 M2 Lookup Coverage Expansion

- [ ] inventory OpenCPN `chartsymbols.xml` object-class coverage and classify what is required for core ENC portrayal
- [ ] expand the clean-room lookup catalog to cover the core ENC classes currently missing from the SDK
- [ ] replace generic fallback rendering for the highest-impact missing classes with real `S-52` portrayal behavior
- [ ] keep the catalog maintainable by splitting large files when responsibility grows too broad

### 4.3 M3 Conditional Symbology Parity

- [ ] implement clean-room equivalents of the core conditional procedures driving ENC readability and hazard semantics
- [ ] prioritize at least:
  - `DEPARE01`
  - `DEPCNT02`
  - `LIGHTS05`
  - `OBSTRN04`
  - `QUAPOS01`
  - `QUALIN01`
  - `RESARE02`
  - `SEABED01`
  - `SOUNDG02`
  - `TOPMAR01`
  - `UDWHAZ03`
  - `WRECKS02`
- [ ] support cross-object and depth-aware logic where `S-52` semantics depend on surrounding `DEPARE`, `DRGARE`, `UNSARE`, or related features

### 4.4 M4 Symbol, Pattern, And Text Fidelity

- [ ] build a reusable clean-room symbol and pattern system instead of only procedural placeholders
- [ ] improve repeated area patterns, boundary patterns, and special symbol placement toward standard `S-52` behavior
- [ ] extend text generation and label styling toward fuller ENC readability, including object-specific formatting and priority handling
- [ ] keep day, dusk, and night palettes unified through named chart-color resolution

### 4.5 M5 Verification And Conformance Tracking

- [ ] keep expanding automated regression coverage as each rule family lands
- [ ] add object-class and conditional-procedure coverage reporting so progress is measurable from repository artifacts
- [ ] maintain reference-render validation artifacts for bundled samples while documenting exactly what remains unmatched
- [ ] update execution and validation docs after every accepted sub-task

## 5. Initial Code-Level Gap Snapshot

Current starting position relative to OpenCPN behavior:

- [x] `S-57` loading, chart-scene construction, base display settings, and backend-neutral portrayal scene are already in place
- [x] the SDK already supports a meaningful `S-52` subset for key area, line, and point classes
- [ ] the current lookup catalog still covers only a subset of the object classes present in OpenCPN lookup tables
- [ ] the current portrayal engine still relies too heavily on object-class-specific hard-coded branching instead of a fuller rule execution system
- [ ] many important `S-52` conditional procedures are still absent or only approximated
- [ ] symbol, pattern, and text handling still remains materially below full ENC product behavior

## 6. Acceptance

- [ ] core ENC object portrayal no longer depends primarily on generic fallback rendering
- [ ] the portrayal stack exposes a maintainable clean-room execution model for `S-52` lookup and conditional behavior
- [ ] the highest-value OpenCPN conditional procedures have clean-room equivalents in this repository
- [ ] Vulkan demo rendering and software validation both consume the same richer portrayal scene without backend-specific rule duplication
- [ ] repository documentation records coverage, validation evidence, and remaining deltas clearly enough for new sessions to continue autonomously

## 7. Immediate Next Batch

- [x] extract and document the current code-to-OpenCPN mapping for lookup coverage, conditional procedure coverage, and remaining missing subsystems
- [x] define the first clean-room execution-layer refactor that moves current hard-coded portrayal rules toward modular instruction execution
- [x] start with the core procedures that most strongly affect ENC semantics and hazard readability, not cosmetic edge cases

## 8. Progress Notes

- [x] 2026-04-12: split the monolithic portrayal engine into modular internal files:
  `engine_common.cpp`, `engine_area.cpp`, `engine_line.cpp`, `engine_point.cpp`,
  `engine_text.cpp`, and `engine_internal.h`, leaving `engine.cpp` as orchestration.
- [x] 2026-04-12: verified the refactor batch by rebuilding `navscene` plus
  `navscene-portrayal-mvp`, `navscene-portrayal-catalog`, and `navscene-label-layout`,
  then executing those three test binaries successfully from `build-gdal-verify`.
- [x] 2026-04-12: documented the current OpenCPN gap snapshot in
  `docs/reference/PHASE9.opencpn-gap-map.md`, including lookup coverage counts,
  conditional-procedure counts, and the next high-value missing class families.
- [x] 2026-04-12: defined the first clean-room execution-layer model in
  `docs/architecture/PHASE9.clean-room-execution-model.md` and introduced
  `rule_program_internal.h` plus `rule_program.cpp` to migrate stable area/line
  portrayal rules out of direct object-class branching.
- [x] 2026-04-12: migrated `DEPCNT` safety-contour and low-accuracy line behavior
  plus point-hazard execution for `OBSTRN`, `UWTROC`, and `WRECKS` into the
  Phase-9 execution layer, including clean-room estimated-depth handling for
  hazard points without explicit `VALSOU`.
- [x] 2026-04-12: verified the above batch by rebuilding
  `navscene-portrayal-mvp`, `navscene-portrayal-catalog`, and
  `navscene-label-layout` in `build-gdal-verify`, then executing all three
  binaries successfully.
- [x] 2026-04-12: migrated `TOPMAR` point portrayal into the same execution layer
  so stable point-family derivation is no longer concentrated in
  `engine_point.cpp`, then re-ran the same three verification binaries
  successfully.
- [x] 2026-04-12: split oversized `src/portrayal/rule_program.cpp` into
  `rule_program.cpp`, `rule_program_line.cpp`, and `rule_program_point.cpp`
  so the execution layer stays maintainable as Phase 9 coverage expands.
- [x] 2026-04-12: verified the rule-program split by rebuilding
  `navscene-portrayal-mvp`, `navscene-portrayal-catalog`, and
  `navscene-label-layout` in `build-gdal-verify`, then executing all three
  binaries successfully with the split compilation units.
- [x] 2026-04-12: migrated low-accuracy shoreline handling further into the
  clean-room execution layer by adding `COALNE` and `SLCONS` line behavior plus
  `SLCONS` area behavior, replacing the old direct procedural fallback for that
  area family.
- [x] 2026-04-12: introduced the first supplemental point-decoration path so
  point features with low `QUAPOS` metadata can emit an additional clean-room
  positional-quality marker on top of the base point symbol.
- [x] 2026-04-12: verified the low-accuracy shoreline and point-decoration batch
  by rebuilding `navscene-portrayal-mvp`, `navscene-portrayal-catalog`, and
  `navscene-label-layout` in `build-gdal-verify`, then executing all three
  binaries successfully with the new regression checks.
- [x] 2026-04-12: fixed the Windows demo runtime deployment blocker by updating
  `demo/qt/CMakeLists.txt` to copy Qt runtime files plus the GDAL/OSGeo4W DLL
  dependency chain into the demo output directory, then directly deploying the
  current runtime libraries into all existing `navscene-demo-qt.exe` Debug
  output folders.
- [x] 2026-04-12: repaired non-Vulkan GDAL builds so `build-gdal` and
  `build-gdal-osgeo` can rebuild `navscene-demo-qt` successfully by always
  compiling `render/vulkan_backend.cpp` for its clean stub path and falling
  back from a null Vulkan backend to the software backend in
  `src/render/backend.cpp`.
- [x] 2026-04-12: validated the above runtime and build-system repair by
  launching all four current demo binaries successfully:
  `build`, `build-gdal`, `build-gdal-osgeo`, and `build-gdal-verify`.
