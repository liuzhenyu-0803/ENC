# `navscene-sdk` Phase 4 Task

Status: archived and completed on `2026-04-11`

## 1. Phase Goal

Phase 4 focused on closing the largest remaining visual gap in the current `S-57` clean-room portrayal:

- [x] make area colors materially closer to standard ENC portrayal for the bundled sample
- [x] remove incorrect generic area fills that wash out the chart
- [x] support boundary-only area portrayal so restricted and caution areas no longer require false solid fills
- [x] keep the implementation renderer-independent and compatible with future `S-100` portrayal growth

## 2. Scope

### 2.1 In Scope

- [x] standardize the core day-scheme area palette around `LANDA`, `CSTLN`, `DEPSC`, `DEPDW`, `DEPMD`, `DEPMS`, `DEPVS`, `CHBRN`, `CHGRD`, `CHGRF`, and `CHMGD`
- [x] correct the effective depth-area palette ordering for `DEPARE`, `DRGARE`, `UNSARE`, `TIDEWY`, and `LAKARE`
- [x] remove the current solid-fill fallback for unknown areas
- [x] add explicit portrayal behavior for `SEAARE`, `SBDARE`, `SLCONS`, `RESARE`, `ACHARE`, and `PSSARE`
- [x] let software raster, SVG export, and projected/Vulkan-native geometry render polygon outlines even when fill is disabled
- [x] update tests to lock the new behavior
- [x] regenerate validation artifacts for the bundled `GB4X0000` sample

### 2.2 Out Of Scope

- [x] full `S-52` conditional symbology coverage
- [x] pattern fills and symbol atlas parity
- [x] full text-template parity with OpenCPN or ECDIS products
- [x] `S-100` implementation work

## 3. Work Breakdown

### 3.1 M1 Active Phase Setup

- [x] create the active Phase 4 execution document
- [x] point `docs/README.md` and `AGENTS.md` at the active phase

### 3.2 M2 Palette And Lookup Cleanup

- [x] update the external `S-57` portrayal catalog with corrected core area colors
- [x] update the embedded fallback catalog to match the external catalog
- [x] replace generic `default_area` solid fill with a non-polluting fallback
- [x] add explicit lookup coverage for the most visible remaining area classes

### 3.3 M3 Area Rendering Capability

- [x] support stroke-only polygon portrayal in the software raster path
- [x] support stroke-only polygon portrayal in SVG export
- [x] support stroke-only polygon portrayal in projected/Vulkan-native geometry preparation

### 3.4 M4 Validation

- [x] update portrayal regression tests for the new palette and fallback behavior
- [x] build the `build-gdal-verify` configuration successfully
- [x] pass the test suite in `build-gdal-verify`
- [x] regenerate the bundled reference-render outputs
- [x] capture updated visual-comparison notes in `docs/validation/`

## 4. Acceptance

- [x] the bundled sample no longer shows large incorrect gray-blue fallback area washes
- [x] deep water is visually lighter than shallow water in the default day palette
- [x] restricted / caution areas can render as boundary-only features
- [x] the Vulkan path and software validation path consume the same portrayal results
- [x] the public SDK boundary remains `C++` only and does not expose `Qt` or backend-native types

## 5. Validation Snapshot

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
  - [PHASE4.area-portrayal-comparison.md](/E:/projects/enc/navscene-sdk/docs/validation/PHASE4.area-portrayal-comparison.md)

## 6. Follow-On Candidates

These are not blockers for Phase 4 completion and belong to later phases:

- expand conditional area portrayal beyond the current compact rule set
- add pattern fills and richer restriction / seabed semantics
- raise point, line, and label fidelity toward fuller `S-52` behavior
- extend dusk and night portrayal beyond the current compact palette mapping
