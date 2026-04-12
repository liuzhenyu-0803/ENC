# `navscene-sdk` Phase 6 Task

Status: completed

## 1. Phase Goal

Phase 6 focuses on the biggest visible fidelity gap remaining after point-symbol cleanup:

- [x] improve conditional portrayal and labeling for high-visibility line and area classes
- [x] make major traffic, route, industrial, and infrastructure features read closer to ENC expectations
- [x] keep the portrayal system clean-room, data-driven, and ready for future `S-100` expansion

## 2. Scope

### 2.1 In Scope

- [x] add dedicated clean-room portrayal coverage for prominent remaining line and area classes in the bundled sample
- [x] improve label extraction for named and measured line or area features where the current output is still overly sparse
- [x] refine chart-like styling for major route, traffic, industrial, and infrastructure classes without introducing GPL-derived logic
- [x] keep external and embedded portrayal catalogs aligned
- [x] extend regression tests and validation artifacts for the new portrayal coverage

### 2.2 Out Of Scope

- [ ] full `S-52` procedural rule parity
- [ ] symbol-atlas bitmap reproduction
- [ ] 3D portrayal, terrain, or raster-basemap composition

## 3. Work Breakdown

### 3.1 M1 Gap Review

- [x] identify the highest-impact remaining differences in the bundled `GB4X0000` comparison
- [x] capture the target classes and behaviors for this phase in a compact validation-oriented list

### 3.2 M2 Portrayal Expansion

- [x] implement dedicated clean-room portrayal for the selected line and area classes
- [x] add higher-value label synthesis where raw attribute output is still misleading or too sparse
- [x] preserve renderer independence in the portrayal scene command model

### 3.3 M3 Verification

- [x] update regression tests for the new catalog and portrayal behavior
- [x] build the `build-gdal-verify` configuration successfully
- [x] pass the test suite in `build-gdal-verify`
- [x] regenerate validation artifacts and record the visual delta in `docs/validation/`

## 4. Acceptance

- [x] the next most visible classes in the bundled sample no longer rely on misleading generic line or area portrayal
- [x] label output for supported named or measured features becomes more chart-like and less raw-data-like
- [x] the clean-room external portrayal catalog remains the main source of styling truth
- [x] the public SDK boundary stays `C++` only and backend-agnostic

## 5. Current Gap Snapshot

Reviewed against the current bundled comparison on `2026-04-11`.

Highest-impact remaining targets for Phase 6:

- `RECTRC`
  - route and traffic-separation geometry is still too generic compared with the reference chart
- `ROADWY` and `RAILWY`
  - prominent infrastructure lines remain under-portrayed and visually lightweight
- `PIPSOL`
  - pipeline portrayal is still missing class-specific visual treatment
- `SLCONS`, `PRDARE`, and related industrial or production areas
  - line and area semantics still read flatter than the reference image
- named or measured line and area annotations
  - several remaining labels still look too sparse compared with the reference chart, especially where route, fairway, or industrial naming should help scene readability

## 6. Completion Notes

Validated on `2026-04-12`.

Implemented in this phase:

- stroke-pattern support in the clean-room portrayal scene model and catalog parsing
- software-raster dashed and dotted line rendering on Win32
- SVG export support for `stroke-dasharray`
- native-Vulkan fallback when the portrayed scene requires patterned-line fidelity
- dedicated portrayal coverage for `RECTRC`, `ROADWY`, `RAILWY`, `PIPSOL`, and refined area treatment for `PRDARE`
- higher-value label synthesis for `RECTRC` orientation and named `PRDARE` production areas
- default display behavior now keeps `symbolized_boundaries` enabled so standard `S-52` line styles are not flattened by default

Verification snapshot:

- `cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug`
- `ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure`
- result: `15 / 15` tests passed
- validation artifact hash:
  - `pixel_hash=5802370713414916964`
