---
name: implement_artboard
description: Use when implementing, changing, removing, or fixing ANY feature in the Arstro Artboard library (this repo — namespace `artboard`, the platform-free 2D drawing + UI framework). Enforces a V-model loop that keeps DOCS (requirements/architecture/detailed_design/design), DESIGN (PlantUML), CODE, and UNIT TESTS (100% core coverage, asserted via RecordingTarget) in sync, applies SOLID, and protects platform-independence: keep behavior in the platform-free core, make the IRenderTarget HAL seam as small as possible, and when a change touches the seam update EVERY adapter. Invoke for tasks like "add a gradient paint", "add a blur/glow effect", "add a Slider control", "add an offscreen layer", "fix the gesture router", "add a render-target op".
---

# implement_artboard

The single workflow for changing this repo (Arstro **Artboard**). Nothing ships unless
**doc, design, code, and tests move together**. Skipping a stage is the bug.

This skill lives in `core/Artboard/.claude/skills/` and is symlinked into the umbrella
(`arstro/.claude/skills/implement_artboard`). Always work **inside the `core/Artboard/` repo**;
paths below are relative to the Artboard repo root.

Umbrella layout (as of the `apps/` + `core/` reorganisation): the Arstro core libraries live
under `arstro/core/` — `core/Artboard/`, `core/DigitalSignalProcessing/`, `core/ImageProcessing/`
— and the applications that consume them live under `arstro/apps/` (`apps/cosmo`, `apps/genesis`,
`apps/pulsar`, `apps/launcher`, plus spec-stage `apps/solaris`, `apps/interstellar`). Small demos
stay at `arstro/examples/`. When a change touches the `IRenderTarget` seam, the adapters to update
are in this repo (`src/adapter/`), but the *consumers* to re-check are under `arstro/apps/` and
`arstro/examples/`.

## 0. Orient (facts about this repo)

- **What it is.** A platform-free 2D drawing + UI framework. The app builds a **scene of
  drawables** (pure graphics) and a **tree of segments** (interactive controls) against one
  abstract surface; a thin **adapter** renders that surface to a real device. The core is a
  *screen HAL* — it never rasterizes; it emits primitive ops.
- **Namespace** `artboard`. C++17. Branding stays `artboard`/`arstro` (no other prefixes).
- **The two HAL seams (the ONLY platform boundary):**
  - **Output HAL** = `IRenderTarget` (`src/render/RenderTarget.h`) — state stack, transform,
    paint, path building, `drawText`. Every backend implements exactly this.
  - **Input HAL** = `src/input/` (`RawPointer`/`Gesture`, `GestureRecognizer`, `InputRouter`).
    Adapter feeds raw down/up/move+button; the core derives click/double/right/drag/drop and
    hit-tests + routes.
- **Platform-free core** (built into `artboard_core`): `src/core` (Geometry, Color),
  `src/anim` (Easing, Animation, AnimatedProperty), `src/render` (IRenderTarget + the
  test/serialization `RecordingTarget`), `src/input`, `src/scene` (Drawable + Shapes,
  Artboard root), `src/ui` (Property, Segment, Theme, Controls). **No OS calls, ever.**
- **Adapters** live under `src/adapter/<device>/` and are selected at build time:
  - `src/adapter/web/Canvas2DTarget` — web (Emscripten + HTML Canvas2D).
  - `src/adapter/native/CairoTarget` — native (Cairo).
  - `src/render/RecordingTarget` — the **test adapter**: records every call as a `DrawOp`.
    It is platform-free, so it is how core/effect behavior is verified deterministically.
- **Docs (the doc surface) — `docs/`:** `requirements.md`, `architecture.md`,
  `detailed_design.md`, `design.md`, `architecture.puml`. (Artboard documents in `docs/`,
  not per-module READMEs.)
- **Tests:** `tests/coreTests.cpp` + harness `tests/MiniTest.h` (`TEST(name){ CHECK(...);
  CHECK_NEAR(a,b,eps); }`). Aggregate header: `include/artboard/artboard.h`.
- **Build & run tests:**
  `cmake -S . -B build && cmake --build build && ./build/artboard_tests`
  Web adapter (WASM) is built by the umbrella: from `arstro/`, `./build.sh --target linux-web-server`.

