# Phase 5 Point Symbols And Labels

## 1. Purpose

This document records the validated outcome of Phase 5 point-symbol and label-fidelity work.

Reference inputs:

- dataset: `data/s57/GB4X0000.000`
- reference image: `data/reference/GB4X0000.png`
- generated render: `build-gdal-verify/tests/validation/GB4X0000-render.bmp`
- generated report: `build-gdal-verify/tests/validation/GB4X0000-render-report.txt`

## 2. What Changed In Phase 5

- software-raster and SVG output now honor the clean-room point symbol kinds instead of collapsing supported points into circles
- the portrayal catalog now includes dedicated point styles for:
  - `LNDMRK`
  - `BUISGL`
  - `FOGSIG`
  - `OFSPLF`
  - `TOPMAR`
  - `WRECKS`
  - `UWTROC`
  - `PILPNT`
  - `LITVES`
  - beacon and buoy subclasses such as `BCNCAR`, `BCNSAW`, `BCNSPP`, `BOYCAR`, `BOYSAW`, and `BOYSPP`
- named beacons, buoys, and offshore platforms now emit chart-style prefixed labels such as `bn`, `by`, and `Prod`
- light labels now synthesize a cleaner signature from `S-57` attributes like `LITCHR`, `SIGGRP`, `COLOUR`, `SIGPER`, and `VALNMR`

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

- `pixel_hash=12962264340343165991`
- `area_commands=372`
- `line_commands=819`
- `point_commands=737`
- `label_candidates=229`

Most visible portrayed classes in the generated sample remain:

- `DEPARE=395`
- `DEPCNT=198`
- `LIGHTS=151`
- `SBDARE=141`
- `SLCONS=131`
- `LNDELV=79`
- `TOPMAR=50`
- `UWTROC=50`
- `BOYSPP=23`
- `BOYLAT=22`

## 5. Visual Outcome

Compared with the Phase 4 output:

- light labels are no longer dominated by raw numeric light-character values
- named navigation aids now read more like chart labels instead of plain object names
- buoy and beacon subclasses are easier to distinguish in validation output because they no longer all share the exact same point styling
- underwater rocks, pilot points, and light vessels no longer fall straight through the default point style

Phase 5 does not attempt full `S-52` symbol-atlas parity. The remaining largest differences are now concentrated in richer conditional line and area portrayal, denser annotations, and more exact symbol variants.
