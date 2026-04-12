# Phase 6 Conditional Portrayal And Line Fidelity

## 1. Purpose

This document records the validated outcome of Phase 6 conditional line and area portrayal work.

Reference inputs:

- dataset: `data/s57/GB4X0000.000`
- reference image: `data/reference/GB4X0000.png`
- generated render: `build-gdal-verify/tests/validation/GB4X0000-render.bmp`
- generated report: `build-gdal-verify/tests/validation/GB4X0000-render-report.txt`

## 2. What Changed In Phase 6

- the portrayal scene model and both external and embedded `S-57` catalogs now support explicit stroke patterns
- Win32 software rasterization now renders dashed and dotted strokes instead of collapsing every line to a solid pen
- SVG export now emits `stroke-dasharray` for patterned chart strokes
- the Vulkan native-geometry path now falls back automatically when the scene requires patterned-line fidelity
- dedicated clean-room portrayal was added for:
  - `RECTRC`
  - `ROADWY`
  - `RAILWY`
  - `PIPSOL`
  - `PRDARE`
- `RECTRC` can now synthesize chart-like orientation labels such as `045 deg`
- named `PRDARE` areas now generate production-style labels prefixed with `Prod`
- default display behavior now keeps `symbolized_boundaries` enabled so standard chart boundaries are not flattened by default

## 3. Verified Build And Test Pass

Verified on `2026-04-12` with:

```powershell
cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug
ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure
```

Result:

- `15 / 15` tests passed

## 4. Updated Artifact Snapshot

Current report values from `GB4X0000-render-report.txt`:

- `pixel_hash=5802370713414916964`
- `area_commands=372`
- `line_commands=819`
- `point_commands=737`
- `label_candidates=248`

Most visible portrayed classes in the generated sample remain:

- `DEPARE=395`
- `DEPCNT=198`
- `LIGHTS=151`
- `SBDARE=141`
- `SLCONS=131`
- `LNDELV=79`
- `ROADWY=55`
- `PIPSOL=29`
- `RECTRC=19`
- `RAILWY=18`

## 5. Visual Outcome

Compared with the Phase 5 output:

- route, pipeline, and restriction-style boundaries no longer have to collapse into solid strokes
- the rendered sample now produces a new validation hash, confirming a visible portrayal change in the bundled reference output
- high-value line and area annotations are less raw and more chart-like where `RECTRC` orientation or production naming is available
- the renderer boundary remains backend-agnostic because patterned-line fidelity is expressed in the portrayal scene rather than hardcoded into one backend

Phase 6 still does not claim full `S-52` procedural or symbol-atlas parity. The largest remaining gaps are richer procedure-driven line symbols, more exact area-symbol behavior, palette indirection across all color schemes, and higher-fidelity symbol composition beyond simple clean-room geometry and stroke styles.