## 1. The V-model loop (do in order, every time)

Left side = specify & design (top→down). Right side = build & verify (bottom→up). Each right
stage validates the same-level left stage. **Do not start a stage until the left-side artifact
above it is updated.**

1. **REQUIREMENTS FIRST — read, then write, then check for conflict.** Open
   `docs/requirements.md` and **actually read it before doing anything else** — every agent
   working on this repo must load the same requirements first, so that everyone acknowledges
   and builds against the *same single source of truth*. Then state the change's contract: what
   the feature is, inputs/params (with units & ranges), expected visual/behavioral result, and
   what "correct" means. If the requirement doesn't exist, **write it before any code.**
   **Before you implement, check the new/changed requirement against the existing ones for
   conflict** (contradictory behavior, overlapping ownership, a rule that breaks an already-
   stated one). If it conflicts, resolve the conflict in `requirements.md` first — do not write
   code against a contradiction. No stage below starts until the requirement is written,
   conflict-checked, and read.
2. **ARCHITECTURE.** Update `docs/architecture.md` (and `design.md` for design intent) if the
   change adds/moves a module, a drawable, a control, or — critically — **touches the
   `IRenderTarget` / input HAL seam**. A seam change is an architecture change; document why
   it must be a primitive (see §3).
3. **DETAILED DESIGN + DESIGN (PlantUML).** Update `docs/detailed_design.md` and
   `docs/architecture.puml` to reflect the new/changed classes and relationships *before*
   coding. A class in code with no box in the puml is an unsynced design.
4. **IMPLEMENT (§2 SOLID, §3 platform rule).** Default to the **platform-free core**. Match
   surrounding style (brace placement, doc-comment header per file, `artboard` namespace).
5. **UNIT TEST → 100% core coverage (§4).** Add tests to `tests/coreTests.cpp`. For anything
   that draws, assert the **exact `DrawOp` stream** via `RecordingTarget` — that is how you
   prove behavior without a real device. Cover every new branch/param/edge.
6. **RUN unit tests** — `cmake --build build && ./build/artboard_tests`. Must report
   `0 failed`. For coverage, build the core with `g++ --coverage -O0` against
   `tests/coreTests.cpp` and run `gcov`; every touched core source must read
   `Lines executed:100.00%`. If a line is genuinely unreachable, delete the dead code rather
   than fake a test.
7. **ADAPTER VERIFY.** If you touched the HAL, build **every** adapter and confirm it renders
   (§3): web via `./build.sh --target linux-web-server` from `arstro/`; native (Cairo) via the
   relevant target. The `RecordingTarget` is updated and tested in step 5.
8. **SYNC CHECK (§5).** If any artifact lags, the task is not done.
9. **COMMIT — to `main`, always.** Once tests pass (`0 failed`, coverage met) and
   docs/puml/tests are in sync, **commit the change directly to the `main` branch** before
   moving on. Every implementation in this project is committed to `main` — do **not** open a
   side/feature branch, and do not leave verified work uncommitted. One focused commit per
   implemented+tested feature, in every repo it touched (Artboard, and the umbrella with its
   submodule bump); do not batch several features into one commit. Use the project's commit
   identity and end the message with the `Co-Authored-By` trailer. Push only when asked (or when
   the user has set up push access).

## 2. SOLID (how to add code)

- **SRP** — one responsibility per class. A new shape, paint, effect, or control is its own
  type, not a flag bolted onto an existing one.
- **OCP** — extend by adding a subclass/strategy, not by editing a switch in the core. New
  drawables subclass `Drawable` and emit themselves through `IRenderTarget::*` in `onDraw`.
  New controls subclass `Segment`.
- **LSP** — every `Drawable`/`Segment`/`IRenderTarget` implementation must honor the base
  contract (e.g. `Drawable::render` brackets `onDraw` with save/setTransform/restore; respect
  `visible`, parent transform composition).
- **ISP** — keep `IRenderTarget` minimal. Do **not** add per-shape or per-effect methods to
  the HAL; shapes/effects compose the existing primitives. Add a primitive only when it
  *cannot* be expressed by the existing ones (see §3).
