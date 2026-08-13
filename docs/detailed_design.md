# Arstro Artboard — Software Detailed Design

## 1. `Segment`

`Segment` is the retained UI base class.

### Responsibilities

- Store animated `x`, `y`, `width`, and `height` properties.
- Render itself and then render children recursively.
- Convert world-space pointer coordinates to local coordinates using inverse transforms.
- Participate in a shared focus group.
- Own hover state and route hover moves (FR-24).
- Capture child interaction during a press-drag-drop sequence.

### Important fields

- `Property x, y, width, height`
- `Property opacity` (FR-32) — group alpha, default `1`; `static constexpr kOpacityEpsilon = 1e-3`
- `Property rotation, scaleX, scaleY, pivotX, pivotY` (FR-33) — the animated transform channel
- `bool enabled, visible, focusable, clipToBounds`
- `int focusIndex`
- `std::vector<std::shared_ptr<Segment>> mChildren`
- `std::shared_ptr<InputController> mInputController`
- `bool mHovered, mHoverPrev` + `Property mHoverAmount` — the hover state and its animated
  `[0,1]` factor (a single global hover owner is held in an anonymous-namespace slot in the .cpp).

### Important operations

- `render()` (FR-32) is a thin opacity wrapper around `renderContent()`: it returns immediately when
  `isFadedOut()`, calls `renderContent()` directly when the alpha is `>= 1 - kOpacityEpsilon` (so the
  common opaque case opens no layer), and otherwise brackets the *whole* call — own paint plus every
  child — in one `pushLayer(alpha)` / `popLayer()` pair. `renderContent()` holds the original body:
  compose parent and local transforms, paint self, then paint children. `renderOverlay()` /
  `renderOverlayContent()` split the same way. Nested opacity composes multiplicatively because a
  child's layer composites into its parent's.
- `localTransform()` (FR-33) returns `translate(x,y) · translate(pivot) · rotate(rotation) ·
  scale(scaleX,scaleY) · translate(-pivot) · transform`. The pivot/rotate/scale block is skipped
  entirely when rotation is `0` and both scales are `1`, so the default collapses to the original
  `translate(x,y).mul(transform)` bit-for-bit. Hit testing needs no change: `toLocal()` already maps
  through `worldTransform().inverse()`, so a rotated/scaled segment is tested in its own frame.
- `hitTest()` rejects `isFadedOut()` segments (FR-32: a panel faded to 0 must stop swallowing
  clicks), then checks children from topmost to backmost, then the local bounds.
- `onGesture()` delegates to `dispatchGesture()`.
- `dispatchGesture()` on a bare `Move` (no press capture) recurses to the deepest hit-tested child
  (as `Down` does) and, at the leaf, calls `setHovered(this)` and delivers the move to the handler;
  a `Move` during a press still goes to the captured child (FR-24).
- `advance(nowMs)` first calls `updateHoverAnim(nowMs)`, which eases `mHoverAmount` toward `1`
  while hovered/`enabled`/`visible` and `0` otherwise (≈120 ms `EaseOutCubic`, reduced-motion-safe),
  then updates the layout properties, `opacity` (FR-32), the five transform properties (FR-33), and
  children.
- `setHovered(seg)` / `hoveredSegment()` manage the one global hover owner (clearing the previous);
  the destructor relinquishes hover if this segment owned it. `isHovered()` / `hoverAmount()` /
  `isHoverWithin()` (self-or-descendant) are the read side controls use.
- `dispatchKey()` routes keyboard events to the focused segment.
- `requestFocus()` updates the shared focus registry.

## 2. `Property`

`Property` is a thin UI-facing wrapper around `AnimatedProperty`.

### Responsibilities

- Store the current scalar value.
- Start an animation toward a target value (simple `animateTo`, or a full `Tween`).
- Advance the value at a supplied time.

## 2b0. `ui::Observable<T>` (FR-23)

A single-source-of-truth value with change notification — the state-link primitive. Header-only
(`ui/base/Observable.h`); a `T mValue` plus a `std::vector<std::function<void(const T&)>>` of
observers.

### Responsibilities

