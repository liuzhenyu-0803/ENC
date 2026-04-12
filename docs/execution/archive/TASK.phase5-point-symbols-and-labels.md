# `navscene-sdk` Phase 5 Task

Status: completed

## 1. Phase Goal

Phase 5 focuses on the next most visible portrayal gap after area colors:

- [x] make point symbols render with their intended clean-room shapes instead of collapsing to circles
- [x] add dedicated portrayal coverage for high-frequency point classes that still fall through generic styling
- [x] improve label availability for named point objects without breaking renderer independence

## 2. Scope

### 2.1 In Scope

- [x] honor `circle`, `triangle`, `square`, and `diamond` point symbol kinds in the software raster path
- [x] honor `circle`, `triangle`, `square`, and `diamond` point symbol kinds in SVG export
- [x] expose point symbol kind through the compatibility paint-style helper
- [x] add explicit point portrayal rules for `LNDMRK`, `BUISGL`, `FOGSIG`, `TOPMAR`, `WRECKS`, and `OFSPLF`
- [x] enable labels for named high-value point classes where the current catalog suppresses them
- [x] update tests to lock the new behavior

### 2.2 Out Of Scope

- [x] full `S-52` symbol atlas parity remains out of scope
- [x] attribute-complete topmark and wreck symbol variants remain out of scope
- [x] GPU-native non-circular point symbol rendering remains out of scope

## 3. Work Breakdown

### 3.1 M1 Active Phase Setup

- [x] create the active Phase 5 execution document
- [x] point `docs/README.md` and `AGENTS.md` at the active phase

### 3.2 M2 Point Rendering Fidelity

- [x] render point symbol kinds correctly in software raster output
- [x] render point symbol kinds correctly in SVG export
- [x] keep the portrayal scene command model renderer-independent

### 3.3 M3 Point Lookup Coverage

- [x] add dedicated point styles for major remaining visible point classes
- [x] add matching clean-room lookup rules in the external catalog
- [x] update the embedded fallback catalog to match

### 3.4 M4 Validation

- [x] update portrayal and SVG tests for symbol kinds and new point rules
- [x] build the `build-gdal-verify` configuration successfully
- [x] pass the test suite in `build-gdal-verify`
- [x] regenerate the bundled reference-render outputs

## 4. Acceptance

- [x] buoys, beacons, and other supported point symbols no longer all look like circles in software validation output
- [x] the bundled sample uses dedicated portrayal for key point classes that previously fell through generic styling
- [x] named supported point objects can generate labels through the portrayal pipeline
- [x] the public SDK boundary remains `C++` only and does not expose `Qt` or backend-native types

## 5. Completion Snapshot

Verified on `2026-04-11` with:

```powershell
cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug
ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure
```

Accepted outcomes:

- software-raster and SVG exports now honor non-circular point symbol kinds
- high-frequency point classes gained clean-room dedicated styles for landmarks, buildings, fog signals, platforms, wrecks, topmarks, underwater rocks, pilot points, light vessels, and buoy/beacon subclasses
- named buoys, beacons, and offshore platforms now emit chart-style prefixed labels
- light labels now synthesize more chart-like signatures from `S-57` attributes instead of collapsing to raw numeric `LITCHR` values
- bundled validation artifacts were regenerated under `build-gdal-verify/tests/validation/`
