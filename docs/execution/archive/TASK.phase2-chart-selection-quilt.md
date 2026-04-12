# `navscene-sdk` Phase 2 Task

## 1. Phase Goal

Phase 2 focuses on turning the current phase-1 technical baseline into a usable ENC browsing baseline:

- [x] automatically choose suitable ENC datasets for the current viewport and scale
- [x] compose overlapping ENC cells into a continuous chart view
- [x] improve portrayal beyond the current placeholder-style geometry rendering
- [x] keep the SDK architecture compatible with future `S-100`, overlays, raster basemaps, and additional render backends

## 2. Status Rules

- `- [ ]` not accepted yet
- `- [x]` accepted and verified
- partial work stays `- [ ]` and should be annotated inline

## 3. Scope And Non-Goals

### 3.1 In Scope

- chart auto-selection by coverage and compilation-scale suitability
- first-pass quilt / stitched chart presentation
- cleaner viewport-to-chart behavior during pan / zoom / fit
- portrayal uplift for the most visible ENC object categories
- automated validation for dataset selection and stitched display behavior

### 3.2 Out Of Scope For This Phase

- full `S-52` compliance
- `S-100` feature delivery
- full text and symbol engine
- 2.5D / 3D scene delivery
- Android delivery

## 4. Work Breakdown

### 4.1 M1 Chart Selection

- [x] enrich chart metadata needed for dataset ranking and suitability decisions
- [x] resolve visible ENC datasets from current viewport + scale instead of rendering every loaded dataset
- [x] prefer more detailed cells where appropriate and suppress coarser overlaps
- [x] define deterministic tie-break rules so selection remains testable and stable

### 4.2 M2 Quilt Presentation

- [x] compose selected ENC cells into a continuous scene instead of isolated blocks
- [x] remove obvious empty gaps caused by naive per-dataset presentation
- [x] keep pan / zoom / fit stable while chart selection changes
- [x] preserve renderer-independent scene generation as the boundary for quilt output

### 4.3 M3 Portrayal Uplift

- [x] improve area / line / point portrayal beyond the current MVP placeholder look
- [x] add a clearer style baseline for commonly visible ENC object classes
- [x] keep portrayal logic independent from backend-specific rendering code

### 4.4 M4 Validation

- [x] add automated tests for chart selection by coverage and scale
- [x] add regression validation for stitched / quilt scene output
- [x] verify the Qt demo still supports auto-load, manual file load, and chart-directory load
- [x] verify Vulkan native rendering and fallback paths still behave correctly after quilt integration

## 5. Acceptance

### 5.1 Functional

- [x] demo no longer shows obviously isolated chart blocks for overlapping sample ENC coverage
- [x] zooming in and out changes selected charts in a predictable way
- [x] `Fit Charts` lands on a visually reasonable combined chart extent
- [x] manual file / directory loading still works

### 5.2 Technical

- [x] chart selection logic lives outside renderer backends
- [x] scene composition remains renderer-independent
- [x] phase-2 additions do not expose `Qt` or Vulkan-native types in the public SDK
- [x] architecture remains compatible with future `S-100` and non-Vulkan backends

## 6. Current Note

Phase 2 was completed on `2026-04-11` with this accepted baseline:

- viewport-aware chart selection now runs per frame in renderer-independent core logic
- overlapping ENC cells are composed coarse-to-fine into a first-pass quilt scene
- chart selection uses coverage intersection, estimated display scale, and deterministic tie-break rules
- portrayal now uses a clearer water / land / harbor / contour baseline instead of placeholder colors
- validation now covers chart selection, quilt ordering, Vulkan render modes, engine load paths, and real GDAL-backed sample datasets
