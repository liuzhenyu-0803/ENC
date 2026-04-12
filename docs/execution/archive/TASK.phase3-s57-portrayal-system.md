# `navscene-sdk` Phase 3 Task

Status: archived and completed on `2026-04-11`

## 1. Phase Goal

Phase 3 focused on building the first clean-room `S-57` portrayal system for the SDK:

- [x] turn the current geometry-first ENC view into a rule-driven chart portrayal pipeline
- [x] make the demo visually closer to a real ENC chart for the most visible chart objects
- [x] keep portrayal logic renderer-independent and host-framework agnostic
- [x] keep the design compatible with future `S-100`, overlays, raster basemaps, and multiple render backends

## 2. Reference Findings From OpenCPN

These are behavioral and architectural findings only. No GPL code or structure is copied.

### 2.1 What OpenCPN Separates Clearly

- `S-57` reading and portrayal are separated responsibilities
- display settings, lookup selection, conditional rules, symbol assets, and render ordering are treated as one connected system

### 2.2 What `navscene-sdk` Took From Those Findings

- portrayal is implemented as a dedicated module boundary
- render backends consume portrayal output instead of raw `S-57` features
- user-visible ENC display settings map to a stable internal `DisplaySettings` model
- clean-room external portrayal data drives colors, text roles, styles, and lookup rules

## 3. Scope And Non-Goals

### 3.1 Delivered In Phase 3

- clean-room portrayal catalog for the first supported `S-57` object set
- rule lookup by feature class, geometry kind, attributes, and display settings
- first-pass conditional portrayal for core ENC categories
- display-category support and core mariner-style settings
- text, symbol, line, and area portrayal emitted as renderer-independent scene commands
- priority ordering, scale filtering, and first-pass declutter
- automated regression checks against bundled ENC and reference image assets

### 3.2 Explicitly Left For Later Phases

- full `S-52` compliance
- `S-100` feature delivery
- raster basemaps or satellite imagery delivery
- pattern fills and full symbol-atlas fidelity
- 2.5D / 3D rendering delivery
- Android delivery
- full ECDIS feature parity with OpenCPN

## 4. Deliverable Definition

Phase 3 is considered complete because the project now has a usable portrayal baseline with these properties:

- [x] the demo loads the bundled ENC sample and renders it with recognizable chart semantics
- [x] sea, land, depth areas, coastlines, contours, key navigational objects, and labels no longer render as placeholder geometry
- [x] portrayal behavior is driven by reusable data and rules, not hard-coded per-backend styling
- [x] the public SDK still exposes `C++` interfaces only and does not leak `Qt` or backend-native types

## 5. Completed Work Breakdown

### 5.1 M1 Portrayal Architecture Foundation

- [x] dedicated portrayal module boundary between normalized chart data and renderer backends
- [x] stable internal types for portrayal profile ids, display settings, feature portrayal context, lookup keys and results, renderer-independent draw commands, label candidates, and collision boxes
- [x] phase-2 quilt and chart-selection outputs now feed portrayal input without backend coupling
- [x] future `S-100` support has a profile seam through `PortrayalProfileId` and `IPortrayalProfile`

### 5.2 M2 External Portrayal Catalog

- [x] clean-room external `S-57` portrayal catalog at `data/portrayal/s57_catalog.txt`
- [x] catalog stores named colors, area styles, line styles, point styles, text roles, and lookup rules
- [x] catalog loader and validation are implemented and covered by regression tests
- [x] catalog loading is independent from `Qt` and Vulkan
- [x] catalog format includes profile identity so future `S-100` catalogs can sit beside the `S-57` profile

### 5.3 M3 Lookup And Filtering Engine

- [x] lookup selection covers object class, geometry kind, relevant attributes, display category, and enabled settings
- [x] stable fallback behavior exists when no exact rule match is found
- [x] scale-aware filtering such as `SCAMIN` is applied outside renderer code
- [x] feature visibility filtering is separated from renderer logic
- [x] rule evaluation remains deterministic and testable

### 5.4 M4 Derived Rule And Conditional Portrayal Engine

