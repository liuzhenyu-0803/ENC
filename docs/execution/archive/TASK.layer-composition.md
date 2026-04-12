# Layer Composition Task

## 1. Goal

Turn the existing `ILayerManager` API from a placeholder into working infrastructure so
layer visibility and attached sources actually affect:

- the rendered scene
- `GetVisibleDatasets()`
- `FrameStats.visible_datasets`
- `FitToData()`

## 2. Status Rules

- `- [ ]` not accepted yet
- `- [x]` accepted and verified

## 3. Work Breakdown

### 3.1 Core Wiring

- [x] Move layer state into shared engine state instead of keeping it isolated inside the manager stub
- [x] Make visible dataset selection respect layer visibility
- [x] Keep backward compatibility when no layers are created yet

### 3.2 Source And Catalog Semantics

- [x] Allow visible chart layers with no attached source ids to surface catalog datasets
- [x] Allow visible layers with attached source ids to surface direct registered sources
- [x] Keep source attachment idempotent to avoid duplicate visibility and duplicate scene append

### 3.3 Engine Behavior

- [x] Rebuild the internal `ChartScene` after layer changes
- [x] Make `GetVisibleDatasets()` report layer-filtered results instead of all known datasets
- [x] Make `FitToData()` use the currently visible coverage instead of all loaded coverage
- [x] Make `FrameStats.visible_datasets` follow the current layer-filtered dataset count

### 3.4 Validation

- [x] Build passes after layer composition wiring
- [x] `navscene-layer-composition` validates layer visibility and direct-source attachment behavior
- [x] Existing smoke and render-path tests still pass after layer filtering is introduced

## 4. Current Note

Current status on `2026-04-11`:

- `ILayerManager` now affects actual dataset visibility instead of being a placeholder-only API
- the default behavior remains unchanged when the host does not create any layers
- once layers exist, visible content is driven by visible layer selection
- this is a foundational slice only; opacity blending, real multi-layer compositing, and per-layer portrayal are still future work