- **DIP** — app, shapes, and UI depend only on `IRenderTarget` and the core abstractions,
  never on a concrete adapter.

### Adding a Drawable / Control (the contract)
```cpp
// in namespace artboard
class MyShape : public Drawable {           // OR : public Segment for an interactive control
public:
    Paint paint;                            // params as public fields / named setters
protected:
    void onDraw(IRenderTarget &t) const override {   // emit ONLY primitive ops — no rasterizing
        t.beginPath(); t.moveTo(/*…*/); /* … */ t.closePath();
        applyPaint(t, paint);               // reuse helpers; don't reinvent fill+stroke
    }
};
```
Rules: emit only `IRenderTarget` primitives in `onDraw`; never call OS/canvas APIs from a
drawable or control; reuse `Paint`/`applyPaint`/`Theme` rather than duplicating paint logic.

### Layout: snap, don't stack (default)
Sibling elements — labels, rows, cells, nodes, controls — **must snap to each other** (align
edge-to-edge or into a shared grid/column) so nothing lands on top of anything else.
**Overlap is a bug unless it is intentional.** The only elements allowed to sit over others are
**deliberate overlays** — modals, dropdowns, popups, tooltips, drag ghosts — the things drawn in
the second `renderOverlay`/`onOverlay` pass. Everything drawn in the normal `onDraw`/`onPaint`
pass is laid out so its bounds don't intersect a sibling's.

Concretely, when you position N things:
- Give each item its own slot. If two items can share a coordinate (e.g. two tree nodes at the
  same depth, two labels on the same baseline), that shared axis is **not** a valid layout key —
  switch to one that is unique per item (a per-item row/index, a running offset, a measured
  advance) so no two ever coincide. Fixed columns are fine **only** when the cross-axis is
  already unique per item.
- Derive each position from the previous item's extent (snap: `next = prev.edge + gap`) or from a
  grid, not from a value that can collide. Prefer the existing snap constraint (`Segment::snapTo`,
  `SnapEdge`) over hand-computed coordinates when gluing one segment's edge to another's.
- If you genuinely intend to stack (a badge on an avatar, an overlay scrim), say so in a comment
  and make sure it is drawn in the overlay pass or explicitly z-ordered — never rely on accidental
  draw order.
