# AGENTS.md

## 1. Document Map

- `AGENTS.md`
  - project-level execution rules and persistent collaboration constraints
- `docs/architecture/PLAN.closed-source-sdk.md`
  - long-term architecture boundaries, layering, and extensibility direction
- `docs/architecture/PHASE9.clean-room-execution-model.md`
  - active internal portrayal instruction and execution-layer model for phase-9 migration
- `docs/execution/TASK.phase7-s52-procedural-parity.md`
  - accepted phase-7 `S-52` procedural parity document
- `docs/execution/TASK.phase9-full-s57-s52-clean-room-rendering.md`
  - active long-running execution document for substantially complete clean-room `S-57 -> S-52` rendering
- `docs/execution/archive/TASK.phase6-conditional-portrayal-and-line-fidelity.md`
  - completed phase-6 conditional portrayal and line-fidelity document
- `docs/execution/archive/TASK.phase8-area-pattern-and-color-fidelity.md`
  - archived phase-8 area-pattern and color-fidelity investigation batch
- `docs/execution/archive/TASK.phase5-point-symbols-and-labels.md`
  - completed phase-5 point-symbol and label-fidelity document
- `docs/execution/archive/TASK.phase2-chart-selection-quilt.md`
  - completed phase-2 baseline document retained for context
- `docs/execution/archive/TASK.phase3-s57-portrayal-system.md`
  - completed phase-3 portrayal-system execution document
- `docs/execution/archive/TASK.phase4-area-portrayal-palette.md`
  - completed phase-4 area portrayal and palette refinement document
- `docs/execution/archive/`
  - accepted execution documents from completed phases
- `docs/decisions/DECISIONS.md`
  - important architectural and product decisions
- `docs/guides/AI_WORKFLOW_STANDARD.md`
  - reusable standard AI development workflow reference
- `docs/reference/PHASE9.opencpn-gap-map.md`
  - active OpenCPN lookup and conditional-procedure gap snapshot for phase-9 execution
- `docs/validation/PHASE5.point-symbols-and-labels.md`
  - validation evidence for completed phase-5 point-symbol and label work
- `docs/validation/PHASE7.palette-indirection-and-targeted-line-coverage.md`
  - validation evidence for the verified phase-7 palette and targeted-coverage batch
- `docs/validation/PHASE6.conditional-portrayal-and-line-fidelity.md`
  - validation evidence for completed phase-6 conditional portrayal and line-fidelity work

## 2. Execution Rules

- Default behavior is to continue autonomously until the current phase goal is fully achieved.
- After each accepted sub-task, immediately continue to the next sub-task.
- Do not pause to ask whether to continue unless there is a real blocker, hidden-risk decision, or target change.
- Before changing scope, acceptance criteria, or architecture direction, update the relevant document first and then continue implementation.

## 3. Task Status Rules

- `- [ ]` means not accepted yet, including partial progress.
- `- [x]` means accepted and verified.
- If work is partial, blocked, or descoped, keep `- [ ]` and annotate inline.

## 4. Documentation Update Rules

- After each accepted sub-task, update the current active execution document immediately.
- Do not leave progress only in chat history.
- If a blocker appears, record it in an execution document before or while handling it.
- If architecture, boundaries, or key tradeoffs change, also update:
  - `docs/architecture/PLAN.closed-source-sdk.md`
  - `docs/decisions/DECISIONS.md`
- New sessions should trust repository documents over chat memory.

## 5. Current Phase

- The active execution phase document is:
  - [TASK.phase9-full-s57-s52-clean-room-rendering.md](/E:/projects/enc/navscene-sdk/docs/execution/TASK.phase9-full-s57-s52-clean-room-rendering.md)
- The latest accepted baseline remains:
  - [TASK.phase7-s52-procedural-parity.md](/E:/projects/enc/navscene-sdk/docs/execution/TASK.phase7-s52-procedural-parity.md)
- The latest verified Phase 9 batch is:
  - portrayal execution-layer expansion for low-accuracy shoreline handling
    (`COALNE`, `SLCONS`, and supplemental `QUAPOS` point markers), on top of
    the previously verified hazard and split-architecture batches, all completed
    in `build-gdal-verify`

## 6. Product And Architecture Constraints

- No GPL code or GPL-derived implementation enters this repository.
- OpenCPN may only be used as product and behavior reference.
- Public SDK stays `C++` only.
- Public SDK must not expose `Qt` types.
- Public SDK must not expose Vulkan-native types.
- Host applications pass native surface or window handles into the SDK.
- Renderer backends stay replaceable and must not own chart-selection policy.
- Renderer code must not consume raw `GDAL` or raw `S-57` objects directly.
- The design must remain extensible for `S-100`, overlays, raster basemaps, satellite imagery, 2.5D/3D, Android, and additional render backends.

## 7. Project Organization Rules

- Keep the repository root minimal.
- Put explanatory and process documents under `docs/`.
- Prefer clear classification under:
  - `docs/architecture`
  - `docs/execution`
  - `docs/decisions`
  - `docs/guides`
- New files and folders must be placed intentionally, not dropped into the root by convenience.
- Any file that grows too large should be split by responsibility, phase, module, or topic.

## 8. Codebase Hygiene Rules

- Reuse existing modules when responsibilities already fit.
- Keep each file, class, and module focused on a single clear job.
- Demo code must not leak host-framework concerns into the SDK boundary.
- Temporary implementations that survive beyond a short experiment must be cleaned up into proper structure.
- Keep naming consistent with the existing project style.
- Use UTF-8 for text files whenever possible.

## 9. Verification Rules

- Every meaningful batch of changes should be validated by build, tests, runtime checks, or a documented limitation.
- Do not treat `should work` as accepted completion.
- If verification is incomplete, record exactly what was verified and what was not.

## 10. Persistent Collaboration Memory

- Completion state must always be written back into the active execution document.
- After one accepted task is done, continue directly into the next task until the phase goal is complete.
- Do not ask for confirmation before ordinary commands, builds, runs, or code edits.
- Keep documents organized, keep files reasonably sized, and keep long-term rules written in repository files rather than only in chat.
