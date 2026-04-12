# `navscene-sdk` Architecture Plan

## 1. Purpose

Build a commercial closed-source geo scene SDK with these constraints:

- no GPL contamination
- no paid commercial dependency requirement
- public API is `C++`
- host UI framework is not part of the SDK boundary
- future support must remain open for `S-100`, overlays, raster basemaps, satellite imagery, 2.5D/3D, Android, and multiple render backends

## 2. Current Architecture Direction

The project is intentionally designed as:

- `S57 first`
- `S100 ready`
- `SDK first`
- `host-framework agnostic`
- `backend pluggable`
- `multi-layer ready`
- `multi-platform ready`

## 3. Hard Boundaries

### 3.1 Licensing

- no OpenCPN code reuse
- no GPL implementation borrowing
- OpenCPN can only be used as behavioral and product reference

### 3.2 Public SDK

- public SDK must not expose `Qt`
- public SDK must not expose Vulkan-native types
- host applications pass native surface handles and size information into the SDK

### 3.3 Renderer

- renderer must not depend on raw `S-57` or future raw `S-100` objects
- renderer consumes internal scene primitives only
- render backend must remain replaceable

## 4. Recommended Layering

### 4.1 `core`

Responsibilities:

- engine lifecycle
- status and logging
- configuration and diagnostics
- viewport-aware chart selection

### 4.2 `data`

Responsibilities:

- chart discovery
- source registration
- format-specific readers such as `S-57`

### 4.3 `model / scene`

Responsibilities:

- normalized feature and scene representation
- renderer-independent chart scene generation
- first-pass quilt / stitched scene composition

### 4.4 `portrayal`

Responsibilities:

- map normalized data into renderable primitives
- keep `S-57` specifics out of renderer implementations

### 4.5 `render`

Responsibilities:

- backend abstraction
- surface attach / resize / present lifecycle
- backend-specific implementations

### 4.6 `platform`

Responsibilities:

- native window/surface description
- platform-specific bridging details

### 4.7 `demo`

Responsibilities:

- host integration examples only
- no leakage back into public SDK boundaries

## 5. Current Implementation Baseline

The current practical baseline after phase 2 is:

- real `GDAL`-backed `S-57` loading
- normalized internal `ChartScene`
- renderer-independent chart selection and first-pass quilt behavior
- renderer-independent portrayal logic
- renderer-independent scene regression outputs
- Qt demo host for native-window integration
- default backend: `Vulkan`
- fallback backend: `Software`
- viewport-aware chart auto-selection by coverage and estimated scale
- coarse-to-fine scene composition for overlapping ENC cells
- improved baseline ENC portrayal colors for commonly visible object classes

Current Win32 Vulkan path is:

1. build renderer-independent projected chart geometry from the internal `ChartScene`
2. render supported primitives through native Vulkan pipelines
3. fall back to `software raster -> BGRA upload -> Vulkan swapchain` when geometry is not yet supported by the native path

This remains a transitional architecture, not the final GPU portrayal design.

## 6. Why The Current Vulkan Path Is Acceptable

It gives us:

- a real Vulkan-backed window presentation path in phase 1
- a native Vulkan primitive-rendering path for supported phase-1 geometry
- minimal change risk through the retained software fallback path
- stable fallback and regression comparison through the `Software` backend
- a clean seam for replacing software raster upload with native GPU primitive rendering later

It does not yet give us:

- GPU-side text and symbol systems
- fully backend-independent render resource abstraction
- full native-GPU coverage for all future chart/overlay geometry types

Those belong to post-phase-1 work.

## 7. Extensibility Requirements

The architecture should continue to preserve room for:

- `S-100` readers and portrayal profiles
- chart auto-selection and layer stack management
- custom vector overlays
- raster basemaps and satellite imagery
- time-aware layers
- offscreen rendering and export
- picking and query systems
- 2D, 2.5D, and 3D scene modes
- Android and other native host environments
- additional backends such as `Metal`, `D3D12`, `OpenGL`, or headless paths

## 8. Near-Term Phase 3 Priorities

1. introduce a dedicated clean-room portrayal subsystem with externalized rules, palettes, symbols, and styles for the first supported `S-57` profile
2. add deterministic lookup, derived-rule evaluation, priority ordering, and scale filtering so portrayal becomes rule-driven instead of geometry-driven
3. add a first-pass text and declutter system that remains renderer-independent and future-compatible with `S-100`
4. continue broadening native GPU coverage while keeping the renderer backend boundary independent from portrayal policy
