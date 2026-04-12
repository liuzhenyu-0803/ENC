# Vulkan Native Render Task

## 1. Goal

Replace the current phase-1 Vulkan path of `software raster upload -> Vulkan swapchain`
with a native Vulkan primitive rendering path, while keeping correctness and maintainability.

## 2. Status Rules

- `- [ ]` not accepted yet
- `- [x]` accepted and verified

## 3. Work Breakdown

### 3.1 Geometry Preparation

- [x] Extract shared `ChartScene -> projected screen-space geometry` builder
- [x] Add automated tests for projection counts, center mapping, and complex-polygon handling
- [x] Add robust triangulation support for polygons with holes

### 3.2 Vulkan Primitive Pipeline

- [x] Add color-only Vulkan shader pipeline for triangles, lines, and points
- [x] Upload projected vertices through GPU buffers
- [x] Render chart primitives without the software upload texture path for supported geometry

### 3.3 Fallback And Transition

- [x] Keep fallback path for unsupported or not-yet-triangulated geometry
- [x] Add diagnostics for which render mode is active

### 3.4 Validation

- [x] Build passes after native triangulation support is added
- [x] `navscene-projected-chart-scene` test passes
- [x] `navscene-polygon-triangulation` test passes
- [x] Win32 surface rendering is verified to use the native Vulkan primitive path for supported geometry
- [x] Win32 surface rendering is verified to fall back to GPU raster upload when native projection is unavailable

## 4. Current Note

Current status on `2026-04-11`:

- projected scene generation has been extracted into a reusable render-independent module
- polygons with holes now triangulate through the internal wrapper around `mapbox-earcut`
- native Vulkan primitive drawing path is wired for triangles, lines, and points
- Vulkan fallback to the existing software-raster upload path remains available for unsupported cases
- frame diagnostics and the Qt demo overlay now expose the active render mode
- native-vs-fallback Vulkan render mode is now covered by an automated Win32 integration test
