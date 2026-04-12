# Phase 3 Covered Object Classes

## 1. Purpose

This document lists the `S-57` object classes and portrayal behaviors intentionally covered by the completed Phase 3 baseline.

The goal is not full `S-52` parity. The goal is to record what Phase 3 already portrays semantically and what is still handled by generic fallback styling.

## 2. Area Coverage

Dedicated clean-room area styling exists for:

- `LNDARE`
- `SEAARE`
- `DEPARE`
- `DRGARE`
- `UNSARE`
- `TIDEWY`
- `LAKARE`
- `BUAARE`
- `HRBARE`
- `PRDARE`
- `SBDARE`
- `RESARE`
- `ACHARE`
- `PSSARE`
- `M_COVR`
- `M_QUAL`

Derived area behavior:

- `DEPARE`, `DRGARE`, `UNSARE`, `TIDEWY`, and `LAKARE` use depth-bucket styling from `DRVAL1` and `DRVAL2`
- metadata and quality layers honor visibility settings instead of always rendering

Fallback behavior:

- unmatched area classes still render through a generic clean-room area style instead of disappearing

## 3. Line Coverage

Dedicated clean-room line styling exists for:

- `COALNE`
- `DEPCNT`
- `CBLSUB`
- `FAIRWY`
- `NAVLNE`

Derived line behavior:

- `DEPCNT` emphasizes the configured safety contour
- `COALNE`, `FAIRWY`, and `NAVLNE` respond to the symbolized-boundary display option

Fallback behavior:

- unmatched line classes render through a generic clean-room line style

## 4. Point Coverage

Dedicated clean-room point styling exists for:

- `SOUNDG`
- `OBSTRN`
- `LIGHTS`
- `BCNCAR`
- `BCNISD`
- `BCNLAT`
- `BCNSAW`
- `BCNSPP`
- `BOYCAR`
- `BOYISD`
- `BOYLAT`
- `BOYSAW`
- `BOYSPP`

Derived point behavior:

- lights support attribute-driven important-label lookup
- navigation marks respond to the simplified-point display option

Fallback behavior:

- unmatched point classes render through a generic clean-room point style

## 5. Text And Label Coverage

Phase 3 label extraction intentionally covers:

- `VALSOU` for `SOUNDG`
- `OBJNAM`
- `NOBJNM`
- `LITCHR` for `LIGHTS`
- `VALDCO` for `DEPCNT`

Phase 3 label behavior includes:

- font roles in the portrayal layer
- deterministic ordering
- important-label-first collision handling
- exported collision-box evidence for regression checks

## 6. Display-Setting Coverage

Phase 3 portrayal responds to these display settings:

- `display_category`
- `color_scheme`
- `show_text`
- `show_soundings`
- `show_lights`
- `show_meta`
- `show_quality_of_data`
- `simplified_points`
- `symbolized_boundaries`
- `safety_contour_m`
- `shallow_contour_m`
- `deep_contour_m`
- `estimated_display_scale`

Phase 3 filtering behavior includes:

- `SCAMIN`
- metadata visibility
- quality-of-data visibility
- sounding visibility
- light visibility

## 7. Real-Sample Evidence

The bundled `GB4X0000` validation render currently includes prominent output from classes such as:

- `DEPARE`
- `DEPCNT`
- `LIGHTS`
- `SBDARE`
- `SLCONS`
- `LNDELV`
- `OBSTRN`
- `COALNE`
- `ROADWY`
- `TOPMAR`
- `UWTROC`
- `LNDARE`
- `LNDMRK`
- `BRIDGE`
- `BUAARE`
- `SEAARE`

Not every class above has a dedicated custom style yet. Some are covered by deliberate generic fallback rendering, which is still part of the Phase 3 baseline.
