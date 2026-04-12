# Phase 4 Area Portrayal Comparison

## 1. Purpose

This document records the validated outcome of Phase 4 area-palette and polygon-boundary portrayal work.

Reference inputs:

- dataset: `data/s57/GB4X0000.000`
- reference image: `data/reference/GB4X0000.png`
- generated render: `build-gdal-verify/tests/validation/GB4X0000-render.bmp`
- generated report: `build-gdal-verify/tests/validation/GB4X0000-render-report.txt`

## 2. What Changed In Phase 4

- the day palette now uses standard ENC-style core area colors for:
  - `LANDA`
  - `CSTLN`
  - `DEPSC`
  - `DEPDW`
  - `DEPMD`
  - `DEPMS`
  - `DEPVS`
  - `CHBRN`
  - `CHGRD`
  - `CHGRF`
  - `CHMGD`
- generic `default_area` solid fill was removed
- `SEAARE` no longer forces a chart-wide semantic sea-area fill
- `SBDARE`, `RESARE`, `ACHARE`, and `PSSARE` no longer rely on incorrect solid fills
- polygon commands can now render outlines even when fill is disabled
- `SLCONS` now has explicit portrayal instead of falling through generic area styling

## 3. Verified Build And Test Pass

Verified on `2026-04-11` with:

```powershell
cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug
ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure
```

Result:

- `15 / 15` tests passed

## 4. Updated Artifact Snapshot

Current report values from `GB4X0000-render-report.txt`:

- `pixel_hash=984971242781703207`
- `area_commands=372`
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
- `COALNE=55`
- `ROADWY=55`
- `TOPMAR=50`
- `UWTROC=50`

## 5. Comparison Against Phase 3 Baseline

Compared with the Phase 3 baseline:

- `area_commands` improved from `571` to `372`
- mean absolute difference improved from `63.9384` to `51.1856`
- RMS difference improved from `75.3784` to `71.5613`

The biggest visible win is that the old gray-blue fallback wash no longer covers large areas of the chart, and shallow-versus-deep water now reads in the expected direction.

## 6. Remaining Gaps

Phase 4 intentionally does not close every remaining difference to the reference image.

Still outstanding:

- full conditional symbology and symbol-atlas fidelity
- pattern fills
- richer industrial / seabed / restriction-area portrayal beyond boundary-only fallback
- fuller label content and placement parity
- higher-fidelity night and dusk portrayal beyond the current compact palette mapping
