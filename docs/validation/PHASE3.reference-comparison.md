# Phase 3 Reference Comparison

## 1. Purpose

This document records the visual-validation baseline for the completed Phase 3 `S-57` portrayal system.

Reference inputs:

- dataset: `data/s57/GB4X0000.000`
- reference image: `data/reference/GB4X0000.png`
- generated render: `build-gdal-verify/tests/validation/GB4X0000-render.bmp`
- generated report: `build-gdal-verify/tests/validation/GB4X0000-render-report.txt`

## 2. Verified Build And Test Pass

Verified on `2026-04-11` with:

```powershell
cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug
ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure
```

Result:

- `15 / 15` tests passed
- `navscene-s57-reference-render` regenerated the current BMP and report artifacts

## 3. Current Artifact Snapshot

Current report values from `GB4X0000-render-report.txt`:

- `pixel_hash=11510498959221728757`
- `area_commands=571`
- `line_commands=819`
- `point_commands=737`
- `label_candidates=213`

Most visible portrayed classes in the generated sample:

- `DEPARE=395`
- `DEPCNT=198`
- `LIGHTS=151`
- `SBDARE=141`
- `SLCONS=131`
- `LNDELV=79`
- `OBSTRN=56`
- `COALNE=55`
- `ROADWY=55`
- `TOPMAR=50`

## 4. Image Comparison Metrics

The generated BMP and the bundled PNG reference are both `1280 x 1024`.

Direct per-channel comparison against the reference currently measures:

- mean absolute difference: `63.9384`
- RMS difference: `75.3784`

These numbers are not treated as a pass/fail gate in Phase 3. They exist to make later portrayal work measurable.

## 5. What Phase 3 Now Matches Well Enough

- chart background and water/land separation are present instead of placeholder geometry colors
- depth areas are bucketed into multiple color ranges
- safety contours are visually emphasized
- coastlines, contours, lights, obstructions, and core labels render as semantic chart objects
- portrayal output is shared across software raster, SVG export, and Vulkan presentation paths

## 6. Known Remaining Gaps

These are accepted Phase 3 limitations and should be improved in later phases:

- symbol fidelity is still geometric and clean-room, not full `S-52` symbol-atlas fidelity
- pattern fills are not implemented yet
- text content and placement are still a first-pass baseline, not a full hydrographic labeling system
- some rendered fallback classes still use generic line, point, or area styles
- visual similarity to the bundled reference is clearly improved, but not yet close enough for pixel-locked acceptance

## 7. Why This Is Enough For Phase 3

Phase 3 was defined as the first clean-room portrayal-system phase, not as full chart-product parity. The generated output now demonstrates:

- reusable, externalized portrayal data
- deterministic rule evaluation and declutter behavior
- recognizable ENC semantics in the default Vulkan-backed demo
- a measurable validation baseline for later symbol and portrayal refinement
