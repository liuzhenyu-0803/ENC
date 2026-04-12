# Phase 9 OpenCPN Gap Map

Status: active snapshot

## 1. Reference Inputs

- OpenCPN lookup source:
  `E:\projects\enc\OpenCPN\data\s57data\chartsymbols.xml`
- OpenCPN conditional symbology source:
  `E:\projects\enc\OpenCPN\libs\s52plib\src\s52cnsy.cpp`
- Current SDK lookup source:
  `E:\projects\enc\navscene-sdk\data\portrayal\s57_catalog.txt`
- Current SDK portrayal logic:
  `src/portrayal/engine_common.cpp`
  `src/portrayal/engine_area.cpp`
  `src/portrayal/engine_line.cpp`
  `src/portrayal/engine_point.cpp`
  `src/portrayal/engine_text.cpp`

## 2. Lookup Coverage Snapshot

- OpenCPN unique lookup object classes: `225`
- Current SDK unique lookup object classes: `69`
- Direct overlap with OpenCPN lookup classes: `68`
- Current SDK classes not present in the OpenCPN lookup-name set: `PSSARE`
- OpenCPN lookup classes not yet represented in the SDK catalog: `157`

This confirms the current SDK is still a targeted usable subset, not yet a broad
OpenCPN-level `S-52` lookup implementation.

## 3. Current SDK Strengths

The current SDK already has meaningful clean-room coverage in these families:

- Depth and water portrayal:
  `DEPARE`, `DRGARE`, `UNSARE`, `DEPCNT`, `SEAARE`, `LAKARE`, `CANALS`, `RIVERS`,
  `DOCARE`, `LOKBSN`
- Hazard portrayal:
  `OBSTRN`, `UWTROC`, `WRECKS`
- Navigation marks and lights:
  `LIGHTS`, `LITVES`, `BCNCAR`, `BCNISD`, `BCNLAT`, `BCNSAW`, `BCNSPP`,
  `BOYCAR`, `BOYISD`, `BOYLAT`, `BOYSAW`, `BOYSPP`, `TOPMAR`
- Coastal and man-made structures:
  `COALNE`, `SLCONS`, `BRIDGE`, `CBLARE`, `CBLSUB`, `ROADWY`, `RAILWY`, `PIPSOL`
- Areas with product-specific semantics:
  `ACHARE`, `RESARE`, `PSSARE`, `AIRARE`, `TSEZNE`, `VEGATN`, `SBDARE`
- Metadata and quality:
  `M_COVR`, `M_QUAL`
- Text and labels:
  soundings, contour labels, light labels, bridge labels, seabed labels, route
  orientation labels, named buoy/beacon/platform labels

## 4. Highest-Value Missing Lookup Classes

These are the most useful missing classes to prioritize next because they are
common in ENC viewing or strongly affect operator readability:

- Aids to navigation and pilotage:
  `DAYMAR`, `BERTHS`, `PILBOP`, `PLNPOS`, `RTPBCN`
- Harbour and checkpoint context:
  `CHKPNT`, `HRBFAC`, `HRBBSN`
- Shoreline and inland-water edge context:
  `RIVBNK`, `LAKSHR`, `DWRTCL`, `DWRTPT`
- Overhead and pipe/cable context:
  `CBLOHD`, `PIPARE`, `PIPOHD`, `OILBAR`
- Radio and ranging context:
  `RADRNG`, `RADSTA`
- General chart semantics and notices:
  `NEWOBJ`, `MIPARE`, `RETRFL`

These are not the whole remaining set, but they are strong candidates for the
next catalog expansion batch.

## 5. Conditional Procedure Snapshot

- OpenCPN named conditional procedures in `s52cnsy.cpp`: `29`
- Current SDK named procedure execution layer: not present yet

The current SDK instead uses object-class and attribute-specific heuristics
distributed across the modular portrayal engine. That is good enough for the
existing subset, but it is still below the target architecture for Phase 9.

## 6. Current Heuristic Equivalents

The SDK already contains partial clean-room equivalents for some high-value
behavior families, even though they are not yet represented as named procedures:

- `DEPARE01` and `DEPCNT02`:
  partial depth-band and safety-contour logic exists
- `LIGHTS05` and `LITDSN01`:
  partial light importance, labeling, and symbol variation exists
- `OBSTRN04`, `UDWHAZ03`, and `WRECKS02`:
  partial hazard, water-level, and danger styling exists
- `QUALIN01` and `QUAPOS01`:
  partial low-accuracy handling exists
- `RESARE02`:
  partial restriction-area emphasis exists
- `SEABED01`:
  partial seabed and rock-ledge handling exists
- `SOUNDG02` and `SNDFRM02`:
  partial sounding label handling exists
- `TOPMAR01`:
  partial topmark shape and color derivation exists
- `SLCONS03`:
  partial shoreline-construction treatment exists

The missing architectural step is to turn these scattered heuristics into a
data-driven instruction and rule layer that can execute `AC`, `AP`, `LS`, `SY`,
`TX`, `TE`, and `CS` style outputs explicitly.

## 7. Recommended Execution Order

1. Introduce the clean-room portrayal instruction model and execution layer.
2. Migrate the existing heuristic families into that execution layer without
   changing scene output.
3. Expand lookup coverage for the high-value missing object-class families.
4. Implement the core hazard/depth procedure cluster:
   `DEPARE01`, `DEPCNT02`, `OBSTRN04`, `UDWHAZ03`, `WRECKS02`
5. Implement the readability cluster:
   `LIGHTS05`, `TOPMAR01`, `SOUNDG02`, `QUALIN01`, `QUAPOS01`
6. Implement the policy-area cluster:
   `RESARE02`, `RESTRN01`, `RESCSP01`, `SEABED01`, `SLCONS03`

## 8. Practical Conclusion

The repo is now structurally ready for the next step, but still needs:

- much broader lookup-table coverage
- a true named rule/procedure execution layer
- procedure-family migration out of object-class-specific branching

This document should be treated as the Phase 9 working gap map for follow-up
implementation batches.