- Cover it in the `RecordingTarget` test: assert the recorded op positions do **not** collide
  (e.g. every row/label baseline is distinct; adjacent cells don't overlap).

### Overflow: if it cannot all fit, it scrolls (mandatory)

Snapping siblings into their own slots produces content **taller than the box that holds it**. The
failure that follows is silent and infuriating: rows are laid out, then clipped away or simply
dropped, and the user sees a panel that is plainly cut off with **no way to reach the rest**. Never
ship that.

**So for every panel, list, tree, or column you add or change, ask one question: can its content
ever exceed its box?** Content grows with the *document*, not with your test fixture — a shape
list, a property list, a track/step list, a log, a palette, a search result set, a text block all
grow without bound, so the answer is yes even if today's sample has three rows.

When the answer is yes:

- **Clip to the box, then make it reachable.** Clipping alone is only half the fix; a clip with no
  scroll is the bug. Wrap it in `ScrollView`, or give the panel a scroll offset that shifts its
  rows (`y = top - offset`) with a measured `maxOffset = max(0, content - viewport)`.
- **Measure both numbers every layout** — viewport height and content height — so the answer stays
  right after a resize, a document edit, or a font change. Never cache a content height computed
  from a fixture.
- **Measure the box you lay out in — define that box ONCE.** This is the trap, and it is silent.
  A list box has one top and one bottom; `measure()`, the row placement, the row-visible test, the
  paint clip, and the indicator must all read them from one accessor. Recompute the box inline at
  each site and the copies drift by a padding, and a viewport even a few pixels taller than the
  rows are allowed to occupy sets a limit that stops short — so the last row is unreachable at
  *every* offset, not merely awkward to reach. If rows are only shown when they fit **whole**
  (right for editable fields — half a text field is not editable), that shortfall hides a whole
  row. Assert reachability, not movement: walk the scroll to the end and check every row was
  fully visible at some point.
- **Clip while painting, too.** The offset shifts rows past *both* edges. Self-drawn content
  (headers, separators, chips) needs `save()`/`clipRect(box)`/`restore()` around the row loop, or
  a half-scrolled row draws over the column captions above and the footer below. A `break` on the
  first row past the bottom is not a clip: it leaves the top edge unguarded. And per-row
  decorations must be drawn on the same condition as the row's widgets, or chips float beside a
  hidden row.
- **Clamp at both ends.** `offset` stays within `[0, maxOffset]`; scrolling past either end is a
  no-op, not a runaway. When `maxOffset == 0`, `scrollable()` is false and the wheel **returns
  false so it bubbles** to an ancestor that can use it (see `Segment::dispatchGesture`,
  `InputRouter` scroll routing) — swallowing a wheel you cannot act on freezes the parent.
- **Show that it can scroll.** A scrollable panel draws its indicator (a thin rounded bar sized
  `viewport/content`, drawn only while scrollable), so "there is more" is visible without
  discovering it by accident. Off-screen content with no visible affordance does not exist.
- **Accept both the wheel and a drag.** `Gesture::Type::Scroll` (pixel `delta`) and a drag on the
  body must land in the same place; a wheel also cancels any kinetic/fling motion rather than
  fighting it.
- **Do not silently drop rows.** If a row does not fit, it is scrolled to — not skipped. A
  `break` in a row loop is acceptable **only** as a draw-time clip for rows already reachable by
  scrolling.

**Test it, in this exact shape** (see
`Every_clipped_panel_scrolls_by_wheel_and_clamps_at_both_ends`): build a document with far more
rows than fit, then for **each** overflowing list assert (1) `scrollable()` is true, (2) a wheel
over it moves `offset()` off zero, (3) wheeling to the end and once more leaves `offset()`
unchanged, (4) wheeling back reaches exactly `0`. Add the mirror case: a list with nothing to
scroll reports `scrollable() == false` and ignores the wheel. Point the wheel at a *row inside*
the list, not at its padding — that is what proves bubbling works. And build the fixture so the
list **actually on screen** is the one that overflows; padding a list nobody is looking at proves
nothing.

## 2A. Visual & interaction quality (design taste)

Correct-and-tested is the floor; a control also has to *look and feel* deliberate. These are the
transferable rules from the `design-taste-frontend` skill, adapted for this native 2D framework —
its web stack (Tailwind / React / Motion / design-system packages / web fonts) does **not** apply
here; the taste does. Apply them to any new or changed control, panel, or app screen.

- **Everything animates — nothing snaps (mandatory).** All movement and change in the UI must be
  smoothly animated. **No component may suddenly change size, appear, disappear, move, recolor, or
  reflow in a single frame.** Every property that affects what the user sees — position, size,
  opacity/visibility (fade, don't pop), color, corner radius, scroll/zoom offset, panel
  open/close, list insert/remove — MUST change its value through an animation primitive
  (`AnimatedProperty` / `Property` / `Spring`), never by direct assignment of the visible value.
  Show/hide is a fade or size tween to/from zero, not a `visible` flip; layout changes ease into
  their new coordinates. The only exception is `artboard::reducedMotion()`, which collapses each
  of these to its final state instantly (below). If you cannot drive an animation in the available
  scope, that is a reason to fix the driving loop — not to snap the value.
- **Motion must be motivated.** Every animation states *why* in one sentence: hierarchy (draw the
  eye), feedback (acknowledge a press), state transition (show what changed), or reveal (sequence
  content in). "It looked cool" is not a reason; no idle/looping motion on informational elements.
- **Motion claimed = motion shown, or drop it.** Time-based motion needs the host's frame tick
  (`advance(nowMs)` called every frame). If you spec an animation, drive it with an
  `AnimatedProperty` / `Property` / `Spring` and confirm it actually moves across frames — never
  ship a half-built tween that snaps. If you can't drive it in the available scope, ship the clean
  static end-state instead.
- **Selection, tab, and press transitions animate specifically (not just "generally").** A moving
  selection — a tab bar, a `SegmentedControl`, a list/section highlight — MUST **slide** its
  highlight box / underline to the newly-selected item (animate x/width via `Property`/`Spring`)
  **or** cross-fade its colour to the new highlight; repainting the active state at its new place
  in one frame is a bug. Switching tab/section **content** slides the new content in (eased
  horizontal translate ± cross-fade), never an instant swap. Every button/tappable shows an
  **animated press response** (background wash or subtle scale) on press-down and eases back on
  release/cancel. A fixed bar (e.g. a bottom tab bar) **stays put while an attached panel/sheet
  slides behind it** — the bar does not ride with the sheet. Assert one of these with a
  `RecordingTarget` sample at t=0/mid/end where practical.
- **Ease, don't lerp-linear.** Use `Easing::EaseOut*` / `Spring` for UI motion (position, size,
  reveal); linear reads mechanical. Keep durations short (≈120–220 ms).
- **Reduced motion is mandatory.** Honor `artboard::reducedMotion()`: when set, motion collapses
  to its final state instantly. The motion primitives already do this — don't re-introduce motion
  that ignores it.
- **Consistency locks (per screen).** ONE accent colour, ONE corner-radius scale, ONE type ramp,
  pulled from `Theme` / design tokens rather than hand-picked per widget. A control that invents
  its own blue or radius is a bug; match the surrounding surface's density and rhythm.
- **Contrast.** Text, icons, and fills stay legible on their background (aim WCAG AA: ≈4.5:1 body,
  ≈3:1 large text / against a fill). No low-contrast label-on-fill (e.g. near-white text on a light
  highlight) — add a scrim/overlay or pick a legible pair.
- **Draw every state, not just the happy path.** idle / hover-or-focus (where the input model has
  it) / pressed / active / disabled, plus **empty** and **loading** where a panel can have no data
  yet. A control that renders only its selected/full state is unfinished — and a control whose
  children are created `visible=true` before their first data push will pile up at (0,0): default
  them hidden/positioned for the empty state.
- **Anti-slop.** Reach past the obvious default: align to a grid, respect whitespace, keep labels
  terse and real, and don't stack decorative dividers/dots or duplicate the same affordance twice.
- **A deferred action must outlast the gesture that would cancel it.** If a control defers an
  action so a later gesture can pre-empt it (e.g. a click-to-jump deferred so a double-click can
  reset instead), the defer window must be `>=` the window of the cancelling gesture (the
  GestureRecognizer double-click window, default 300ms) — a shorter guard lets the action fire
  between the two clicks and flash the wrong state. Also cancel the pending action on the next
  press (the second click's `Down`), so the outcome is clean regardless of exact timing. Assert
  it with a slow-gesture test that ticks `advance()` between the two clicks (see
  `Slider_slow_double_click_no_flash_toward_cursor`).
- **Prove it with `RecordingTarget`.** Where practical, assert the motion/state in a test: sample
  an `AnimatedProperty` at t = 0 / mid / end, or assert the op stream differs between states — the
  same way §4 proves layout.

## 3. Platform-independence rule (non-negotiable)

**Order of preference for any new capability:**

1. **Express it in the platform-free core** by composing existing `IRenderTarget` primitives
   (paths, transforms, paint, text). This is always the first choice. Effects like blur/glow
   that need offscreen compositing should, wherever possible, be built as a **core software
   compositor** so they render *identically on every adapter* — including no-GPU targets.
2. **Only if it is impossible to express with existing primitives**, extend the HAL — and keep
   the new surface **as small as possible** (ISP). One new primitive, minimal arguments,
   documented in `architecture.md` as to why it must be a primitive.
3. **When you extend the HAL, you MUST update EVERY adapter in the same change:**
   - `src/render/RecordingTarget.{h,cpp}` (+ its `DrawOp::Kind` enum) — required, and it is
     what the unit tests assert against.
   - `src/adapter/web/Canvas2DTarget.{h,cpp}`.
   - `src/adapter/native/CairoTarget.{h,cpp}`.
   - any other adapter present under `src/adapter/`.
   A HAL method declared but unimplemented in even one adapter is an incomplete change.

**Trade-off rule (explicit):** when consistency-across-platforms conflicts with performance,
**prioritize identical behavior over performance, and put the cost on the adapter side.**
Prefer doing extra work inside an adapter (or the core software fallback) so the visible
result is the same everywhere, even if slower, over a faster path that makes one platform look
different. A platform may *accelerate* the result natively (e.g. a GPU/Skia adapter doing blur
in hardware) **only if** its output matches the core software reference — the software path is
the source of truth, the conformance reference, and the floor every adapter must meet.

## 4. Unit tests (`tests/coreTests.cpp` via RecordingTarget)

- For drawing/effect behavior, render through `RecordingTarget` and assert the recorded
  `DrawOp` stream (op kinds, coordinates with `CHECK_NEAR`, colors, transforms, text). This is
  device-free and deterministic — the canonical way to verify the core.
- For geometry/animation/input/property logic, assert values directly (`CHECK`, `CHECK_NEAR`).
- For UI, drive the `InputRouter`/gestures and assert focus, hit-testing, and emitted ops.
- New tests register automatically via the `TEST(...)` macro (static registry in
  `MiniTest.h`); they all build into the single `artboard_tests` target — no CMake edit needed
  unless you add a new source file (then add it to the `artboard_tests` executable in
  `CMakeLists.txt`).
- Cover **every** new line and branch. Aim: touched core sources at `Lines executed:100.00%`.

## 5. Definition of done — the sync checklist

- [ ] `docs/requirements.md` was **read first**, states the feature's contract (added if it was
      missing), and the new/changed requirement was **checked against the existing ones for
      conflict** (conflicts resolved in `requirements.md` before any code).
- [ ] **Everything animates, nothing snaps:** every visible property change (position, size,
      show/hide via fade, color, radius, scroll/zoom, panel open/close, list insert/remove) goes
      through an animation primitive — no single-frame size change / pop-in / pop-out / jump — and
      collapses to the final state only under `reducedMotion()`.
- [ ] `docs/architecture.md` + `docs/design.md` reflect any module/seam change.
- [ ] `docs/detailed_design.md` + `docs/architecture.puml` match the code (classes, params,
      relationships) — no code class without a puml box.
- [ ] Code keeps behavior in the platform-free core; HAL surface grew **only** if unavoidable,
      and minimally.
- [ ] **If the HAL changed:** `RecordingTarget`, `Canvas2DTarget`, `CairoTarget`, and every
      other adapter were all updated and build.
- [ ] Any cross-platform/performance trade-off resolved in favor of identical behavior, with
      the cost on the adapter / core-software side; software path is the reference.
- [ ] `./build/artboard_tests` reports `0 failed`; touched core sources at 100% line coverage.
- [ ] SOLID respected (new behavior = new type; `IRenderTarget` stayed minimal).
- [ ] Layout snaps, doesn't stack: sibling elements align/don't overlap; any overlap is an
      intentional overlay (modal/dropdown/tooltip drawn in the overlay pass) and is commented,
      with a test asserting recorded positions don't collide.
- [ ] **Overflow scrolls (§2):** every panel/list/tree whose content can outgrow its box clips
      **and** scrolls — viewport + content measured every layout, offset clamped at both ends, a
      visible indicator while scrollable, wheel **and** drag land in the same place, an unscrollable
      list bubbles the wheel instead of eating it, and no row is silently dropped. The list box is
      defined **once** and `measure()`/placement/visible-test/clip/bar all read it. Tested per list:
      scrollable → wheel moves it → clamps at the end → returns exactly to 0, **and** every row was
      fully visible at some offset along the way.
- [ ] Visual/interaction quality (§2A): motion is motivated + actually driven each frame (no
      snapping half-tween) + eased + honors `reducedMotion()`; colour/radius/type pulled from
      `Theme` (consistency locks); text/fills legible (contrast); empty & loading states drawn
      (children default hidden/positioned before first data), not just the happy path.
- [ ] Branding stays `artboard`/`arstro`.
- [ ] **Committed to `main`** — the implemented + tested feature is committed directly to the
      `main` branch (one focused commit per feature, every touched repo; no side branch), not
      left in the working tree.

If you changed code but not the docs/puml/tests (or extended the HAL but not every adapter),
or you left a verified feature uncommitted, you are **not done**.