- [x] clean-room derived-rule stage handles portrayal decisions that are not static lookup only
- [x] depth areas use contour-bucket-based styling
- [x] safety contours receive emphasized styling
- [x] buoy, beacon, and light baseline symbol decisions are handled through clean-room lookup data plus derived overrides
- [x] meta and quality-of-data visibility controls are enforced in portrayal
- [x] plain-boundary versus symbolized-boundary choices are supported
- [x] derived rules adjust styling, symbol choice, label priority, and text selection without mutating source data
- [x] derived-rule execution stays profile-oriented so future `S-100` rules can plug into the same seam

### 5.5 M5 Scene Command Expansion

- [x] portrayal expands into renderer-independent filled polygon, stroked line, point symbol, and text-label commands
- [x] explicit z-order and priority sorting are part of the command model
- [x] symbol size, text style, anchors, and label bounds stay outside backend policy code
- [x] backend-specific policy decisions are not embedded in the portrayal command model

### 5.6 M6 Labeling And Declutter Baseline

- [x] core label text extraction covers `SOUNDG`, `OBJNAM`, `NOBJNM`, `LITCHR`, and `VALDCO`
- [x] portrayal exposes font roles instead of backend-specific font objects
- [x] first-pass label placement with collision avoidance is implemented
- [x] important labels win collisions first
- [x] label layout stays deterministic enough for regression testing and exposes collision-box evidence

### 5.7 M7 Vulkan Integration And Demo Usability

- [x] Vulkan renders the portrayal command stream as the default engine path
- [x] software fallback remains available for validation and partial-coverage cases
- [x] demo exposes a minimal control path for core display settings:
  - text
  - soundings
  - lights
  - metadata
  - quality of data
  - simplified points
  - symbolized boundaries
  - display category
  - day / dusk / night color schemes
- [x] startup auto-load with bundled sample data still works under the portrayal pipeline

### 5.8 M8 Validation And Acceptance Evidence

- [x] focused tests cover catalog loading, lookup matching, derived-rule behavior, scale filtering, and label collision behavior
- [x] bundled sample ENC is compared against the bundled reference image and the current remaining gaps are documented
- [x] regression-friendly evidence is written to `build-gdal-verify/tests/validation/`
- [x] Phase 3 object-class coverage is documented

## 6. Acceptance

### 6.1 Functional

- [x] bundled sample ENC renders with chart-like water, land, depth, coastline, and navigational object portrayal instead of placeholder geometry
- [x] key labels appear for supported object classes and do not overlap in obviously broken ways
- [x] zooming changes visibility in a rule-driven way rather than drawing every feature all the time
- [x] display settings change portrayal behavior without renderer-specific branching

### 6.2 Technical

- [x] portrayal rules live outside Vulkan and outside the demo host
- [x] portrayal output is renderer-independent
- [x] the public SDK remains `Qt`-free and backend-type-free
- [x] the portrayal architecture leaves a clean seam for a future `S-100` profile
- [x] Phase 3 does not introduce GPL code or GPL-derived assets

## 7. Validation Snapshot

- verified on `2026-04-11`
- commands:
  - `cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug`
  - `ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure`
- result:
  - `15 / 15` tests passed
- reference-render artifacts:
  - `E:\projects\enc\navscene-sdk\build-gdal-verify\tests\validation\GB4X0000-render.bmp`
  - `E:\projects\enc\navscene-sdk\build-gdal-verify\tests\validation\GB4X0000-render-report.txt`
- supporting docs:
  - [PHASE3.reference-comparison.md](/E:/projects/enc/navscene-sdk/docs/validation/PHASE3.reference-comparison.md)
  - [PHASE3.covered-object-classes.md](/E:/projects/enc/navscene-sdk/docs/reference/PHASE3.covered-object-classes.md)

## 8. Follow-On Candidates

These are not blockers for Phase 3 completion and belong to later phases:

- expand clean-room symbol fidelity beyond current geometric markers
- add pattern fills and richer line styles
- broaden conditional symbology coverage
- extend label templates and placement quality
- introduce `S-100` catalogs and profile implementations
- add raster, satellite, overlay, and 3D scene layers on top of the same scene composition model
