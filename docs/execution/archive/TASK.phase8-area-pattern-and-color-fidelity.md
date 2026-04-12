# `navscene-sdk` Phase 8 Task

Status: archived

Archive note:

- This task is archived as a completed investigation and partial-improvement batch.
- Remaining work is superseded by the new active execution document for full clean-room `S-57 -> S-52` rendering parity.

## 1. Phase Goal

Phase 8 focuses on the next rendering gap after the accepted Phase 7 baseline:

- [ ] reduce the remaining visible gap in area portrayal, especially region color fidelity and missing `S-52` pattern fills
- [ ] correct area classes whose current clean-room portrayal semantics still flatten or wash out the reference scene
- [ ] keep all additions backend-agnostic so Vulkan, software validation, and future backends still consume the same portrayal scene

## 2. Scope

### 2.1 In Scope

- [ ] correct incorrect or over-strong area fills where the standard portrayal is actually boundary-plus-pattern only
- [ ] add the highest-impact missing patterned area classes present in the bundled sample
- [ ] improve procedural pattern glyphs toward standard `S-52` symbol-table geometry for the supported subset
- [ ] refresh validation artifacts and record the new comparison snapshot

### 2.2 Out Of Scope

- [ ] full bitmap-symbol atlas parity
- [ ] pixel-perfect reproduction of every OpenCPN chart decoration
- [ ] `S-100` portrayal implementation

## 3. Work Breakdown

### 3.1 M1 Area Semantics Corrections

- [x] remove or reduce area fills that currently mute underlying water or land colors incorrectly
- [x] align `AIRARE`, `M_QUAL`, `UNSARE`, and similar classes more closely with standard area procedure intent

### 3.2 M2 Missing Area Pattern Coverage

- [x] add clean-room support for the highest-value missing patterned area classes in the bundled sample, especially `VEGATN` and no-data / incompletely-surveyed overlays
- [x] keep patterned areas represented inside the portrayal scene rather than hidden in backend-only code

### 3.3 M3 Pattern Glyph Fidelity

- [ ] strengthen and refine procedural glyph geometry for `AIRARE02`, `DQUAL*`, `RCKLDG01`, `CBLARE51`, and newly added vegetation / no-data patterns. Clean-room vectors are improved, but chart-scale parity is still incomplete.
- [ ] ensure patterned areas remain visible enough at the validation viewport scale. More tuning is still needed against the bundled reference.

### 3.4 M4 Verification

- [x] extend regression coverage for the corrected area semantics and new patterned classes
- [x] rebuild the `build-gdal-verify` configuration successfully
- [x] pass the targeted test suite in `build-gdal-verify`
- [x] regenerate the bundled reference-render artifacts and capture the updated snapshot

## 4. Current Progress Notes

- [x] OpenCPN / `S-52` table references for `AIRARE02`, `DQUAL*`, `RCKLDG01`, `CBLARE51`, `NODATA03`, `PRTSUR01`, and `VEGATN03/04` were reviewed as behavior references
- [x] the bundled sample was confirmed to contain `VEGATN` and `UNSARE` features that are not yet fully portrayed
- [x] area semantics were corrected so `M_QUAL` no longer washes underlying chart colors, `AIRARE` no longer forces land fill when `CONVIS` is absent, and `UNSARE` now uses `NODTA` + no-data pattern treatment
- [x] clean-room patterned overlays were added for `VEGATN`, `UNSARE`, and incompletely surveyed `DEPARE` / `DRGARE` fallback cases
- [x] validation projection now applies a local longitude scale factor, reducing horizontal distortion in the bundled sample comparison
- [x] dataset loading now derives generic default-view hints from `M_COVR CATCOV=1` and compilation scale so single-chart runtime fits are chart-aware instead of raw-bbox-only
- [x] bundled reference validation now uses a dedicated reference viewport fixture for `GB4X0000`, keeping runtime view hints generic while aligning the regression render to the checked-in reference image
- [x] the Qt demo now applies the same bundled-reference viewport fixture when auto-loading `GB4X0000`, so the startup view matches the validation framing more closely
- [x] regression coverage now includes `AIRARE` conspicuity behavior, `UNSARE`, and `VEGATN`
- [x] latest verified validation snapshot:
  - `viewport_center=60.99,-32.4551`
  - `viewport_display_scale=52000`
  - `pixel_hash=7588793774110652591`
  - `area_commands=454`
  - `area_overlays=44`
  - sampled per-channel mean absolute difference: `31.2656`
  - sampled per-channel RMS difference: `49.7289`
- [ ] implementation and validation updates are still in progress for the remaining pattern-density and chart-scale parity gap
