# `navscene-sdk` Decisions

## D-001 Clean-room boundary

- Status: `Accepted`
- Decision:
  - no GPL code enters this repository
  - OpenCPN may be used only as black-box product reference
- Impact:
  - implementation, naming, and module boundaries must remain original

## D-002 Product positioning

- Status: `Accepted`
- Decision:
  - this project is a closed-source geo scene SDK, not a one-off ENC viewer
- Impact:
  - architecture must remain extensible for overlays, raster basemaps, satellite imagery, `S-100`, and 2.5D/3D

## D-003 Public SDK boundary

- Status: `Accepted`
- Decision:
  - public SDK exposes `C++` interfaces only
  - `Qt` is demo-only and must not appear in the public API
  - host applications pass native surface/window handles into the SDK

## D-004 Render backend strategy

- Status: `Accepted`
- Decision:
  - renderer backends are replaceable
  - the architecture is not `Vulkan only`
  - future `Metal / D3D12 / OpenGL / headless` backends remain valid extensions

## D-005 Phase-1 scope

- Status: `Accepted`
- Decision:
  - phase 1 focuses on `S-57` load, display, and basic interaction only
  - full `S-52`, `S-100`, advanced overlays, and 3D content stay out of the delivery scope

## D-006 Data normalization boundary

- Status: `Accepted`
- Decision:
  - phase-1 `S-57` content is normalized to internal `Point / Line / Area + OBJL`
  - renderer code does not consume raw `GDAL` or raw `S-57` objects directly

## D-007 Phase-1 default backend

- Status: `Accepted`
- Decision:
  - phase-1 default `Auto` backend resolves to `Vulkan` when Vulkan support is compiled in
  - `Software` stays implemented as the stable fallback and regression baseline
  - current Win32 Vulkan path presents a software-rasterized chart image through a Vulkan swapchain
- Rationale:
  - this satisfies the phase-1 requirement that Vulkan is the default render engine
  - it preserves the existing portrayal and regression pipeline
  - it keeps the backend boundary ready for future native GPU primitive rendering

## D-008 SDK design principle for future expansion

- Status: `Accepted`
- Decision:
  - the architecture must stay `S57 first`, `S100 ready`, `host-framework agnostic`, `backend pluggable`, and `multi-platform ready`
- Impact:
  - source, portrayal, scene, render backend, and platform surface concerns remain separated

## D-009 Phase-2 chart selection boundary

- Status: `Accepted`
- Decision:
  - viewport-aware chart selection and quilt decisions live outside renderer backends
  - render backends consume only the renderer-independent selected `ChartScene`
  - coarse-to-fine draw ordering is decided before backend rendering
- Impact:
  - `Vulkan`, `Software`, and future backends share one chart-selection policy
  - future `S-100` and mixed-source quilt strategies have a clean extension seam

## D-010 Raw scene preservation vs portrayal filtering

- Status: `Accepted`
- Decision:
  - `ChartScene` remains a raw normalized scene and should preserve metadata and other source feature classes
  - visibility filtering such as soundings, lights, metadata, data-quality, and `SCAMIN` belongs to the portrayal stage, not chart-scene building
  - label generation and declutter are portrayal-driven concerns even when final screen placement happens in a later projection/layout step
- Impact:
  - source fidelity, feature picking, debugging, and regression baselines no longer depend on early scene filtering
  - multiple portrayal profiles such as future `S-100` can consume the same raw scene without rebuilding loader-side policy
