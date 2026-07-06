# Arstro Artboard — Software Architecture

## 1. Architectural Overview

Artboard is organized as a platform-free core with adapter edges. The architecture has six internal
packages and one adapter package:

- `core`: geometry and color value types.
- `anim`: easing, tweening, and animated scalar properties.
- `render`: output HAL (`IRenderTarget`) and the recording test adapter.
- `input`: pointer abstraction, gesture recognition, and generic routing.
- `scene`: freeform drawing objects built from `Drawable`.
- `ui`: interactive object model built from `Segment`, style objects, and basic controls.
- `adapter/web`: backend implementation for Canvas2D.

## 2. Key Architectural Decisions

### 2.1 Two authoring models on one HAL

The architecture keeps freeform graphics and interactive UI separate:

- `Drawable` serves retained graphics.
- `Segment` serves retained interactive UI.

Both models emit the same render primitives to `IRenderTarget` and use the same transform model.

### 2.2 Composite UI through `Segment`

`Segment` is the UI composition root. It extends `Drawable` for rendering and implements
`InputTarget` for interaction. This makes each UI object:

- Renderable.
- Hit-testable.
- Focusable.
- Nestable.
- Animatable through scalar properties.

### 2.3 Behavior separated from style

Behavior is carried by the control class hierarchy and input hooks. Style is carried by concrete
theme structures such as `SliderStyle`, `ButtonStyle`, `CheckboxStyle`, and `TextBoxStyle`.

This keeps visual decisions replaceable without changing control logic.

### 2.4 Behavior separated from slider visuals

`AbstractSlider` owns value semantics, range, analog/digital mode, and drag model. `Slider`
inherits the behavior and realizes it visually using child `RectangleSegment` and `CircleSegment`
objects.

### 2.5 Shared focus registry

Focus is treated as a cross-tree concern. Each focus group is represented by an index, and only one
segment in a group may be focused at a time. This supports keyboard navigation policies without
forcing all controls into one monolithic manager class.

## 3. Package Responsibilities

### 3.1 `core`

- `Point`, `Size`, `Rect`, `Transform`, and `Color` are immutable-style value holders.
- `Transform::inverse()` supports UI hit-testing in nested segment trees.

### 3.2 `anim`

- `Easing` is a library of pure easing curves (`applyEasing(curve, t)`); curves never touch a backend.
- `Animation` defines a pure single-shot time-based scalar tween (kept for back-compat).
- `Tween` is a richer pure spec: `from/to/durationMs/delayMs/easing/repeat/yoyo`, sampled by elapsed
  time (`at(elapsedMs)`), used by both `AnimatedProperty` and `Animator`.
- `AnimatedProperty` stores runtime animation state and drives a single scalar from a `Tween`
  (delay/repeat/yoyo + an `onComplete` callback).
- `Animator` is a callback-based timeline: it owns many tracks, each animating an arbitrary value
  through an `onUpdate(value)` callback; one `advance(nowMs)` per frame ticks them all and drops
  finished tracks. It is the ergonomic "animate anything" entry point and depends only on
  `std::function` — no backend, no `Segment` coupling.
- `Spring` is a critically-damped, **framerate-independent** display-smoothing follower
  (`advance(dtSeconds, omega)` via a closed-form step). It is the single source of truth for the
  "value glides to a target" motion that `Knob` and `Slider` (and app displays) previously each
  hand-integrated with per-frame Euler.
- `reducedMotion()` / `setReducedMotion(bool)` are a global accessibility switch consulted by
  `Spring` and `AnimatedProperty`; when on, all motion collapses to instant. This keeps the
  reduce-motion decision in one place instead of per-control conditionals.
- `ui::Property` wraps `AnimatedProperty` for segment-level use.

### 3.3 `render`

- `IRenderTarget` is the only output seam. Besides state/paint/path/text it exposes one
  region primitive, `clipRect(x,y,w,h)`, intersected with the current clip and scoped by
  `save()`/`restore()`. Clipping is a primitive because no combination of fill/stroke/path ops
  can restrict subsequent drawing to a region; every adapter implements it natively
  (Canvas2D `clip()`, Cairo `cairo_clip()`), and `RecordingTarget` records it.
- It also exposes one paint-server primitive, `setRadialFill(cx,cy,r,inner,outer)` — a two-stop
  radial gradient that the next `fillPath()` uses. A smooth colour/opacity gradient is a primitive
  because solid fills can only approximate it by stacking translucent shapes (which bands); every
  adapter maps it to a native gradient (Canvas2D `createRadialGradient`, Cairo radial pattern), so
  glows fade continuously to zero opacity. `RecordingTarget` records it.
