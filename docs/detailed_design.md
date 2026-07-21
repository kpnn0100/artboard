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
- `bool enabled, visible, focusable, clipToBounds`
- `int focusIndex`
- `std::vector<std::shared_ptr<Segment>> mChildren`
- `std::shared_ptr<InputController> mInputController`
- `bool mHovered, mHoverPrev` + `Property mHoverAmount` — the hover state and its animated
  `[0,1]` factor (a single global hover owner is held in an anonymous-namespace slot in the .cpp).

### Important operations

- `render()` composes parent and local transforms, paints self, then paints children.
- `hitTest()` checks children from topmost to backmost, then checks the local bounds.
- `onGesture()` delegates to `dispatchGesture()`.
- `dispatchGesture()` on a bare `Move` (no press capture) recurses to the deepest hit-tested child
  (as `Down` does) and, at the leaf, calls `setHovered(this)` and delivers the move to the handler;
  a `Move` during a press still goes to the captured child (FR-24).
- `advance(nowMs)` first calls `updateHoverAnim(nowMs)`, which eases `mHoverAmount` toward `1`
  while hovered/`enabled`/`visible` and `0` otherwise (≈120 ms `EaseOutCubic`, reduced-motion-safe),
  then updates the layout properties and children.
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

## 3. `InputController`

`InputController` is an abstract behavior strategy.

### Responsibilities

- Consume gestures for a segment.
- Consume keyboard events for a segment.

### Intent

This supports reusable behavior injection when behavior should be shared across multiple segment
types without creating a deep inheritance chain.

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
  manipulation, exempt from FR-25), clamped to `[0, max(0, contentHeight - height)]`.
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