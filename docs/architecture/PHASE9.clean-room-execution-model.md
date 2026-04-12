# Phase 9 Clean-Room Execution Model

Status: active

## 1. Goal

Phase 9 moves portrayal behavior away from one-off object-class branching toward a
clean-room execution layer that can eventually represent `S-52` style outputs such
as `AC`, `AP`, `LS`, `SY`, `TX`, `TE`, and conditional procedure families.

## 2. Responsibility Split

### 2.1 Static Catalog

The static portrayal catalog remains responsible for:

- named palette colors
- reusable base area, line, point, and text styles
- lookup resolution by geometry kind, object class, display category, and simple
  attribute conditions
- stable default priorities and label-role selection

### 2.2 Executable Rule Layer

The executable rule layer is responsible for:

- attribute-driven style mutation after catalog lookup
- enabling or suppressing fills and strokes
- switching palette references
- changing line width and pattern
- adding overlay/pattern instructions to the backend-neutral portrayal scene
- emitting supplemental decoration commands when one feature requires more than
  one portrayed symbol contribution in the final scene
- derived styling such as safety-band depth behavior or hazard emphasis
- future cross-object conditional procedures and symbol assembly

## 3. Internal Model

The current internal clean-room model uses:

- `RuleProgram`
  describes one executable portrayal program for a geometry/object-class family
- `RuleBranch`
  contains condition-gated instruction lists
- `RuleCondition`
  expresses attribute or derived-state predicates
- `RuleInstruction`
  expresses backend-neutral mutations such as palette selection, stroke changes,
  fill visibility, color mixing, and overlay emission

This model is currently implemented in:

- `src/portrayal/rule_program_internal.h`
- `src/portrayal/rule_program.cpp`

## 4. Migration Strategy

The migration path for Phase 9 is:

1. keep catalog lookup as the first stage
2. apply clean-room executable programs after lookup
3. preserve the existing portrayal scene output contract
4. migrate stable object families first
5. migrate the most important conditional procedure clusters after the execution
   layer is proven

## 5. Current Scope Of The Execution Layer

The first execution-layer batch now covers stable area/line rule families where
behavior is mostly attribute-local and does not yet require cross-object search:

- `AIRARE`
- `CBLARE`
- `UNSARE`
- `ADMARE`
- `BRIDGE`
- `TSEZNE`
- `ACHARE`
- `PSSARE`
- `VEGATN`
- `M_QUAL`
- line `DEPCNT`
- line `BRIDGE`
- line `COALNE`
- line `OBSTRN`
- line `SLCONS`
- point `OBSTRN`
- point `TOPMAR`
- point `UWTROC`
- point `WRECKS`

This newest batch also adds clean-room estimated-depth resolution for hazard
points so `OBSTRN` and `WRECKS` can follow OpenCPN-like danger defaults even
when explicit `VALSOU` is absent. It also introduces the first supplemental
point-decoration path for `QUAPOS`-driven low-accuracy markers without changing
the public SDK scene contract.

More complex families such as fuller `DEPARE`, `DRGARE`, `OBSTRN` area hazard
logic, richer `LIGHTS`, fuller `QUAPOS` symbol fidelity, and cross-object depth
procedures still remain in direct clean-room code and will be migrated in later
batches.

## 6. Why This Matters

This keeps the SDK architecture aligned with future needs:

- broader `S-52` portrayal coverage
- future `S-100` compatibility at the architecture level
- backend-neutral rendering outputs
- easier testing of portrayal behavior without renderer coupling
- cleaner incremental migration from heuristics to named rule/procedure execution
