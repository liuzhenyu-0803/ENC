# Phase 1 Blockers

## 1. Status

All phase-1 blockers are currently closed.

Status vocabulary:

- `Open`: actively blocks the phase goal
- `Mitigated`: workaround exists but the blocker is not fully closed
- `Closed`: no longer blocks phase delivery

## 2. Historical Blockers

### B-003 Native window integration and real renderer backend

- Status: `Closed`
- Outcome:
  - Qt demo attaches a native window through `NativeSurfaceDesc`
  - `AttachSurface / Resize / RenderFrame` is verified
  - phase-1 default backend is now `Vulkan`
  - `Software` backend remains available as fallback and regression baseline

### B-004 Real `S-57` display baseline

- Status: `Closed`
- Outcome:
  - real ENC samples under `E:\projects\enc\enc` are part of validation
  - normalized `ChartScene` output is stable
  - renderer-independent `scene signature` and `SVG` regression paths exist

### B-005 Phase-1 portrayal scope freeze

- Status: `Closed`
- Outcome:
  - scope is intentionally limited to `Point / Line / Area + OBJL`
  - phase 1 does not attempt full `S-52`
  - portrayal MVP is separated from window presentation details

### B-006 Local GDAL toolchain and sample verification

- Status: `Closed`
- Outcome:
  - local `OSGeo4W GDAL 3.12.3` integration is verified
  - real-sample loading is covered in automated tests

### B-007 Productized error-path handling

- Status: `Closed`
- Outcome:
  - invalid source, empty directory, missing surface attach, and fallback behavior are covered
  - demo now exposes clear failure feedback instead of failing silently

## 3. Current Focus

Phase 1 is no longer blocked. The next engineering focus should move to post-phase-1 capabilities:

1. native GPU portrayal instead of software-raster upload
2. chart selection and layer stack
3. `S-100`-ready expansion of source and portrayal pipelines
