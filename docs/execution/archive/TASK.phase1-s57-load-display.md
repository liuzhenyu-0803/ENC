# `navscene-sdk` Phase 1 Task

## 1. Phase Goal

Phase 1 only targets one closed loop:

- [x] Program starts successfully
- [x] User can choose local `S-57 ENC` data
- [x] SDK can load and manage chart datasets
- [x] Main view can display loaded ENC content
- [x] View supports basic `pan / zoom / fit`
- [x] Default render backend is `Vulkan`

## 2. Status Rules

- `- [ ]` not accepted yet
- `- [x]` accepted and verified
- Partial work stays `- [ ]` and should be annotated inline

Current blocker register: [BLOCKERS.phase1.md](/E:/projects/enc/navscene-sdk/docs/execution/BLOCKERS.phase1.md)

## 3. Current Snapshot

Date: `2026-04-11`

- [x] SDK public API keeps `Qt` out of the interface surface
- [x] Qt is only used in the demo host under `demo/qt`
- [x] Chart directory scan and direct `.000` registration both work
- [x] Real `GDAL`-backed `S-57` loading is integrated and verified against local samples
- [x] Internal normalized chart scene pipeline is in place
- [x] `ChartScene` portrayal logic is renderer-agnostic
- [x] Renderer-independent regression baselines exist through `scene signature` and `SVG export`
- [x] Windows demo can attach a native window handle and render charts
- [x] Windows demo can auto-load a bundled ENC sample from `data/`
- [x] `Auto` now resolves to `Vulkan` when Vulkan is enabled at build time
- [x] `Software` remains as the stable fallback and regression baseline

## 4. Acceptance

### 4.1 Functional

- [x] Demo starts and opens a main window
- [x] Demo can choose a single `.000` file
- [x] Demo can choose a chart directory
- [x] Demo can auto-load a bundled ENC sample on startup
- [x] SDK recognizes at least one real ENC dataset
- [x] Main view renders chart content after load
- [x] `Fit Charts` works
- [x] Drag-to-pan works
- [x] Mouse-wheel zoom works
- [x] Resize still renders correctly

### 4.2 Technical

- [x] Public SDK does not expose `Qt` types
- [x] `S-57` loading is behind an internal adapter boundary
- [x] `GDAL` stays internal to the SDK
- [x] Renderer backend is replaceable
- [x] `Vulkan` is the phase-1 default backend
- [x] Architecture still leaves room for `S-100`, overlays, raster basemaps, 2.5D/3D, Android, and other render backends

### 4.3 Quality

- [x] Real sample-based automated validation exists
- [x] Error paths are covered by automated tests
- [x] Renderer-independent regression outputs exist
- [x] Demo launch was rechecked after switching phase-1 default to `Vulkan`

## 5. Work Breakdown

### 5.1 M1 Project Skeleton

- [x] Top-level `CMakeLists.txt`
- [x] `include/`, `src/`, `demo/`, `tests/`, `docs/`
- [x] Minimal `CreateEngine` lifecycle
- [x] Logging, status codes, diagnostics baseline

### 5.2 M2 Host Integration And Rendering Loop

- [x] Qt demo creates a native host window
- [x] Host window is bridged through `NativeSurfaceDesc`
- [x] `AttachSurface / RenderFrame / Resize` path works
- [x] Replaceable renderer backend abstraction is in place
- [x] Phase-1 default backend is `Vulkan`
- [x] `Software` backend remains available as fallback

### 5.3 M3 `S-57` Loading

#### 5.3.1 Demo-Side Selection

- [x] Open single `.000`
- [x] Open chart directory
- [x] Pass the result into the SDK

#### 5.3.2 SDK Registration And Scan

- [x] Register chart directory
- [x] Register direct dataset source
- [x] Scan `.000` datasets and build catalog snapshot
- [x] Merge scanned datasets with direct sources

#### 5.3.3 GDAL Integration

- [x] Integrate `GDAL/OGR` `S-57` reader path
- [x] Verify local `OSGeo4W` environment integration
- [x] Read dataset coverage and base metadata
- [x] Validate against real sample charts in `E:\projects\enc\enc`

#### 5.3.4 Data Normalization

- [x] Normalize to internal `Point / Line / Area + OBJL`
- [x] Cover basic geometry types
- [x] Feed stable input into portrayal and scene generation

### 5.4 M4 Display Closed Loop

#### 5.4.1 Portrayal MVP

- [x] Basic polygon fill
- [x] Basic line stroke
- [x] Basic point placeholder rendering

#### 5.4.2 Scene Generation

- [x] Convert normalized data to render primitives
- [x] Build `ChartScene`
- [x] Keep renderer independent from `GDAL`

#### 5.4.3 Window Rendering

- [x] `Software` raster path exists and stays reusable
- [x] `Vulkan` backend presents the chart into a Win32 window
- [x] Phase-1 default backend is `Vulkan`, with native primitive rendering preferred and software-raster upload retained as fallback

### 5.5 M5 Interaction And Reliability

- [x] Pan updates viewport
- [x] Zoom updates viewport
- [x] `FitToData()` restores chart-fit view
- [x] Invalid input and empty-directory errors are handled
- [x] Unsupported-backend fallback behavior is covered

## 6. Latest Verification Record

Date: `2026-04-11`

Build:

```powershell
$env:OSGEO4W_ROOT='E:\OSGeo4W'
$env:GDAL_DIR='E:\OSGeo4W\lib\cmake\gdal'
$env:GDAL_DATA='E:\OSGeo4W\apps\gdal\share\gdal'
$env:PROJ_LIB='E:\OSGeo4W\share\proj'
$env:NAVSCENE_S57_SAMPLE_ROOT='E:\projects\enc\enc'
$env:PATH='C:\VulkanSDK\1.4.341.1\Bin;E:\OSGeo4W\bin;' + $env:PATH
cmake -S E:\projects\enc\navscene-sdk -B E:\projects\enc\navscene-sdk\build-gdal-verify -DNAVSCENE_ENABLE_VULKAN=ON -DNAVSCENE_ENABLE_GDAL=ON -DGDAL_DIR=E:\OSGeo4W\lib\cmake\gdal
cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug
```

Tests:

```powershell
ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure
```

Result:

- [x] `navscene-smoke`
- [x] `navscene-error-paths`
- [x] `navscene-portrayal-mvp`
- [x] `navscene-scene-signature`
- [x] `navscene-scene-svg`
- [x] `navscene-projected-chart-scene`
- [x] `navscene-polygon-triangulation`
- [x] `navscene-layer-composition`
- [x] `navscene-vulkan-win32-render-modes`
- [x] `navscene-s57-samples`
- [x] Qt demo process launch verified after Vulkan-default switch

## 7. Phase Conclusion

- [x] Phase 1 target is complete
- [x] Phase 1 default backend is now `Vulkan`
- [x] Next stage can build on the current `S-57 first / S-100 ready / backend pluggable` baseline
