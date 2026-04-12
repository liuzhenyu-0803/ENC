# navscene-sdk

Closed-source geo scene SDK focused on a clean-room `S-57` first architecture with future-ready paths for `S-100`, overlays, raster layers, and multiple render backends.

Current verified baseline:

- Qt demo host can open a single `.000` file or a chart directory
- Qt demo auto-loads the first bundled ENC sample from `data/` on startup
- SDK can discover, register, and load `S-57` datasets
- `GDAL`-backed `S-57` loading is integrated and verified against real local samples
- raw normalized `ChartScene` preservation is separated from portrayal-time visibility filtering
- internal chart portrayal and scene generation are renderer-agnostic
- default render backend is `Vulkan`
- `Software` backend remains available as fallback and regression baseline
- baseline labels now render through the portrayal pipeline with deterministic first-pass declutter
- external clean-room portrayal catalog is loaded from `data/portrayal/s57_catalog.txt`
- demo display controls cover text, soundings, lights, metadata, quality, display category, symbolization, and day/dusk/night color schemes
- Qt demo supports `pan / zoom / fit`
- phase-2 chart selection and quilt baseline is in place
- phase-3 `S-57` portrayal baseline is complete and archived in the execution docs

Latest completed work:

- clean-room rule-driven `S-57` portrayal pipeline
- reference render validation against bundled `GB4X0000` data and image assets
- `S-100`-ready portrayal profile seam without exposing host-framework or backend-native SDK types

## Repository Layout

- [AGENTS.md](/E:/projects/enc/navscene-sdk/AGENTS.md)
- [docs/README.md](/E:/projects/enc/navscene-sdk/docs/README.md)
- [include/navscene/navscene.h](/E:/projects/enc/navscene-sdk/include/navscene/navscene.h)
- `src/`
- `demo/`
- `tests/`
- `third_party/`
- `data/`

## Build Options

- `NAVSCENE_BUILD_QT_DEMO`
  - build the Qt host demo
- `NAVSCENE_ENABLE_VULKAN`
  - enable Vulkan backend detection and build
- `NAVSCENE_ENABLE_GDAL`
  - enable GDAL-backed `S-57` loading

If a requested dependency is not found, the build falls back to the best available implemented path.

## Verified Local Environment

- `OSGEO4W_ROOT=E:\OSGeo4W`
- `GDAL_DIR=E:\OSGeo4W\lib\cmake\gdal`
- `GDAL_DATA=E:\OSGeo4W\apps\gdal\share\gdal`
- `PROJ_LIB=E:\OSGeo4W\share\proj`
- `NAVSCENE_S57_SAMPLE_ROOT=E:\projects\enc\enc`
- prepend `C:\VulkanSDK\1.4.341.1\Bin`
- prepend `E:\OSGeo4W\bin`

## Verified Build

```powershell
$env:OSGEO4W_ROOT='E:\OSGeo4W'
$env:GDAL_DIR='E:\OSGeo4W\lib\cmake\gdal'
$env:GDAL_DATA='E:\OSGeo4W\apps\gdal\share\gdal'
$env:PROJ_LIB='E:\OSGeo4W\share\proj'
$env:NAVSCENE_S57_SAMPLE_ROOT='E:\projects\enc\enc'
$env:PATH='C:\VulkanSDK\1.4.341.1\Bin;E:\OSGeo4W\bin;' + $env:PATH
cmake -S E:\projects\enc\navscene-sdk -B E:\projects\enc\navscene-sdk\build-gdal-verify -DNAVSCENE_ENABLE_VULKAN=ON -DNAVSCENE_ENABLE_GDAL=ON -DGDAL_DIR=E:\OSGeo4W\lib\cmake\gdal
cmake --build E:\projects\enc\navscene-sdk\build-gdal-verify --config Debug
ctest --test-dir E:\projects\enc\navscene-sdk\build-gdal-verify -C Debug --output-on-failure
```

Verified test targets:

- `All 15` current `CTest` targets passed on `2026-04-11`
- detailed validation evidence:
  - [docs/validation/PHASE3.reference-comparison.md](/E:/projects/enc/navscene-sdk/docs/validation/PHASE3.reference-comparison.md)
  - [docs/reference/PHASE3.covered-object-classes.md](/E:/projects/enc/navscene-sdk/docs/reference/PHASE3.covered-object-classes.md)
