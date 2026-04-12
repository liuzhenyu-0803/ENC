# Phase 8 Validation Snapshot

## 1. Purpose

This document records the current verification snapshot for the active Phase 8 area-pattern and color-fidelity work.

Reference inputs:

- dataset: `data/s57/GB4X0000.000`
- reference image: `data/reference/GB4X0000.png`
- generated render: `validation/GB4X0000-render.bmp`
- generated report: `validation/GB4X0000-render-report.txt`

## 2. Verified Build And Test Pass

Verified on `2026-04-12` with:

```powershell
cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug --target navscene navscene-s57-reference-render navscene-s57-feature-dump navscene-demo-qt
ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure -R "portrayal-mvp|portrayal-catalog|label-layout"
$env:PATH='E:\OSGeo4W\bin;' + $env:PATH
.\build-gdal-verify\tests\Debug\navscene-s57-reference-render.exe
```

Result:

- targeted tests passed
- the bundled reference render was regenerated successfully

## 3. Current Artifact Snapshot

Current report values from `GB4X0000-render-report.txt`:

- `default_view_center=61.0461,-32.4772`
- `default_display_scale=52000`
- `viewport_center=60.99,-32.4551`
- `viewport_display_scale=52000`
- `pixel_hash=7588793774110652591`
- `area_commands=454`
- `area_overlays=44`
- `line_commands=819`
- `point_commands=737`
- `label_candidates=0`

## 4. Current Verified Improvements

- `M_QUAL` now renders as boundary-plus-pattern instead of tinting underlying areas
- `AIRARE` now respects conspicuity and no longer forces land fill when the standard rule is pattern-only
- `UNSARE` now uses `NODTA` semantics plus a no-data area pattern
- `VEGATN` is now represented in the portrayal scene with clean-room vegetation overlays
- validation rendering now applies a local longitude scale factor to reduce horizontal distortion in the sample
- single-chart runtime view fitting now derives generic default-view hints from `M_COVR` plus compilation scale instead of relying only on raw dataset extents
- the bundled `GB4X0000` comparison now uses a dedicated reference viewport fixture so the regression image compares the same harbor framing as the checked-in PNG
- the Qt demo applies that same fixture when it auto-loads the bundled reference sample, keeping the interactive startup view aligned with the validation artifact

## 5. Sampled Comparison Metrics

Sampled on an `8 x 8` grid across the full `1280 x 1024` image:

- per-channel mean absolute difference: `31.2656`
- per-channel RMS difference: `49.7289`

These metrics are still not acceptance gates, but they confirm the active Phase 8 batch changed the rendered output measurably.

## 6. Remaining Gap

The largest remaining visual differences are still concentrated in:

- chart-scale density and exact geometry of repeated area patterns
- richer line / point / annotation fidelity in high-detail comparison regions
- residual center / crop sensitivity inside the validation harbor view, where small viewport shifts still move sample pixels between land, intertidal, and water polygons
- remaining portrayal differences inside the aligned viewport, especially exact shallow-water / intertidal appearance and object-detail richness
