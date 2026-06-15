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
  - `Segment` (composite interactive base; `clipToBounds` clips children via the HAL `clipRect`),
  - `InputController` (abstract input strategy; also declares `KeyEvent` and the `isConfirmKey`
    helper),
  - `Property` (animated scalar wrapper),
  - `Theme` (all concrete visual style structs + the baseline theme),
  - `AbstractSlider` (ranged value/behavior with no visual concerns),
  - `RectangleSegment`, `CircleSegment`, `LabelSegment` (reusable visual nodes).
- **`ui/concrete/`** — the finished, themed controls, each its own file:
  - baseline: `Button`, `Slider`, `Checkbox`, `TextBox`,
  - extended: `Knob`, `ToggleSwitch`, `ProgressBar`, `ComboBox`, `TabView`, `ScrollView`,
    `LineGraph`.

Concrete controls reuse `Segment` composition, the shared `drawRoundedRect`/`drawCircle` helpers
(in `scene`), and `AbstractSlider` where a ranged value applies (`Slider`, `Knob`). The aggregate
header `include/artboard/artboard.h` pulls in every base + concrete header.

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
  step; `onChange(value)`.
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

- No text measurement service yet, so text box / combo / graph text placement is approximate.
- No constraint/flow layout containers yet, so sizing and placement remain explicit at the
  segment level (widgets size themselves but are positioned by the app).
- Clipping is rectangular only (no arbitrary path clip / soft masks yet).