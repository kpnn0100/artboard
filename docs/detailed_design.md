# Arstro Artboard — Software Detailed Design

## 1. `Segment`

`Segment` is the retained UI base class.

### Responsibilities

- Store animated `x`, `y`, `width`, and `height` properties.
- Render itself and then render children recursively.
- Convert world-space pointer coordinates to local coordinates using inverse transforms.
- Participate in a shared focus group.
- Capture child interaction during a press-drag-drop sequence.

### Important fields

- `Property x, y, width, height`
- `bool enabled, visible, focusable, clipToBounds`
- `int focusIndex`
- `std::vector<std::shared_ptr<Segment>> mChildren`
- `std::shared_ptr<InputController> mInputController`

### Important operations

- `render()` composes parent and local transforms, paints self, then paints children.
- `hitTest()` checks children from topmost to backmost, then checks the local bounds.
- `onGesture()` delegates to `dispatchGesture()`.
- `dispatchKey()` routes keyboard events to the focused segment.
- `requestFocus()` updates the shared focus registry.

## 2. `Property`

`Property` is a thin UI-facing wrapper around `AnimatedProperty`.

### Responsibilities

- Store the current scalar value.
- Start an animation toward a target value (simple `animateTo`, or a full `Tween`).
- Advance the value at a supplied time.

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
- `advance(nowMs)` calls `layout()` then `Segment::advance`. Children use fixed sizes, so the
  layout is resolved in the same frame.

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
- Left and right arrow keys decrement or increment the value.
- Each of these (and the double-click reset) fires `onChange(value())` with the resulting
  value; a programmatic `setValue()` does **not** fire `onChange` (so syncing controls to
  state doesn't recurse) — same contract as `Knob`.

### Rendering sequence

1. Ensure the internal child tree exists.
2. Synchronize child geometry from current segment bounds and slider value.
3. Render the segment tree.

## 8. `Button`

### Internal composition

- `RectangleSegment mBody`
- `LabelSegment mLabel`

### Behavior

- `Down` sets the pressed state.
- `Click` invokes `onClick`.
- `Enter` or `Space` invokes `onClick`.

## 9. `Checkbox`

### Internal composition

- `RectangleSegment mBox`
- `RectangleSegment mIndicator`
- `LabelSegment mLabel`

### Behavior

- `Click`, `Enter`, or `Space` toggles the boolean state.
- The indicator visibility mirrors the checked state.

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
- `advance(nowMs)` springs a smoothed *display* value (critically damped) toward the real value,
  so the dial moves smoothly when the value changes; `onPaint` draws from the smoothed value.
- `onPaint` draws the dial, a 270° arc track (sampled), a value arc up to the value, and the
  indicator line — which reaches the **outer edge of the value arc** (arc radius + ½ arc width).
  Optional `label`.
- **Modulation (FR-18).** Holds `std::vector<KnobMod>` (`{sourceId, depth∈[-1,1], color}`) and a
  `const ModBus*`. `modulatedValue()` = `clamp(base + Σ depthᵢ·bus.value(sourceᵢ)·range)`. `onPaint`
  draws one concentric ring per routing (radius `r+4+i·5`): a depth arc from the base to `base+depth`
  in the source colour plus a live dot at the modulated value. `handleGesture` picks the drag mode by
  press radius — `ringAtRadius()` selects a ring band (vertical drag → `setModDepth`) else the dial
  (value drag); double-click on a ring erases that routing, on the dial resets to default.
- `ModBus` (in `ui/base/`) maps `sourceId → value`; sources publish with `set`, targets read with
  `value`. It is the minimal seam that decouples a `Knob` target from concrete sources.

### 12.2 `ToggleSwitch`

- Boolean `on()`. `Click`/confirm toggles, animates `mThumb` (`AnimatedProperty`) toward 0/1,
  fires `onChange(bool)`. `advance(nowMs)` ticks the thumb.
- `onPaint` draws the rounded track (color lerps with thumb position) and the moving thumb.

### 12.3 `ProgressBar`

- Non-interactive; `value()` in `[0,1]`. `onPaint` draws track + clamped fill. `hitTestSelf`
  returns false (input passes through).

### 12.4 `ComboBox`

- `options`, `selectedIndex`, `isOpen`. `Click` on the field toggles open; when open,
  `ensureRows()` creates one child `Segment` per option below the field; clicking a row selects it,
  closes the popup, and fires `onChange(index)`. Rows are removed when closed.
- `onPaint` draws the field, the selected text, and a caret glyph.

### 12.5 `TabView`

- `addPage(title, segment)` appends a page; `selectedIndex` chooses the visible page (others have
  `visible=false`). Tab headers are child hit regions; `Click` on a header selects it and fires
  `onChange(index)`. The active page is positioned under the tab strip.

### 12.6 `ScrollView`

- `setContent(segment)`, `contentHeight`. `clipToBounds = true`; the content child is translated by
  `-offset`. `Drag` on the body and `Drag` on the scrollbar thumb both change `offset`, clamped to
  `[0, max(0, contentHeight - height)]`.
- `onPaint` draws the viewport background + a scrollbar track/thumb sized to the visible fraction.

### 12.7 `LineGraph`

- Non-interactive. `setSeries(values)`, `setRange(min,max)`. `onPaint` draws background, horizontal
  grid lines, an optional filled area, and the series polyline mapped into the bounds. Empty or
  single-point series draw only the frame.

### 12.8 `ImageView`

- Displays a raster photo aspect-fitted into its bounds (the only core consumer of the §11c HAL
  primitive). `setImage(rgba, w, h)` copies the pixels (so it can re-register if drawn into a
  different target — a `RecordingTarget` in tests, then Cairo/Canvas2D at runtime); `clearImage()`
  drops them. `setFit(Contain|Cover|Fill)`; `fittedRect()` returns the destination rect in local
  space so overlays (e.g. a crop tool) can align to the displayed image.
- `onPaint` (lazy, target-aware): if the target changed or there is no handle yet, `registerImage`
  and cache the handle + target; else if the pixels are dirty, `updateImage`; then `drawImage`
  into `fittedRect()`. It never calls `releaseImage` (the adapter reclaims handles on teardown).
  `hitTestSelf` returns false — display-only; interactive overlays are separate segments.

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