- It exposes a second paint-server primitive, `setLinearFill(x0,y0,x1,y1,start,end)` — a two-stop
  linear gradient along an axis that the next `fillPath()` uses. Same justification as the radial
  fill (a smooth ramp can't be built from solid fills without banding); every adapter maps it to a
  native gradient (Canvas2D `createLinearGradient`, Cairo linear pattern). It gives depth/shading
  ramps (panel gradients, a receding floor under a 3-D plot). `RecordingTarget` records it.
- The HAL keeps **one current path** (built by `beginPath`/`moveTo`/`lineTo`/`quadTo`/`cubicTo`/
  `closePath`). The lifecycle is fixed and must be identical on every adapter: `beginPath` clears
  the path, while `fillPath` and `strokePath` **paint and preserve** it. Preserving the path is
  what lets a shape fill then stroke the *same* path — how `applyPaint` draws a `filledStroked`
  paint (filled body + border). An adapter that clears the path inside `fillPath`/`strokePath`
  (e.g. a stray `cairo_new_path`) silently drops every border and diverges from the others, so it
  is a bug — the path is only reset by the next `beginPath`/`clipRect`.
- It exposes a **raster image** primitive (`registerImage`/`updateImage`/`drawImage`/`releaseImage`)
  for drawing a photograph or pixel buffer — impossible to express with fill/stroke/path/text. It is
  a **handle/registration** model rather than an immediate `drawImage(pixels,...)`: a photo editor
  redraws at frame rate while the pixels change only on an edit, so uploading once (register) and
  re-uploading only on change (update) avoids re-sending megabytes every frame. This is the
  platform-independence trade-off rule in action — the cost (one offscreen canvas / Cairo surface per
  handle, and the RGBA8→native conversion) lives on the adapter side while the visible result is
  identical everywhere. Pixel format is fixed straight RGBA8 top-down; Canvas2D's `ImageData` matches
  it directly, Cairo converts to premultiplied BGRA honoring its stride, and `RecordingTarget` records
  id/dimensions/draw-rect (+ a pixel hash to prove an update changed the bytes).
- `IRenderTarget::drawText(text, x, y, sizePx, fontFamily="", letterSpacingPx=0)` carries two
  optional parameters beyond the original three: a font family name (resolved by the adapter's
  own text stack — Fontconfig on native, the CSS font stack on web — never loaded by the HAL
  itself) and extra per-glyph advance for tracking. Both default to the prior behavior, so this
  is a backward-compatible widening of an existing primitive, not a new one: a font family
  string cannot be composed from the existing path/fill/stroke primitives (glyph outlines are
  adapter/OS text-stack territory), so it stays a parameter on the one text primitive rather
  than a new HAL method. `RecordingTarget` records both fields; `CairoTarget` selects the
  family via `cairo_select_font_face` and, when tracking is non-zero, advances glyph-by-glyph
  (UTF-8 aware) using `cairo_text_extents`; `Canvas2DTarget` builds a quoted CSS `font` string
  and sets `ctx.letterSpacing` where supported.
- `RecordingTarget` records draw operations for tests and inspection.

### 3.4 `input`

- `GestureRecognizer` converts raw pointer samples into gestures.
- `InputRouter` handles z-order routing and press capture.
- `KeyEvent` lives in the UI layer because only controls currently depend on it.

### 3.5 `scene`

- `Drawable` is the retained drawing base.
- `Rectangle`, `Line`, `Polyline`, `Ellipse`, `Path`, and `Text` implement primitive graphics.
- `Artboard` is the scene root for ordered drawing.

### 3.6 `ui`

The `ui` module is split by role into two folders, **one class per file** for maintainability:

- **`ui/base/`** — framework foundations and reusable building blocks:
  - `Segment` (composite interactive base; `clipToBounds` clips children via the HAL `clipRect`;
    `snapTo()` constrains one edge to another segment's edge + offset, resolved each `advance()`
    so a segment follows the one it is snapped to),
  - `LinearLayout` (base) + `Row` / `Column` — position visible children along one axis with
    spacing/padding and auto-size to content (resolved each `advance()`),
  - `InputController` (abstract input strategy; also declares `KeyEvent` and the `isConfirmKey`
    helper),
  - `Property` (animated scalar wrapper),
  - `Theme` (all concrete visual style structs + the baseline theme),
  - `AbstractSlider` (ranged value/behavior with no visual concerns),
  - `ModBus` (the live modulation-source value bus: source id → value, read by targets),
  - `Observable<T>` (a single-source-of-truth value with change notification: several UI nodes
    bind one value via `observe()` so a toggle button and the panel it controls can't drift out
    of sync — FR-23),
  - `RectangleSegment`, `CircleSegment`, `LabelSegment` (reusable visual nodes).
- **`ui/concrete/`** — the finished, themed controls, each its own file:
  - baseline: `Button`, `Slider`, `Checkbox`, `TextBox`,
  - extended: `Knob`, `ToggleSwitch`, `ProgressBar`, `ComboBox`, `TabView`, `ScrollView`,
    `LineGraph`, `ImageView` (aspect-fits a registered raster image into its bounds; the only
    core consumer of the HAL raster primitive).

Concrete controls reuse `Segment` composition, the shared `drawRoundedRect`/`drawCircle` helpers
(in `scene`), and `AbstractSlider` where a ranged value applies (`Slider`, `Knob`). The aggregate
header `include/artboard/artboard.h` pulls in every base + concrete header.

`Knob` is also a **modulation target**: it holds a list of `KnobMod` routings (source id + signed
depth + colour) and reads live source values from a `ModBus`, so it draws Serum-style depth rings
and computes a modulated value without depending on any concrete source. Source→target assignment
(drag-and-drop) is orchestrated by the application, which then calls `Knob::addModulation`; the bus
keeps the seam minimal (a target needs only `value(id)`).

## 4. Control Architecture

### 4.1 Button

- Behavior: press, click, keyboard confirm.
- Visual composition: body rectangle + label segment.

### 4.2 Slider

- Behavior: value, range, drag semantics, keyboard step adjustments.
- Visual composition: track rectangle + range fill rectangle + thumb circle.

### 4.3 Checkbox

- Behavior: boolean toggle by click or keyboard confirm.
- Visual composition: outer box + inner indicator + label segment.

### 4.4 TextBox

- Behavior: focus acquisition, text insertion, backspace.
- Visual composition: background rectangle + text label + caret rectangle.

### 4.5 Knob

- Behavior: `AbstractSlider` value/range; vertical drag (drag distance / sensitivity) and keyboard
  step; `onChange(value)`. `advance()` springs a smoothed display value so the dial eases to the
  target; the indicator reaches the outer edge of the value arc.
- Visual composition: drawn directly in `onPaint` — dial circle, sampled arc track, value arc, and
  indicator line over a 270° sweep.

### 4.6 ToggleSwitch

- Behavior: boolean; click or keyboard confirm toggles; `onChange(bool)`; thumb position is an
  `AnimatedProperty` for a smooth slide.
- Visual composition: rounded track + moving thumb circle, drawn in `onPaint`.

### 4.7 ProgressBar

- Behavior: non-interactive; `setValue([0,1])`.
- Visual composition: track + clamped fill, drawn in `onPaint`.

### 4.8 ComboBox

- Behavior: open/closed state; click toggles; option rows are child `Segment`s created when open;
  selecting a row sets the index, closes, and fires `onChange(index)`.
- Visual composition: field box + caret glyph + label; popup rows below the field.

### 4.9 TabView

- Behavior: a selected index; each tab header is a child hit region; selecting shows one page
  segment and hides the others; `onChange(index)`.
- Visual composition: tab-strip headers + the active page subtree.

### 4.10 ScrollView

- Behavior: holds one taller content segment; vertical drag and a draggable thumb adjust a scroll
  offset clamped to `[0, contentHeight - viewportHeight]`; `clipToBounds` clips the content.
- Visual composition: clipped viewport (content translated by `-offset`) + scrollbar track/thumb.

### 4.11 LineGraph

- Behavior: non-interactive; `setSeries(values)` + value range; data visualisation.
- Visual composition: background, grid lines, an optional filled area, and the series polyline,
  drawn in `onPaint`.

## 5. SOLID Mapping

- SRP: render HAL, input HAL, scene graphics, UI composition, and control state are distinct units.
- OCP: new adapters, segments, and controls can be added without modifying the core abstractions.
- LSP: any `Drawable` can render, and any `Segment` can participate in the same render/input flow.
- ISP: `IRenderTarget` and `InputController` stay narrow.
- DIP: application code depends on `IRenderTarget`, `Segment`, and abstract input contracts rather
  than backend classes.

## 6. Known Architectural Gaps

- No text measurement service yet, so text box / combo / graph text placement is approximate
  (this remains true even with FR-22's family/letter-spacing support — selecting a family does
  not report its metrics back to the caller).
- No constraint/flow layout containers yet, so sizing and placement remain explicit at the
  segment level (widgets size themselves but are positioned by the app).
- Clipping is rectangular only (no arbitrary path clip / soft masks yet).