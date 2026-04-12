# `navscene-sdk` Phase 7 Task

Status: completed

## 1. Phase Goal

Phase 7 focuses on the next gap after line-pattern fidelity:

- [x] reduce the remaining visual gap between the current clean-room renderer and richer `S-52` procedural portrayal behavior
- [x] move the portrayal system closer to table-driven symbol, palette, and procedure parity without exposing GPL implementation details
- [x] keep the SDK backend-agnostic while preparing for fuller `S-57` coverage and later `S-100` coexistence

## 2. Scope

### 2.1 In Scope

- [x] add clean-room support for higher-fidelity `S-52` line and area procedures where simple solid or dashed strokes are still insufficient
- [x] introduce palette indirection so portrayal styles can map through named chart colors instead of baking only day RGB triples
- [x] expand conditional portrayal by key attributes for prominent remaining classes in the bundled sample
- [x] keep external and embedded portrayal definitions aligned as the single source of portrayal truth
- [x] extend validation to capture procedure-driven visual changes in the bundled sample

### 2.2 Out Of Scope

- [ ] full bitmap-symbol atlas reproduction identical to OpenCPN
- [ ] `S-100` portrayal implementation
- [ ] 3D portrayal, terrain, or raster-basemap composition

## 3. Work Breakdown

### 3.1 M1 Procedure Gap Review

- [x] identify the highest-impact remaining procedure-driven differences in the bundled comparison after Phase 6
- [x] group those gaps into line-symbol, area-symbol, palette, and annotation categories

### 3.2 M2 Portrayal Model Expansion

- [ ] extend the portrayal command model beyond simple fills, plain strokes, and basic point symbols where needed. Not required for the accepted Phase 7 batch because the largest remaining differences are now mostly symbol-atlas and repeated-area-symbol fidelity, which stays out of scope.
- [x] add palette indirection so day, dusk, and night styles can evolve without duplicating raw RGB definitions everywhere
- [x] preserve clean separation between portrayal decisions and backend-specific rendering implementation

### 3.3 M3 Targeted Class Coverage

- [x] improve the next most visible procedure-heavy classes in the bundled sample, especially where OpenCPN relies on `LC(...)`, `AP(...)`, or conditional `CS(...)` behavior
- [x] add class-specific or attribute-specific annotations only when they clearly improve chart readability
Progress in this batch: palette-driven `day/dusk/night` color resolution is live, `PRDARE` moved closer to standard emphasized boundaries, `SLCONS` now uses a darker construction outline, `LNDELV` no longer falls through the generic line fallback, `TOPMAR` derives simplified shape/color portrayal from chart attributes, `OBSTRN` now covers area, line, and point hazard variants, `UWTROC` and `WRECKS` now vary by danger/depth semantics, `RESARE` and `ACHARE` differentiate special-use boundaries more clearly, and `M_QUAL` no longer falls through a plain metadata outline.

### 3.4 M4 Verification

- [x] update regression tests for the new portrayal model and catalog behavior
- [x] build the `build-gdal-verify` configuration successfully
- [x] pass the test suite in `build-gdal-verify`
- [x] regenerate validation artifacts and record the new delta in `docs/validation/`

## 4. Acceptance

- [x] the next most visible remaining sample differences no longer come primarily from missing procedure-driven line or area portrayal
- [x] palette handling is no longer hard-wired to a single day-style RGB set
- [x] the portrayal catalog remains the main source of styling truth
- [x] the public SDK boundary stays `C++` only and backend-agnostic

## 5. Starting Gap Snapshot

After Phase 6, the remaining highest-value gaps are expected to concentrate in:

- procedure-driven line symbols
  - several OpenCPN `LC(...)` behaviors still collapse to simple clean-room strokes
- procedure-driven area symbols and composite styling
  - some `CS(...)` and `AP(...)` behaviors still have no clean-room equivalent
- palette indirection
  - most styles still store direct RGB values instead of resolving through named chart palettes
- broader conditional portrayal
  - some important classes still need attribute-sensitive styling rather than one-class-one-style fallback

## 6. Current Verified Snapshot

- [x] standard `S-52` palette IDs now back the clean-room catalog with explicit `day`, `dusk`, and `night` RGB sets
- [x] portrayal output resolves palette IDs late in the engine so backend code still consumes plain resolved colors
- [x] Vulkan clear color now follows the portrayed scene background instead of a hard-wired day-only clear color
- [x] targeted class coverage improved for `PRDARE`, `SLCONS`, `LNDELV`, `TOPMAR`, `OBSTRN`, `UWTROC`, `RESARE`, `ACHARE`, `WRECKS`, and `M_QUAL`
- [x] hazard and obstruction labels now reuse `VALSOU` where chart readability clearly benefits and the source feature does not already provide a better object name
- [x] `build-gdal-verify` validation hash after this batch is `7836653780391631011`

## 7. Completion Notes

- [x] Phase 7 is accepted as complete for the clean-room scope
- Remaining differences against the bundled reference image now concentrate primarily in out-of-scope or later-scope items:
  - bitmap and composite symbol atlas fidelity
  - repeated area symbols and richer `AP(...)` pattern fills
  - denser annotation placement and label conflict handling closer to full chart products
  - optional metadata overlays and chart-frame decorations not enabled in the default validation view