- `get()` returns the current value; `set(v)` replaces it and notifies every observer **only when
  the value changed** (equality-guarded, so mutually-bound observers can't recurse).
- `observe(fn, fireNow = true)` registers an observer, firing it immediately by default so the
  view initialises in sync; a null `fn` is rejected (never stored), so `set()` needs no null guard.
- No drawing, no HAL — it is pure state. Callers decide which fields observe it (a button's
  `active`, a panel's `visible`/target width), giving one authoritative value instead of N copies
  that can disagree. Sits beside `Property` and `ModBus` as the third `ui/base` state primitive.

## 2a. `anim::Easing` / `applyEasing`

A library of easing curves as a pure function `applyEasing(Easing, t)` with `t` clamped to `[0,1]`.
Families: linear; quad/cubic/quart (in/out/in-out); sine, expo (in/out/in-out); back, elastic
(in/out/in-out, may overshoot mid-curve but pinned to `0` at `t=0` and `1` at `t=1`); bounce
(in/out/in-out). No backend code; trivially unit-testable.

## 2a-i. Cubic-bezier `Easing` curves + `anim::MotionTokens`

Five `Easing` entries (`Standard`, `StandardDecel`, `StandardAccel`, `EmphasizedDecel`,
`EmphasizedAccel`, FR-31) are named cubic-bezier control-point curves (matching CSS
`cubic-bezier(x1,y1,x2,y2)` timing functions) rather than a closed-form polynomial. `Easing.cpp`
gains two anonymous-namespace helpers: `cubicBezierSolveX(x1, x2, t)` solves the bezier's
parametric `x(u) = t` for `u` via Newton-Raphson (8 iterations) with a bisection fallback (30
iterations) for robustness when the derivative is near zero, and `cubicBezierY(x1, y1, x2, y2,
t)` calls it and evaluates `y(u)` at the resulting `u`. Each of the five `applyEasing` cases
calls `cubicBezierY` with its named control points. Because the underlying bezier is anchored at
`(0,0)` and `(1,1)`, every curve still pins `0` at `t=0` and `1` at `t=1` (FR-4a) with no special
casing. This keeps curve lookup to the single existing `Easing`/`applyEasing` seam — no second
mechanism for "curves with arbitrary control points" alongside the closed-form ones.

`anim/MotionTokens.h` (header-only, like `Observable`) is pure named data with no new primitive:
a millisecond duration scale (`kDurationShort1..4`, `kDurationMedium1..4`, `kDurationLong1..4`)
and `Spring` settle-speed presets (`kSpatialFast/Default/Slow`, `kEffectsFast/Default/Slow` —
"spatial" for position/size motion, "effects" for fades/colour, faster) that are just named
`omega` values consumed by the existing `Spring::advance(dtSeconds, omega)` (FR-4d). It carries
no dependency the rest of `anim` doesn't already have.

## 2b. `anim::Tween`

A pure value type describing a whole scalar animation.

### Fields

- `double from, to, durationMs, delayMs`
- `Easing easing`
- `int repeat` (additional cycles; `-1` = infinite)
- `bool yoyo` (reverse direction on odd cycles)

### Operations

- `at(elapsedMs)` — sampled value: holds `from` during `delayMs`, eases across each cycle, applies
  yoyo on odd cycles, and clamps to the final value once finished.
- `totalMs()` — `delayMs + durationMs * (repeat+1)`; `+inf` when infinite.
- `finished(elapsedMs)` — `false` for infinite tweens; otherwise `elapsedMs >= totalMs()`.

Purity means the same `Tween` yields identical output for live UI and offline rendering.

## 2c. `anim::AnimatedProperty`

Holds a live scalar, an optional active `Tween`, and a start timestamp.

### Operations

- `set(v)` — snap, cancel any animation.
- `animateTo(target, durationMs, easing, nowMs)` — back-compatible single-shot; `from` is current.
- `animate(tween, nowMs, onComplete)` — full control via a `Tween`; fires `onComplete` once when the
  (finite) tween finishes.
- `update(nowMs)` — recompute the value from the tween; deactivate + fire `onComplete` at the end.
- Reduced-motion (FR-4e): when `reducedMotion()` is set, `animateTo`/`animate` skip the tween and
  snap `value` to the resting target (the tween's `to`), leaving the property inactive and firing
  `onComplete` immediately.

## 2d. `anim::Animator`

A callback-based timeline that lets an application animate **any** value without a `Segment`.

### Design

- Owns tracks (`std::shared_ptr<Track>`), each holding a `Tween`, a lazily-captured start time, an
  `onUpdate(double)` callback, and an optional `onComplete()`.
- `tween(from, to, durationMs)` returns a fluent `Handle` (`easing/delay/repeat/yoyo/onUpdate/
  onComplete`) so a complete animation reads as one expression.
- `advance(nowMs)` ticks every track (lazily stamping its start on first tick), invokes `onUpdate`
  with the sampled value, fires `onComplete` for finished tracks, and erases them.
- `clear()` drops all tracks; `activeCount()` reports the live track count.
- Depends only on `Tween` + `std::function`; SRP (timing/dispatch only), OCP (new behavior via
  callbacks not new core branches), DIP (no backend, no UI coupling).

## 2c-i. `anim::Spring` (framerate-independent follower)

A critically-damped follower for *display smoothing* (a value that should glide to a target rather
than snap). Fields: `value`, `target`, `velocity`.

### Operations

- `reset(v)` — snap value and target to `v`, zero velocity.
- `setTarget(t)` / `target()` / `value()` / `velocity()`.
- `advance(dtSeconds, omega)` — step the value toward the target. Uses the **closed-form**
  critically-damped solution `y(t) = (y0 + (v0 + ω·y0)·t)·e^(−ω·t)` (with `y = value − target`), so
  the trajectory is identical whether advanced in one big step or many small ones (framerate
  independence, FR-4d). `dt` is clamped to `0.05s` to bound a long stall; `omega` (rad/s) sets the
  settle speed (`18` ≈ 0.2s, the previous hand-tuned constant). Critically damped ⇒ no overshoot.
- `isMoving(eps)` — value/velocity still meaningfully away from the target.

Replaces the per-frame semi-implicit Euler blocks previously duplicated in `Knob` and `Slider`
(one source of truth). When `reducedMotion()` is set, `advance` jumps straight to the target.

## 2c-ii. `anim::reducedMotion` (accessibility switch)

Free functions `setReducedMotion(bool)` / `reducedMotion()` backed by a single translation-unit
flag (default off, FR-4e). `Spring::advance` and `AnimatedProperty::animateTo`/`animate` consult it;
when on, springs jump to target and tweens snap to their resting value and fire `onComplete`, so all
framework motion collapses to instant with no per-control special-casing.

## 2e. `Segment` snap constraint

`Segment` can glue one of its edges to another segment's edge with an offset.

### API

- `enum class SnapEdge { Left, Right, Top, Bottom, CenterX, CenterY }`.
- `snapTo(Segment *target, SnapEdge myEdge, SnapEdge targetEdge, double offset = 0)` — store the
  constraint (ignored if `target` is null or `this`).
- `clearSnap()` / `bool hasSnap()`.

### Resolution

- `advance(nowMs)` updates `x/y/width/height`, then calls `resolveSnap()`.
- `edgeCoord(edge)` = the edge's coordinate in parent space (`Left=x`, `Right=x+w`,
  `CenterX=x+w/2`, and the `y` analogues). `edgeInset(edge)` = the edge's distance from the
  segment origin (`0`, `w`, `w/2`, …).
- Horizontal `myEdge` sets `x = target.edgeCoord(targetEdge) + offset − edgeInset(myEdge)`;
  vertical sets `y` analogously. So when the target moves, the snapped segment tracks it. The
  target is held as a raw pointer (same ownership model as the parent pointer; caller keeps it
  alive). Snapping is pure geometry — no HAL involvement.

## 2f. `LinearLayout` / `Row` / `Column`

Linear layout containers (`ui/base/LinearLayout`, `ui/concrete/Row`, `ui/concrete/Column`).

- `LinearLayout : Segment` holds `spacing` and `padding` and an axis flag; `Row(true)` /
  `Column(false)` are thin subclasses.
- `layout()` walks the **visible** children: along the main axis it sets each child's `x`
  (Row) or `y` (Column) to a running cursor (`padding` + Σ(extent + spacing)); the cross-axis
  coordinate is `padding`. It then auto-sizes the container — main axis to the content extent,
  cross axis to the largest child + `2·padding`; with no visible children both axes collapse to
  `2·padding`.
- `advance(nowMs)` calls `layout()` then `Segment::advance`. It positions from each child's
  **current** (already-animated) extent, so when a child's size/visibility animates the layout
  follows smoothly frame-to-frame; only a child insert/remove reflows in one frame (a layout-level
  insert/remove animation is out of scope — FR-25 governs a control's own visible state, not
  container membership churn).

## 2g. `VisualLoop` (FR-34)

The authorable base for indeterminate looping visuals. State: `mRunning`, `mStartMs`, `mNowMs`,
`mCycleMs`, `mCycles`.

- `start(nowMs)` is guarded on `mRunning` (idempotent), stamps `mStartMs`/`mNowMs`, zeroes the cycle
  counter, then fires `onLoopStart()` — so `now()` is already valid inside the signal.
- `advance(nowMs)` stamps `mNowMs`, and while running with `mCycleMs > 0` computes
  `completed = floor((nowMs - mStartMs) / mCycleMs)` and fires `onCycle(++mCycles)` in a loop until
  `mCycles == completed`. The loop (rather than a single "did it wrap" test) is what makes a long
  frame emit one signal per elapsed cycle instead of collapsing them.
- `cyclePhase()` returns the fractional part of `elapsed / mCycleMs`; elapsed is never negative
  while running, so a truncating cast is the correct floor.
- Constructor sets `inputTransparent = true`.

## 2h. `ProgressIndicator` (FR-35)

The determinate counterpart. State: `mValue`, `mIndeterminate`, `mCompleted` (the `onComplete`
latch), `mPhase`, `mPeriodMs`, `mOmega`, and a `Spring mDisplay`.

- `setValue(v)` clamps, retargets the spring, fires `onValueChanged`, then runs the completion
  latch: fire `onComplete()` on the `< 1 -> 1` edge, and clear the latch whenever the value is below
  `1` so a reused indicator can complete again.
- `advance(nowMs)` derives `dt` from the previous timestamp, advances the spring at `mOmega`, and —
  only while indeterminate with `mPeriodMs > 0` — advances `mPhase` by `dt*1000/mPeriodMs` and wraps
  it into `[0,1)`.
- `hitTestSelf` returns false; the constructor sets `inputTransparent`.

`ProgressBar` keeps only `ProgressStyle` + `mShuttle` and paints: track, then either the
`displayValue()` fill or (indeterminate) a shuttle of width `w*mShuttle` at
`x = phase*(w+sw) - sw`, drawn inside a `clipRect` of the track so it enters and exits behind the
ends.

## 2i. Signal hooks (FR-36)

Protected virtuals fired from the state transitions themselves, never from the render path:

- `Segment::updateHoverAnim` fires `onHoverChanged(h)` on the same edge that starts the hover tween.
- `Segment::requestFocus` fires `onFocusChanged(false)` on the segment it displaces and
  `onFocusChanged(true)` on itself only when focus was actually gained.
  `clearFocusRegistration(bool notify)` fires `onFocusChanged(false)`; the destructor passes
  `notify = false` so no virtual is dispatched on a dying object.
- `Button` fires `onPressDown` on `Down`, `onRelease` + `onClicked` on a `Click` that completes a
  press, `onCancel` on a `Drop` that abandons one, and `onClicked` on keyboard confirm.
- `Slider::notifyChange()` is the single funnel: `onValueChanged(value())` then the public
  `onChange`. Every mutating path calls it. `mDragging` gives `onDragStart`/`onDragEnd` their edges.
- `Checkbox::toggle()` fires `onCheckedChanged`; `setChecked()` deliberately does not.

## 2j. `PathSegment` (FR-37)

`Path` gains `emit(t)` (ops + `applyPaint`, no graphics-state changes), `clear()`, and
`segmentCount()`. `Path::onDraw` becomes a one-line call to `emit`, and `PathSegment::onPaint` is
the same one-line call — the segment's `render()` has already installed the world transform, so the
path's local coordinates land in segment space with no second transform path to keep in sync.

## 2k. `TextBox` caret (FR-38)

State: `mCaret_` (a byte offset into `text`, always on a codepoint boundary) and `mMeasure`
(the last `IRenderTarget` seen by `render`, so caret placement can use the adapter's own
metrics instead of the estimate).

- `setCaret(b)` clamps into `[0, size]` and then walks left off any UTF-8 continuation byte,
  so no operation can ever leave the caret mid-codepoint. `stepLeft`/`stepRight` move by a
  whole codepoint.
- `handleKey` clamps first (`text` may have been assigned from outside since the last key),
  then handles Left/Right/Home/End **before** the `readOnly` check — inspection is not a
  mutation — and insert/Backspace/Delete after it. Backspace deletes `[stepLeft(caret),
  caret)`; Delete deletes `[caret, stepRight(caret))`.
- `handleGesture` on `Down` scans the codepoint boundaries and picks the one whose prefix
  width is nearest the pointer, using `textWidthTo` (target metrics when available, the
  estimate otherwise).
- `syncVisuals` draws the caret at `padding + textWidthTo(mCaret_)` rather than at the end
  of the string; it still fades with the focus factor.

## 2l. Text fitting and disabled state (FR-39, FR-40)

- `TextBox` sets `clipToBounds` in its constructor (the backstop) and keeps a `mScrollX`
  text offset. `syncVisuals` computes `caretX = textWidthTo(mCaret_)` and shifts `mScrollX`
  by the MINIMUM amount that brings the caret back inside `[0, width - 2*padding]`, then
  clamps it to `[0, fullWidth - visible]` so the field never scrolls past the end of the
  text. The label draws at `padding - mScrollX` and the caret at `padding + caretX -
  mScrollX`, so both stay in one coordinate frame.
- `ComboBox::fitText` shortens the selected label to the space between the left padding and
  the caret triangle, trimming whole UTF-8 codepoints and appending an ellipsis; zero or
  negative space draws nothing rather than overflowing.
- `Segment::updateDisabledAnim` mirrors `updateHoverAnim`: it eases `mDisabledAmount` on
  every `enabled` edge, so `disabledAmount()` is a smooth `[0,1]` factor. `Interaction.h`
  gains `dimColor`/`dimPaint`/`dimBox`, which drop `kDisabledFade` of a colour's alpha; every
  themed control multiplies its own style through them. One mechanism, so a disabled Button
  and a disabled Slider read the same on any theme.

## 2m. `drawsBuiltInVisuals` (FR-41)

A `Segment` flag, default true. Controls composed from child visual nodes (`Button`,
`Checkbox`, `Slider`, `TextBox`) set those children's `visible` from it at the top of
`syncVisuals` and return early; self-drawn controls (`ToggleSwitch`, `ComboBox`,
`ProgressBar`, `LineGraph`, and `Slider`'s gradient track) return early from `onPaint`.
Nothing on the behaviour paths reads it, so hit testing, gestures, value/press/check state
and the FR-36 signals are untouched — which is exactly the separation that lets a generated
subclass keep a control's behaviour and supply its own picture.

## 2n. `Path` trim (FR-42)

`Path::Piece` is one drawable span reduced to a line or a cubic — quadratics are raised to
cubics on the way in, so the splitter has two cases instead of three, and a `close()` becomes
the line back to the subpath's start, which is what makes a closed shape trim as one
continuous run instead of stopping at the seam.

- `measure()` samples a cubic at `kArcSamples` (32) points and accumulates chord lengths,
  keeping the cumulative marks; `paramAt(d)` then locates a distance by scanning those marks
  and interpolating within the bracketing pair. Sampling is why `length()` is an
  approximation — and it is the right one, because a trim needs *arc length*, which has no
  closed form for a cubic.
- `slice(t0, t1)` splits by de Casteljau twice — at `t1` to keep the head, then at the
  rescaled `t0` to drop the tail — so a trimmed curve follows the ORIGINAL curve rather than a
  polyline through it.
- `trimmed()` normalises the range first: a span of a full turn or more becomes the whole
  path; otherwise both ends shift by `floor(start + offset)` so the pair lands in `[0,1)`
  together. A range that then runs past `1` is emitted as two runs — tail, then head — which
  is what lets a spinner's arc cross the seam.
- `trimmedRange()` walks the pieces, skips those wholly outside, slices the partial ones, and
  emits `moveTo` once. It deliberately does not re-apply `close()`: a trim is a cut.

`ellipsePath()` / `roundedRectPath()` hold the outlines that `drawCircle` / `drawRoundedRect`
emit, and those two now build and emit a `Path` rather than duplicating the geometry — so the
shape a trim operates on is the same shape that gets drawn. `CircleSegment`,
`RectangleSegment` and `PathSegment` each carry a `Trim` and apply it in `onPaint`.

## 2o. Ellipse sector (FR-43)

`Trim` and `Arc` answer different questions and are deliberately separate types: `Trim` is
"how much of the OUTLINE is drawn" (a stroke that draws itself in); `Arc` is "which part of
the DISK this is" (a pie, a ring, a pac-man). `CircleSegment` applies `Arc` first to pick the
geometry, then `Trim` to that geometry's outline, so the two compose rather than compete.

`appendArc` emits the elliptical arc as cubics of at most 90 degrees each, with control points
at `4/3*tan(delta/4)` along the parametric tangents `(-rx*sin(t), ry*cos(t))`. That is the
standard approximation and it is accurate at any radius, which is why an arc needs no HAL
primitive.

`ellipseArcPath` then has three shapes:
- **full sweep** — the plain ellipse; with `innerRatio > 0`, an annulus whose inner contour is
  wound the OTHER way, so nonzero winding leaves the hole empty;
- **pie** (`innerRatio == 0`) — centre, ray out, arc, close. The two straight edges are the
  rays that make a pac-man's mouth;
- **ring segment** — outer arc, across, inner arc back, close.

A zero sweep or a zero radius returns an empty path rather than a degenerate one.

## 2p. `TextBox` selection, clipboard, blink (FR-44)

### Selection

The field holds `mAnchor` beside `mCaret_`; the selection is `[min, max)` of the two and is
empty when they coincide. Every mutation routes through two helpers so no path can forget the
selection: `deleteSelection()` (erase the range, caret and anchor to its start) and
`insertText()` (delete any selection, then insert at the caret). Typing, paste, Backspace and
Delete are all written in terms of them.

`setSelection(anchor, caret)` clamps both onto codepoint boundaries via the same `setCaret`
walk, so no operation can leave either end mid-glyph.

Arrow keys distinguish two cases deliberately: with **shift** they move the caret and leave the
anchor, extending; **without** shift and with a selection present they COLLAPSE to the
corresponding edge rather than moving from the caret — which is what makes pressing Left after
a drag land at the selection's start rather than one character in from wherever the drag ended.

`wordLeft`/`wordRight` skip a run of separators and then a run of word characters (letters,
digits, `_`), which gives Ctrl+Arrow, Ctrl+Backspace and Ctrl+Delete one shared definition of
"word". `wordAt` returns the run under a byte offset — a run of word characters, or the run of
separators if the offset is in whitespace — and is what a double-click selects.

### Clipboard

`Clipboard` holds a `Reader`/`Writer` pair in a function-local static. The default pair reads
and writes an in-process string, so copy/paste works in tests and headless builds with no host
involvement; `install()` swaps in the real one and `reset()` restores the default (which the
tests use to stay isolated from each other). `copy()`/`cut()` no-op on an empty selection;
`cut()` and `paste()` are refused while `readOnly`, but `copy()` is not — reading is not
mutation, the same rule FR-38 applies to caret movement.

### Caret

`mBlinkT0` is stamped by `resetBlink()`, which every caret move and every edit calls, so the
caret is always solid at the instant it moves rather than possibly mid-dark-phase. Visibility is
`fmod(now - t0, 2*kBlinkMs) < kBlinkMs`, and under `reducedMotion()` it is simply true — a
blinking caret is motion, and the accessibility switch turns motion off rather than speeding it
up. The blink multiplies the focus factor (FR-38) rather than replacing it, so a caret still
fades in with focus and then begins to blink.

Geometry: 1px wide, `1.25 x` the text size tall, vertically centred — a text cursor rather than
the earlier block.

### Visual tree

`box -> selection -> label -> caret`, so the highlight sits behind the glyphs and the caret in
front of them. The selection node is one rectangle (the field is single-line) spanning
`textWidthTo(start)..textWidthTo(end)` in the same scrolled frame as the label, and is hidden
when the selection is empty or the field is unfocused.

## 3. `InputController`

`InputController` is an abstract behavior strategy.

### Responsibilities

- Consume gestures for a segment.
- Consume keyboard events for a segment.

### Intent

This supports reusable behavior injection when behavior should be shared across multiple segment
types without creating a deep inheritance chain.

## 3a. `GestureRecognizer` touch extensions (FR-28)

### `touch` carry-through

`RawPointer` gains `bool touch = false`; `GestureRecognizer::feed` records it (`mTouch`,
alongside the existing `mAlt`/`mShift`/`mCtrl`) and `emit()` carries it onto every synthesized
`Gesture`, exactly like the existing modifier flags.

### Long press

- New state: `mDownTimeMs`, `mLongPressFired`, `mLongPressMs` (default 500, `setLongPressMs`).
- `Down` resets `mLongPressFired = false` and records `mDownTimeMs = e.timeMs`.
- `advance(nowMs)`: if `mPressed && !mDragging && !mLongPressFired && (nowMs - mDownTimeMs) >=
  mLongPressMs`, emits `Gesture::Type::LongPress` at `mDownPos` and sets `mLongPressFired = true`
  (fires at most once per press). A no-op otherwise (not pressed, already dragging — a drag has
  its own semantics — or already fired).
- `Up`: if `mLongPressFired`, the existing `Click`/`DoubleClick` branch is skipped (only `Up` is
  emitted) — a press that long-pressed does not also register as a regular tap.

### Fling (velocity tracking)

- New state: `mVelocitySamples` (a `std::vector<{timeMs, pos}>`), `mVelocityWindowMs` (default
  100, `setVelocityWindowMs`), `mFlingThreshold` (default 400 px/s, `setFlingVelocityThreshold`).
- `Down` clears `mVelocitySamples`.
- `Move` while `mDragging`: after emitting `Drag`, appends `{e.timeMs, e.pos}` to
  `mVelocitySamples` and prunes samples older than `e.timeMs - mVelocityWindowMs`.
- `Up` while `mDragging` (before the existing `Drop` emission's sample state is cleared): if
  `mVelocitySamples` has at least one remaining sample and the elapsed time to it is > 0,
  computes `velocity = (upPos - oldestSample.pos) / elapsedSeconds`; if
  `hypot(velocity.x, velocity.y) > mFlingThreshold`, emits `Gesture::Type::Fling` with that
  `velocity`, **after** the existing `Drop` emission (additive — every existing `Drop` consumer
  is unaffected; a kinetic control also listens for `Fling`).

### Touch hover suppression (FR-24 addendum)

`Segment::dispatchGesture`'s `Move` case still routes a touch-flagged bare `Move` to the deepest
hit-tested handler (unchanged hit-testing), but skips the `setHovered(this)` call when
`g.touch == true` — a touchscreen has no ambient "resting over" state, so a touch drag must not
leave a control looking permanently hovered once the finger lifts. `LongPress` and `Fling` both
route with `DragStart`/`Drag`/`Up`/`Drop` (to the currently captured child if any, else falling
through to this segment's own `handleGesture` — the same path `Drag` already takes when there is
no capture, e.g. a `ScrollView` driving its own drag/fling directly). `Fling` is a continuation of
the drag that just ended, not a fresh interaction, so it must reach the same target `Drop` did —
routing it like `Click` (a fresh hit-test) would let an unrelated child under the release point
absorb it instead of the segment that was actually being dragged (e.g. a `ScrollView`'s own
content child, rather than the `ScrollView`).

## 4. Visual Segment Primitives

### 4.1 `RectangleSegment`

- Draws a rectangle using a `BoxStyle`.
- Uses the segment bounds as its local geometry.

### 4.2 `CircleSegment`

- Draws a circle or ellipse from the segment bounds.
- Uses Bezier approximation so it stays backend-neutral.

### 4.3 `LabelSegment`

- Draws a text string with a `TextStyle`.
- Does not participate in hit-testing by default.

## 5. `Theme`

`Theme` is a concrete style bundle.

### Responsibilities

- Provide baseline styles for the standard controls.
- Keep appearance data separate from control behavior.

### Current baseline theme

- Dark neutral surfaces.
- Warm accent color for active state and caret.
- Shared outline color for control framing.

## 5a. `ui::interaction` / `Interaction.h` (FR-24)

Header-only, platform-free helpers that define the **one** hover treatment reused by every control,
plus general colour/paint interpolation. Keeping it in a single place is the interaction
"consistency lock": hover reads identically framework-wide and needs no per-control theme fields.

### Constants (`namespace interaction`)

- `kHoverMs = 120` — hover fade in/out duration.
- `kHoverFillLift = 0.14` — fraction a fill brightens toward white at full hover.
- `kHoverStrokeLift = 0.5` — fraction a border is pulled toward the emphasis colour at full hover.

### Free functions

- `lerpColor(a, b, t)`, `brighten(c, amt)` — colour maths (alpha preserved by `brighten`).
- `lerpPaint(a, b, t)`, `lerpBox(a, b, t)` — interpolate a whole `Paint` / `BoxStyle`.
- `hoverBox(base, emphasis, t)` — the standard hover appearance for a box: brighten the fill and
  pull the stroke toward `emphasis`, scaled by `t` (the control's `hoverAmount()`).

## 6. `AbstractSlider`

`AbstractSlider` owns slider semantics only.

### State

- `value`
- `minimum`
- `maximum`
- `defaultValue` — snap-back target; defaults to the initial value
- `isAnalog`
- `dragType`

### Rules

- Value is always clamped to the current range.
- Normalized value is derived from current value and range.
- `defaultValue` is clamped to the range, and re-clamped when `setRange` changes the range.
- `resetToDefault()` sets the value to `defaultValue`; concrete controls call it on a
  double-click and then emit `onChange` (see FR-9a).

## 7. `Slider`

`Slider` is a concrete `Segment` plus `AbstractSlider`.

### Internal composition

- `RectangleSegment mTrack`
- `RectangleSegment mRangeFill`
- `CircleSegment mThumb`

### Behavior

- Pointer down, drag, and click convert local x-position into a slider value.
- `advance(nowMs)` springs the *displayed* thumb/fill toward the value via a shared `anim::Spring`
  (FR-4d, `omega≈18`, same follower as `Knob`); `displayValue()` exposes the smoothed value.
- **Hover (FR-24):** the thumb radius grows by `2·hoverAmount()` and its style is
  `hoverBox(thumb, rangeFill.fill, hoverAmount())` (brighten + border toward the accent).
- Left and right arrow keys decrement or increment the value.
- Each of these (and the double-click reset) fires `onChange(value())` with the resulting
  value; a programmatic `setValue()` does **not** fire `onChange` (so syncing controls to
  state doesn't recurse) — same contract as `Knob`.
- `setClickJumps(bool)` (default true): when false, only `Drag`/`DragStart` set the value; a
  `Down`/`Click` is captured (returns true so the following drag is delivered) but does not
  jump the value to the cursor. This keeps a double-click from being preceded by a value-
  changing click, so `DoubleClick` reliably resets to default (FR-6/FR-9a).
- With `clickJumps` on, a `Click` records a **pending** jump (`mPendingClick`, `mPendingValue`)
  rather than moving the value; `advance()` commits it once `nowMs - mPendingSince >=
  mClickGuardMs`. `mClickGuardMs` (300ms) is set `>=` the recognizer's double-click window so
  the jump can't commit between the two clicks of a slow double-click. The pending jump is
  cleared by a `DoubleClick` (which then resets to default) **and** by the next `Down` (the
  second click's press), so the reset is flash-free at any double-click speed.

### Rendering sequence

1. Ensure the internal child tree exists.
2. Synchronize child geometry from current segment bounds and slider value.
   `mRangeFill`'s span is anchored at the zero-crossing (`clamp(0, minimum(), maximum())`
   normalized) when the range spans zero, so it grows from neutral rather than from the
   left edge; otherwise it fills from the left edge as always.
3. Render the segment tree.

## 8. `Button`

### Internal composition

- `RectangleSegment mBody`
- `LabelSegment mLabel`

### Behavior

- `Down` sets the pressed state and animates `mPress` toward `1`.
- `Click`/`Drop` clears it and animates `mPress` toward `0`; `Click` invokes `onClick`.
- `Enter` or `Space` invokes `onClick`.
- **Motion/hover (FR-24/FR-25):** `advance()` ticks `mPress`; the body style is
  `lerpBox(idle, pressed, blend)` where `blend = press + (1-press)·0.4·hoverAmount()` — so the
  press crossfades and hover nudges the body ~40% toward the themed pressed (accent) look. Nothing
  snaps.

## 9. `Checkbox`

### Internal composition

- `RectangleSegment mBox`
- `RectangleSegment mIndicator`
- `LabelSegment mLabel`

### Behavior

- `Click`, `Enter`, or `Space` toggles the boolean state and animates `mCheck` (0↔1).
- The indicator **grows in/out** from the centre (size scaled by `mCheck`); it is only `visible`
  while `mCheck > 0.001`, so it never pops. `setChecked()` snaps `mCheck` (programmatic).
- **Hover (FR-24):** the box uses `hoverBox(box, accent, hoverAmount())` — brighten + border toward
  the accent (indicator fill).

## 10. `TextBox`

### Internal composition

- `RectangleSegment mBox`
- `LabelSegment mLabel`
- `RectangleSegment mCaret`

### Behavior

- `Down` requests focus.
- `KeyEvent::Text` appends entered text.
- `Backspace` removes one character.
- Read-only mode disables editing.
- **Motion/hover (FR-24/FR-25):** `advance()` eases `mFocusAmt` (0↔1) on focus change; the box is
  `hoverBox(lerpBox(idle, focused, focusAmt), accent, hoverAmount())` (focus border blends, hover
  brightens) and the caret's alpha is scaled by `focusAmt` (fades in on focus, out on blur — no pop).

### Current simplifications

- Caret x-position uses estimated text width.
- Selection, cursor movement, and clipboard behavior are not implemented yet.

## 11. Clipping

### HAL

- `IRenderTarget::clipRect(x, y, w, h)` intersects the current clip with the rectangle in the
  current transform space; it is scoped by `save()`/`restore()`.
- `RecordingTarget` records it as `DrawOp::Kind::ClipRect` with `args[0..3] = x,y,w,h`.
- `Canvas2DTarget` maps it to `ctx.beginPath(); ctx.rect(...); ctx.clip()`.
- `CairoTarget` maps it to `cairo_rectangle(...); cairo_clip()`.

## 11a. Path-clip primitive

### HAL

- `IRenderTarget::clipPath()` intersects the current clip with the **current path** (nonzero
  winding), in the current transform space, scoped by `save()`/`restore()`. Unlike `clipRect`,
  which builds its own internal rectangle path, `clipPath()` clips using whatever path the caller
  already built via `beginPath`/`moveTo`/`lineTo`/`quadTo`/`cubicTo`/`closePath` — the same
  "current path" `fillPath`/`strokePath` paint (FR-16). The caller builds the path, then calls
  `clipPath()` instead of `fillPath()`/`strokePath()`.
- `RecordingTarget` records `DrawOp::Kind::ClipPath` with no extra fields — the preceding
  `MoveTo`/`LineTo`/`QuadTo`/`CubicTo`/`ClosePath` ops already in the stream capture the clipped
  shape, so a test asserts the whole op sequence rather than parameters on the clip op itself.
- `CairoTarget::clipPath()` sets the fill rule to `CAIRO_FILL_RULE_WINDING` (nonzero — already
  Cairo's default, set explicitly so the primitive's contract does not depend on no other call
  ever changing it) and calls `cairo_clip()`. Cairo clears the current path as a side effect of
  `cairo_clip()` (the non-`_preserve` variant, mirroring `cairo_fill`/`cairo_stroke`), which is
  exactly the "fresh path" postcondition the primitive requires — no extra call needed.
- `Canvas2DTarget::clipPath()` calls `ctx.clip()` (default nonzero winding) using whatever path is
  already built on the context, **then an explicit trailing `ctx.beginPath()`** — Canvas2D's
  `clip()` does *not* clear the current path the way Cairo's does, so without the extra call the
  two adapters would leave different path state after an identical primitive call. This is the
  platform-independence trade-off rule (§3 of the skill / NFR-1): identical behavior wins, and the
  (trivial) extra cost lands on the adapter that needs it.

## 11b. Radial-gradient fill

### HAL

- `IRenderTarget::setRadialFill(cx, cy, radius, inner, outer)` sets the current fill to a two-stop
  radial gradient (centre `inner` → edge `outer` at `radius`, current transform space). The next
  `fillPath()` paints with it; it is superseded by the next `setFill`/`setRadialFill`.
- `RecordingTarget` records `DrawOp::Kind::SetRadialFill` with `args[0..2] = cx,cy,radius`,
  `color = inner`, and a new `color2 = outer`.
- `Canvas2DTarget`: `g = ctx.createRadialGradient(cx,cy,0, cx,cy,radius)` + two colour stops →
  `ctx.fillStyle = g`.
- `CairoTarget`: `cairo_pattern_create_radial(cx,cy,0, cx,cy,radius)` + two stops →
  `cairo_set_source`.
- Rationale: a smooth gradient to zero opacity (a soft glow) cannot be expressed by solid fills;
  stacking translucent shapes only approximates it and bands. This is the minimal paint-server
  primitive (two stops, radial) needed for glows; richer gradients can extend it later (OCP).
- `IRenderTarget::setLinearFill(x0, y0, x1, y1, start, end)` sets the current fill to a two-stop
  linear gradient along the axis `(x0,y0)→(x1,y1)` (current transform space; constant
  perpendicular to the axis). The next `fillPath()` paints with it; superseded by the next
  `setFill`/`setRadialFill`/`setLinearFill`.
- `RecordingTarget` records `DrawOp::Kind::SetLinearFill` with `args[0..3] = x0,y0,x1,y1`,
  `color = start`, `color2 = end`.
- `Canvas2DTarget`: `g = ctx.createLinearGradient(x0,y0,x1,y1)` + two colour stops →
  `ctx.fillStyle = g`. `CairoTarget`: `cairo_pattern_create_linear(x0,y0,x1,y1)` + two stops →
  `cairo_set_source`.
- Rationale: same as the radial fill — a smooth ramp cannot be built from solid fills without
  banding. Linear is the second canonical gradient (depth/shading ramps); it reuses
  `DrawOp::color2`.

## 11c. Raster image primitive

### HAL

- `IRenderTarget::registerImage(rgba, w, h) -> int` uploads pixels (straight RGBA8, top-down,
  stride `w*4`, sRGB) and returns a positive handle (0 = failure). `updateImage(id, rgba, w, h)`
  replaces a handle's pixels/dimensions. `drawImage(id, dst)` blits the handle into the `Rect`
  `dst` in the current transform space. `releaseImage(id)` frees it.
- Rationale (handle model, not immediate-mode): a photo editor redraws at frame rate while the
  pixels change only on an edit. Re-uploading megabytes per frame would be wasteful, so pixels are
  uploaded once and re-sent only on `updateImage`. Per the platform trade-off rule the cost lives
  on the adapter (one surface/canvas per handle + the RGBA8→native conversion); the visible result
  is identical everywhere.
- `RecordingTarget` records `DrawOp::Kind::{RegisterImage,UpdateImage,DrawImage,ReleaseImage}`:
  register/update store `imageId`, `imgW`, `imgH`, and a position-weighted `pixelHash` (so a test
  can prove an update changed the bytes without storing them); draw stores `imageId` and the dst
  rect in `args[0..3]`; an internal `mNextImageId` assigns handles.
- `Canvas2DTarget` keeps a JS registry (`window.__abimg.map[id]`) of offscreen `<canvas>` elements;
  `ImageData` is straight RGBA8 top-down so the bytes copy in directly (`HEAPU8.subarray` →
  `putImageData`); `drawImage` calls `ctx.drawImage(canvas, x,y,w,h)`, honoring the current transform.
- `CairoTarget` keeps an `unordered_map<int, ImageEntry>` of ARGB32 surfaces. It converts straight
  RGBA8 → **premultiplied BGRA** (little-endian ARGB32 byte order) honoring
  `cairo_format_stride_for_width`, and keeps the owning byte buffer alive alongside the surface
  (`cairo_image_surface_create_for_data` does not copy). `drawImage` does
  `save → translate(dst) → scale(dst/size) → set_source_surface → paint → restore`. The destructor
  destroys all surfaces.

### Segment wiring

- `Segment::render` is unchanged for non-clipping segments (same op stream). When
  `clipToBounds == true`, children are rendered inside an extra `save()` → `setTransform(world)`
  → `clipRect(localBounds)` → … → `restore()` bracket, so the clip applies to the whole subtree
  and is released afterwards.

## 11d. Text font family + letter-spacing

### HAL

- `IRenderTarget::drawText(text, x, y, sizePx, fontFamily = "", letterSpacingPx = 0.0)` — the
  two new trailing parameters default to the prior behavior, so every existing 4-argument call
  site is source- and binary-compatible (only `RecordingTarget`, `CairoTarget`, and
  `Canvas2DTarget` implement `IRenderTarget` and needed updating; every other call site in the
  codebase only *calls* `drawText`).
- `RecordingTarget` records `DrawOp::fontFamily` (string) and `DrawOp::letterSpacingPx`
  (double) alongside the existing `text`/`args[0..2]` (x, y, sizePx).
- `CairoTarget`: `cairo_select_font_face(family.empty() ? "Sans" : family, NORMAL, NORMAL)`.
  When `letterSpacingPx == 0`, one `cairo_show_text` call (fast path, unchanged from before).
  When non-zero, iterates the UTF-8 string one codepoint at a time (`utf8SeqLen`, an anonymous-
  namespace helper reading the leading byte's high bits), drawing each codepoint with
  `cairo_show_text` and stepping the pen by `cairo_text_extents(...).x_advance +
  letterSpacingPx`.
- `Canvas2DTarget`: builds `ctx.font = "<sizePx>px \"<family>\", sans-serif"` (or plain
  `sans-serif` when `family` is empty) and sets `ctx.letterSpacing = "<letterSpacingPx>px"`
  when the browser supports the property (`'letterSpacing' in ctx`).
- Rationale: font-family resolution is inherently adapter/OS text-stack territory (glyph
  outlines cannot be composed from `beginPath`/`fillPath`), so it stays a parameter on the
  existing text primitive. It does not load font files — an application that wants a custom
  bundled font registers it with the OS font system itself (e.g. `FcConfigAppFontAddFile` on
  Linux) before the family name is passed in; the HAL only asks the adapter's text stack to
  resolve whatever name it's given.
- **`measureText(text, sizePx, fontFamily, letterSpacingPx)`** — the read side of
  `drawText`, for laying out / right- and centre-aligning text without clipping (e.g. placing
  the accent "." right after the "cosmo" wordmark, or right-aligning a slider's value). It is a
  HAL primitive for the same reason `drawText` is: advance width needs the font. `IRenderTarget`
  provides a concrete **default** — a headless estimate (~0.5em per UTF-8 codepoint +
  letterSpacing between glyphs) used by `RecordingTarget` and any target with no font engine, so
  it is not a pure method and does not break existing targets. `CairoTarget` overrides it with
  `cairo_text_extents` (single-run advance when untracked; per-codepoint sum + spacing otherwise,
  mirroring its `drawText` pen), and `Canvas2DTarget` with `ctx.measureText` (+ manual tracking
  when the browser lacks `letterSpacing`). Tested headless in `coreTests`
  (`RenderTarget_measureText_headless_estimate`).
- `CairoTarget` font resolution on fontconfig-free hosts (Android): the toy
  `cairo_select_font_face` path above requires fontconfig to map a family name to a face, which
  the Android Cairo build omits. Under the `ARTBOARD_CAIRO_FT` compile flag (Android only),
  `CairoTarget` gains a static `registerFontFile(family, ttfPath)` that builds a
  `cairo_ft_font_face_create_for_ft_face` from a FreeType face and caches it by family; `drawText`
  then prefers a registered face via `cairo_set_font_face`, falling back to `cairo_select_font_face`
  for any unregistered family. This is an adapter-internal, opt-in addition — it does **not** touch
  the `IRenderTarget` HAL, so no other adapter changes, and the desktop/GTK build (flag off) is
  byte-for-byte the prior toy-API path. The host registers each bundled family once at startup
  (the Android host extracts the DM Sans / JetBrains Mono TTFs from APK assets and calls
  `registerFontFile`), mirroring what `FcConfigAppFontAddFile` does on Linux.

### Propagation to `ui::TextStyle`

- `TextStyle` (`ui/base/Theme.h`) gained `fontFamily`/`letterSpacingPx` fields alongside
  `color`/`sizePx`. `LabelSegment::onPaint` forwards them to `drawText`, so `Button`,
  `Checkbox`, and `TextBox` (all label via `LabelSegment`) pick up a themed family/tracking for
  free. `TabView`, `ComboBox`, and `Knob` draw their own text directly (not through
  `LabelSegment`) and were each updated to forward their relevant `TextStyle`'s two new fields
  to their `drawText` call.
- `scene::Text` (the freeform drawable) gained the same two fields for parity with `TextStyle`,
  forwarded in `Text::onDraw`.

## 11e. Opacity layer (group compositing)

### HAL

- `IRenderTarget::pushLayer(alpha)` / `popLayer()` bracket a region of drawing that must be
  composited as one unit at `alpha` — the primitive a panel/card fade-in needs when its content
  has overlapping shapes (a group fade), which per-shape alpha cannot express without double-
  blending the overlap. `popLayer()` takes no arguments; the adapter remembers the alpha its
  matching `pushLayer` was given.
- `RecordingTarget` records `DrawOp::Kind::PushLayer` (`args[0] = alpha`) and `DrawOp::Kind::
  PopLayer` (no fields) — the ops in between are whatever the caller recorded, so a test asserts
  the whole bracketed sequence the same way it asserts a `clipPath()` sequence (§11a).
- `CairoTarget::pushLayer` keeps a `std::vector<double> mLayerAlphas` (LIFO) and calls
  `cairo_push_group()`. `popLayer()` pops the remembered alpha, calls
  `cairo_pop_group_to_source()` (which composites all drawing since the matching push into a
  pattern and installs it as the current source — restoring every other piece of graphics state
  to what it was before the push, since `cairo_push_group`/`cairo_pop_group` already bracket an
  implicit `cairo_save`/`cairo_restore` pair), then `cairo_paint_with_alpha(alpha)` to blend that
  pattern into the destination. An unbalanced `popLayer()` (empty `mLayerAlphas`) is a no-op guard,
  matching the codebase's existing style of tolerating calls on unknown/absent handles (e.g.
  `releaseImage`) rather than asserting.
- `Canvas2DTarget` has no native group-compositing call, so it builds the same contract from an
  offscreen `<canvas>` stack (`window.__abLayerStack`), reusing the existing pattern where every
  draw primitive already reads the mutable `window.__abctx` fresh on each call:
  - `pushLayer(alpha)`: create an offscreen canvas the same pixel size as the current
    `window.__abctx.canvas`, copy the current transform (`getTransform()`/`setTransform()`) and
    paint state (`fillStyle`/`strokeStyle`/`lineWidth`/`font`) into its 2D context so drawing
    inside the layer positions and paints exactly as it would on the destination, push
    `{ctx: <previous __abctx>, alpha}` onto the stack, then reassign `window.__abctx` to the new
    offscreen context. Every existing `ab_*` drawing function needs **no changes** — they all
    already read `window.__abctx` fresh, so redirecting that one binding redirects every
    primitive automatically.
  - `popLayer()`: pop the stack frame, restore `window.__abctx` to the destination context, then
    `dest.save(); dest.setTransform(identity); dest.globalAlpha = alpha; dest.drawImage(layerCanvas,
    0, 0); dest.restore();` — an identity transform during the composite blit because the layer
    canvas's pixels are already positioned in device space (its transform matched the
    destination's at push time); applying the destination's current transform again would
    transform it twice. The destination's own active clip (untouched throughout, since all layer
    drawing happened on the separate offscreen context) still applies to this `drawImage`, so
    content is correctly constrained by whatever clip was active before the `pushLayer` — matching
    Cairo's behavior of resuming the pre-push clip on `cairo_pop_group_to_source`'s implicit
    restore.
  - An unbalanced `popLayer()` (empty stack) is a no-op guard, same style as the Cairo adapter.
- Nesting: both adapters generalize to arbitrary nesting for free — Cairo's group stack is
  internal to the library; the Canvas2D stack's `prev = window.__abctx` captures whichever layer
  (or the real destination) was active, so a nested `pushLayer` layers correctly on top.

## 11f. Elevation shadow (`drawShadow` / `drawElevation`)

### Geometry

Given a shadow rect `(sx,sy,sw,sh)` (the object's rect shifted by `offsetX/offsetY`), corner
radius `r` (clamped to half the smaller side, exactly like `drawRoundedRect`), and blur `b`:

- **Four edge strips** — plain rectangles between the two adjacent corners on each side (skipped
  if their span would be non-positive, e.g. a very small rect), filled with `setLinearFill`
  perpendicular to that edge: full `color` alpha at the shadow rect's own edge, fading to a
  transparent copy of `color` at distance `b` beyond it.
- **Four corner wedges** — a "kite" path per corner (`moveTo` the corner's arc centre `C`,
  `lineTo` the point at radius `r+b` in the pure horizontal direction, `quadTo` (control point at
  the outward-shifted sharp corner, mirroring how `drawRoundedRect` itself approximates a corner
  with one quadratic bezier rather than a true arc) to the point at radius `r+b` in the pure
  vertical direction, `lineTo` back to `C`, close), filled with `setRadialFill(C, r+b, color,
  transparent)`.
- Both `color`'s own alpha (as the peak) and a zero-alpha copy of the same RGB are used as the two
  gradient stops, so the shadow fades to fully transparent rather than to an opaque background
  color.

### Known limitation

The corner wedge's radial gradient measures distance from the arc **centre** `C` (the only
gradient the HAL offers), not distance from the arc itself; at the shared seam with an edge strip
(exactly at the object's straight edge, distance `r` from `C`), the wedge's colour is already
partially faded (`lerp` fraction `r/(r+b)`) rather than full peak, while the strip is at full peak
at that same point. This is a real, minor continuity gap between the two gradient shapes —
unavoidable without a HAL-level annulus/multi-stop gradient (out of scope; FR-30 composes only
from the existing 2-stop primitives) — documented in `requirements.md` §5. It is hidden under the
object's own opaque fill (drawn by the caller afterward) for the region actually inside the
corner radius, and only mildly softens the visible fade right at the tangent point.

### `drawElevation`

A convenience wrapper: two `drawShadow` calls scaled by `elevationDp` — a tighter, more opaque
"key" layer (`offsetY = elevationDp*0.5`, `blurPx = elevationDp*1.0`, alpha `0.30`) and a softer,
lighter "ambient" layer (`offsetY = elevationDp*0.25`, `blurPx = elevationDp*2.0`, alpha `0.15`),
both black, matching Material's two-shadow elevation convention without claiming exact parity
with Material's published elevation tables.

## 12. Extended widgets (`ui/concrete/`)

File layout: the `ui` module is one class per file, split into `ui/base/` (foundations +
`AbstractSlider` + the reusable `RectangleSegment`/`CircleSegment`/`LabelSegment`) and
`ui/concrete/` (the finished controls — `Button`, `Slider`, `Checkbox`, `TextBox`, plus the
extended widgets below).

Shared helpers live in `scene` (`Shapes.h`) so no control duplicates them: `drawRoundedRect(target,
rect, radius, paint)` and `drawCircle(target, cx, cy, r, paint)`. `isConfirmKey(KeyEvent)` is an
inline helper in `base/InputController.h`.

### 12.1 `Knob`

- Extends `Segment` + `AbstractSlider`. `sensitivity` px maps vertical drag to value delta;
  `Left`/`Right` keys step. Fires `onChange(value)`.
- `advance(nowMs)` springs a smoothed *display* value toward the real value via a shared
  `anim::Spring` (FR-4d, `omega≈18`), so the dial moves smoothly when the value changes; `onPaint`
  draws from the smoothed value.
- `onPaint` draws the dial, a 270° arc track (sampled), a value arc up to the value, and the
  indicator line — which reaches the **outer edge of the value arc** (arc radius + ½ arc width).
  Optional `label`.
- **Modulation (FR-18).** Holds `std::vector<KnobMod>` (`{sourceId, depth∈[-1,1], color}`) and a
  `const ModBus*`. `modulatedValue()` = `clamp(base + Σ depthᵢ·bus.value(sourceᵢ)·range)`. `onPaint`
  draws one concentric ring per routing (radius `r+4+i·5`): a depth arc from the base to `base+depth`
  in the source colour plus a live dot at the modulated value. Each `KnobMod` owns an `anim::Spring
  appear` (target `1`); a newly added routing starts at `0` and grows in over `advance` (FR-18), and
  `onPaint` scales the drawn depth by `appear.value()`; re-colouring an existing source keeps its
  ring at full. `handleGesture` picks the drag mode by
  press radius — `ringAtRadius()` selects a ring band (vertical drag → `setModDepth`) else the dial
  (value drag); double-click on a ring erases that routing, on the dial resets to default.
- `ModBus` (in `ui/base/`) maps `sourceId → value`; sources publish with `set`, targets read with
  `value`. It is the minimal seam that decouples a `Knob` target from concrete sources.
- **Hover (FR-24):** the dial uses `hoverBox(dial, valueColor, hoverAmount())` and the value arc
  colour brightens on hover.

### 12.2 `ToggleSwitch`

- Boolean `on()`. `Click`/confirm toggles, animates `mThumb` (`AnimatedProperty`, `EaseOutCubic`
  160ms) toward 0/1, fires `onChange(bool)`. `advance(nowMs)` ticks the thumb. Reduced-motion
  (FR-4e) is honored automatically via `AnimatedProperty`.
- `onPaint` draws the rounded track and the moving thumb. **FR-25:** the track is
  `hoverBox(lerpBox(trackOff, trackOn, t01), trackOn.fill, hoverAmount())` — the off→on colour
  **blends continuously** with the thumb (no hard swap at the midpoint) and brightens on hover; the
  thumb radius grows slightly with `hoverAmount()`.

### 12.3 `ProgressBar`

- Non-interactive; `value()` in `[0,1]`. `hitTestSelf` returns false (input passes through).
- **FR-25:** `setValue` sets a target; a `Spring mDisplay` eases the **shown** level toward it,
  ticked in `advance(nowMs)`. `onPaint` draws track + a fill of the shown width (drawn only when
  `> 1e-4`, so a near-zero level shows no sub-pixel sliver). Reduced-motion snaps via `Spring`.

### 12.4 `ComboBox`

- `options`, `selectedIndex`, `isOpen`. `Click` on the field toggles open; when open, clicking a
  row selects it, closes the popup, and fires `onChange(index)`. The list is drawn in `onOverlay`.
- `advance(nowMs)` stamps the current time and ticks an `AnimatedProperty mOpenAnim` (0 closed → 1
  open, `EaseOutCubic` 160ms), started on open/close via the stamped time (mirrors `ToggleSwitch`).
  Logical `mOpen` flips immediately so rows are hit-testable during the reveal; the *visual* list
  fades in and slides down by `(1−p)·6px`. `onOverlay` early-outs when `p≈0`. Honors reduced-motion
  (FR-4e) through `AnimatedProperty`.
- **Hover (FR-24):** a `Move` records the option row under the pointer (`mHoverRow`); `advance`
  glides a `Spring mRowHiY` to that row and eases a `Spring mRowHiA` (alpha) in only while a row is
  hovered — so `onOverlay` draws a highlight bar that **glides** between rows and fades, never
  popping. The field uses `hoverBox(field, caret, hoverAmount())`.
- `onPaint` draws the field, the selected text, and a caret glyph.

### 12.5 `TabView`

- `addPage(title, segment)` appends a page; `selectedIndex` chooses the visible page (others have
  `visible=false`). Tab headers are child hit regions; `Click` on a header selects it and fires
  `onChange(index)`. The active page is positioned directly under the tab strip (`y = tabHeight`,
  no gap). **Unification (FR-21):** the active tab is full-height and extends `tabHeight+10`
  downward; the page (rendered after `onPaint`) covers the overhang, so the active tab merges into
  the content.
- **FR-25/FR-24:** per-tab `Property mTabFade` (eased on select, `EaseOutCubic` 180ms) interpolates
  each tab's geometry (`y = 4·(1−f)`, `hh = (tabHeight−4)+14·f`), box colour (`lerpBox(idle,active,f)`),
  active-indicator alpha, and title colour (`lerpColor(label,labelActive,f)`) — so selecting a tab
  **eases** instead of snapping. A per-tab `Spring mTabHover` (driven from the `Move`-tracked
  `mHoverTab`, cleared when the pointer leaves via `isHovered()`) brightens the tab under the
  pointer via `hoverBox`. Tabs are drawn least-active-first so the selected one lands on top.

### 12.6 `ScrollView`

- `setContent(segment)`, `contentHeight`. `clipToBounds = true`; the content child is translated by
  `-offset`. `Drag` on the body and `Drag` on the scrollbar thumb both change `offset` (1:1 direct
  manipulation, exempt from FR-25) via `applyRubberBand(raw)`, which compresses (rather than hard-
  clamps) any excess past `[0, maxOffset()]` by `kOverscrollFactor` (0.35).
- **Kinetic scrolling (FR-29).** `DragStart` cancels any live fling/snap-back (`mFlingVelocity =
  0`, `mSnapBackActive = false`) and marks `mDragging = true`; `Drop` clears it. A `Fling` gesture
  sets `mFlingVelocity = -g.velocity.y` (offset moves opposite the finger's Y, matching the drag
  math `offset = start - dy`). `advance(nowMs)`, while not dragging:
  - if `mOffset` is outside `[0, maxOffset()]`: on first entry, resets `Spring mSnapBack` to the
    current offset and targets the nearest boundary, zeroing `mFlingVelocity` (kinetic motion
    stops; the spring takes over); every subsequent frame advances the spring and adopts its
    value, snapping to the exact target and clearing `mSnapBackActive` once the spring stops
    moving (`Spring::isMoving()`);
  - else if `mFlingVelocity != 0`: integrates `mOffset += mFlingVelocity * dt`, decays
    `mFlingVelocity *= kFlingFriction ^ dt` (framerate-independent exponential, `dt` clamped to
    0.05s like `Spring`), and zeroes it below `kFlingStopVelocity` (20px/s) to avoid an infinite
    crawl. A fling that carries the offset out of range this frame is caught by the snap-back
    branch on the next frame (a one-frame lag, imperceptible).
  - This is a genuine **spring** recovery (FR-4d), not a new hand-rolled per-frame integrator, and
    inherits reduced-motion handling for free (`Spring::advance` already snaps under
    `reducedMotion()`, FR-4e).
- **Hover (FR-24):** `advance` eases a `Spring mScrollbar` toward `1` while `isHoverWithin()` (the
  content child owns hover, so hover-within is used, not this control's own hover); `onPaint` widens
  the scrollbar (`8 → 10px`) and brightens the thumb by that factor.

### 12.7 `LineGraph`

- Non-interactive. `setSeries(values)`, `setRange(min,max)`. `onPaint` draws background, horizontal
  grid lines, an optional filled area, and the polyline. Empty or single-point series draw only the
  frame.
- **FR-25:** the shown series is a `std::vector<Spring> mDisplay` that **morphs** toward the target
  `mSeries` (ticked in `advance`, `omega≈26` so live/streaming data still tracks). A change in point
  count rebuilds `mDisplay` and lands immediately (a morph across differing counts is ill-defined).

### 12.8 `ImageView`

- Displays a raster photo aspect-fitted into its bounds (the only core consumer of the §11c HAL
  primitive). `setImage(rgba, w, h)` copies the pixels (so it can re-register if drawn into a
  different target — a `RecordingTarget` in tests, then Cairo/Canvas2D at runtime); `clearImage()`
  drops them. `setFit(Contain|Cover|Fill)`; `fittedRect()` returns the destination rect in local
  space so overlays (e.g. a crop tool) can align to the displayed image.
- `onPaint` (lazy, target-aware): if the target changed or there is no handle yet, `registerImage`
  and cache the handle + target; else if the pixels are dirty, `updateImage`; then `drawImage`
  into `fittedRect()`. It never calls `releaseImage` (the adapter reclaims handles on teardown).
- Zoom/pan (`zoomAbout`, `panBy`, `resetView`) are **direct-manipulation transforms** exempt from
  FR-25: `fittedRect()` must remain the authoritative *immediate* geometry that overlays align to
  and hit-testing uses, so it cannot lag behind an easing value. When zoomed (>1×) the view is
  interactive and a drag pans 1:1; at 1× it is display-only (`hitTestSelf` false, click-through).

## 13. Traceability to Requirements

- FR-3 and FR-10 map to `Segment`.
- FR-4 maps to `Property` and `AnimatedProperty`.
- FR-5 maps to the segment focus registry.
- FR-7 maps to `Theme` and the style structs.
- FR-8 maps to `Button`, `Slider`, `Checkbox`, and `TextBox`.
- FR-9 maps to the split between `AbstractSlider` and `Slider`.
- FR-11 maps to `IRenderTarget::clipRect`, `RecordingTarget`, the two adapters, and
  `Segment::clipToBounds`.
- FR-26 maps to `IRenderTarget::clipPath`, `RecordingTarget` (`DrawOp::Kind::ClipPath`), and the
  Canvas2D / Cairo adapters.
- FR-27 maps to `IRenderTarget::{pushLayer,popLayer}`, `RecordingTarget` (`DrawOp::Kind::
  {PushLayer,PopLayer}`), and the Canvas2D / Cairo adapters.
- FR-31 maps to the five cubic-bezier `Easing` entries + `cubicBezierSolveX`/`cubicBezierY` in
  `Easing.cpp`, and the named constants in `anim/MotionTokens.h`.
- FR-30 maps to `drawShadow`/`drawElevation` in `scene/Shapes.h`/`.cpp`, composed from
  `IRenderTarget::setLinearFill`/`setRadialFill`.
- FR-28 maps to `RawPointer::touch`/`Gesture::touch`/`Gesture::velocity`, the new
  `Gesture::Type::{LongPress,Fling}` values, `GestureRecognizer::advance` + its long-press/fling
  state, and the touch check in `Segment::dispatchGesture`'s `Move` case.
- FR-29 maps to `ScrollView::applyRubberBand`, `mFlingVelocity`, `mSnapBack` (a `Spring`), and the
  regime handling in `ScrollView::advance`.
- FR-13 maps to `IRenderTarget::setRadialFill`, `RecordingTarget` (+ `DrawOp::color2`), and the
  Canvas2D / Cairo adapters.
- FR-17 maps to `IRenderTarget::setLinearFill`, `RecordingTarget` (`DrawOp::Kind::SetLinearFill`,
  reusing `DrawOp::color2`), and the Canvas2D / Cairo adapters.
- FR-14 maps to `Segment::SnapEdge`, `snapTo`/`clearSnap`/`resolveSnap`, resolved in
  `Segment::advance`.
- FR-15 maps to `LinearLayout` (base) and `Row` / `Column` (concrete), resolved in
  `LinearLayout::advance`.
- FR-18 maps to `KnobMod` + `Knob::addModulation/setModDepth/modulatedValue` and the `ModBus`
  (`ui/base/ModBus.h`); the depth rings render and drag in `Knob::onPaint`/`handleGesture`.
- FR-19 maps to `IRenderTarget::{registerImage,updateImage,drawImage,releaseImage}`,
  `RecordingTarget` (`DrawOp::Kind::{RegisterImage,UpdateImage,DrawImage,ReleaseImage}` +
  `imageId`/`imgW`/`imgH`/`pixelHash`), the Canvas2D / Cairo adapters, and `ImageView`
  (`ui/concrete/ImageView`).
- FR-12 maps to `Knob`, `ToggleSwitch`, `ProgressBar`, `ComboBox`, `TabView`, `ScrollView`, and
  `LineGraph`, one class per file under `ui/concrete/`.
- FR-24 maps to `Segment` hover state (`mHovered`/`mHoverAmount`, `setHovered`/`hoveredSegment`/
  `isHovered`/`hoverAmount`/`isHoverWithin`), the bare-`Move` routing in `Segment::dispatchGesture`,
  and the shared `ui/base/Interaction.h` treatment applied by every interactive control.
- FR-25 maps to the per-control animation state: `Button::mPress`, `Checkbox::mCheck`,
  `TextBox::mFocusAmt`, `TabView::mTabFade`/`mTabHover`, `ComboBox::mRowHiY`/`mRowHiA`,
  `ScrollView::mScrollbar`, `ProgressBar::mDisplay`, `LineGraph::mDisplay`, plus the continuous
  `ToggleSwitch` track blend — all reduced-motion-safe via `Property`/`Spring`.
- FR-22 maps to `IRenderTarget::drawText`'s `fontFamily`/`letterSpacingPx` parameters,
  `RecordingTarget` (`DrawOp::fontFamily`/`DrawOp::letterSpacingPx`), the Canvas2D / Cairo
  adapters, and `ui::TextStyle` + `scene::Text` (propagated through `LabelSegment`, `TabView`,
  `ComboBox`, and `Knob`).