# Arstro Artboard — Software Requirements

## 1. Purpose

Artboard shall provide a platform-free graphics and UI core for Arstro applications. The core shall
support both custom drawing and a basic themed control set without embedding backend-specific code
into the application layer.

## 2. Scope

The system covers:

- 2D drawing through a render HAL.
- Time-based animation through abstract properties.
- Pointer and keyboard interaction through platform-free input abstractions.
- Composite UI objects through `Segment`.
- A baseline theme and basic controls: `Button`, `Slider`, `Checkbox`, and `TextBox`.
- A rectangular clip primitive on the render HAL, and clip-to-bounds for segments.
- An extended widget set: `Knob`, `ToggleSwitch`, `ProgressBar`, `ComboBox`, `TabView`,
  `ScrollView`, and `LineGraph`.

The system does not currently include constraint/flow layout managers, font measurement, or
accessibility APIs.

## 3. Functional Requirements

### FR-1 Graphics abstraction

The framework shall expose a minimal render HAL so adapters can render the same scene on multiple
backends.

### FR-2 Scene drawing

The framework shall allow applications to build a tree of `Drawable` objects for freeform graphics.

### FR-3 Composite UI objects

The framework shall provide a `Segment` base class representing an interactive object with:

- Position and size.
- Visibility and enabled state.
- Optional clipping intent.
- Recursive child ownership.
- Recursive drawing.
- Pointer hit testing.
- Keyboard focus participation.

### FR-4 Animated UI properties

The framework shall expose animated scalar properties so a segment can animate position and size
without backend-specific timing logic.

### FR-4a Easing curve library

The framework shall provide a library of easing curves as pure functions over `t` in `[0,1]`,
covering at minimum linear, quad, cubic, quart, sine, expo, back, elastic, and bounce families
(in/out/in-out where applicable). Each curve shall clamp its input to `[0,1]` and be `0` at `t=0`
and `1` at `t=1` (back/elastic may overshoot between the endpoints but still satisfy them). Easing
selection shall not embed any backend-specific code.

### FR-4b Tween specification

The framework shall provide a `Tween` value type describing a complete scalar animation: `from`,
`to`, `durationMs`, optional `delayMs`, an easing curve, a `repeat` count (`-1` = infinite), and a
`yoyo` flag (reverse on alternate repeats). `Tween` shall be a pure, time-source-agnostic function
of elapsed time so the same specification drives live UI and offline rendering identically.

### FR-4c Animator timeline

The framework shall provide an `Animator` that lets an application animate **any** value with a
single declarative call by supplying an `onUpdate(value)` callback (and optional `onComplete`),
without binding the value to a `Segment` property. The application shall advance all active
animations with one `advance(nowMs)` call per frame; finished animations shall be removed
automatically. The `Animator` shall not embed any backend-specific timing or drawing code.

### FR-5 Shared focus groups

The framework shall support focus groups so only one segment per focus index is focused at a time.

### FR-6 Input abstraction

The framework shall accept platform-neutral pointer gestures and keyboard events and route them to
segments or other input targets without backend-specific gesture logic in controls.

### FR-7 Theme support

The framework shall provide concrete visual style data objects that can be attached to visual
segments and controls.

### FR-8 Basic controls

The framework shall provide baseline implementations for:

- `Button`
- `Slider`
- `Checkbox`
- `TextBox`

### FR-9 Slider separation of concerns

The slider design shall separate behavior from rendering by introducing an `AbstractSlider` that
owns slider state without visual concerns, and a concrete `Slider` that renders using child
segments.

### FR-9a Default value and double-click reset

Every ranged control built on `AbstractSlider` (`Slider`, `Knob`) shall carry a **default
value** within `[min,max]`. The default is settable (`setDefault(v)`, clamped to the range and
re-clamped when the range changes) and defaults to the control's initial value. A
**double-click** on the control resets its value to the default and emits `onChange(default)`
(the same notification a drag would emit), so a user can restore a parameter's nominal setting
with one gesture. The `Knob`'s smoothed display springs to the default like any other value
change.

### FR-10 Parent-child motion

When a parent segment moves, all child segments shall move with it through composed transforms.

### FR-11 Clip primitive

The render HAL shall provide a rectangular clip primitive `clipRect(x, y, w, h)` that intersects
the current clip region with the given rectangle in the current transform space. The clip shall be
scoped by the `save()`/`restore()` state stack (a `restore()` discards clips pushed since the
matching `save()`). A `Segment` with `clipToBounds == true` shall clip its children to its local
bounds. This is a HAL extension because a clip region cannot be expressed by fill/stroke/path
primitives alone.

### FR-12 Extended widgets

The framework shall provide the following widgets, each a `Segment`, themeable, and driven only
through the input + render HALs:

- `Knob` — a rotary analog control over a `[min,max]` range; vertical drag changes the value;
  emits `onChange(value)`.
- `ToggleSwitch` — a boolean control; click/confirm toggles it with an animated thumb; emits
  `onChange(bool)`.
- `ProgressBar` — a non-interactive bar displaying a `[0,1]` value (e.g. a level meter).
- `ComboBox` — a drop-down that shows the selected option and, when open, a list of options;
  selecting one closes it and emits `onChange(index)`.
- `TabView` — a tab strip plus pages; selecting a tab shows its page and emits `onChange(index)`.
- `ScrollView` — a clipped viewport over taller content; vertical drag (and a draggable thumb)
  scrolls the content within `[0, contentHeight - viewportHeight]`.
- `LineGraph` — a non-interactive plot of a numeric series over a `[min,max]` value range, drawn
  as grid + polyline (+ optional filled area) for data visualisation.

### FR-13 Radial-gradient fill primitive

The render HAL shall provide a two-stop radial-gradient fill,
`setRadialFill(cx, cy, radius, innerColor, outerColor)`, which sets the current fill so that a
subsequent `fillPath()` paints a smooth radial gradient: `innerColor` at the centre `(cx, cy)`
fading to `outerColor` at `radius` (coordinates in the current transform space). It replaces any
solid `setFill` until the next `setFill`/`setRadialFill`. This is a HAL extension because a smooth
opacity/colour gradient **cannot** be expressed by solid fills (stacking translucent shapes only
approximates it with visible banding). It enables true soft glows that fade continuously to zero
opacity (`outerColor` alpha = 0). Adapters back it with their native gradient (Canvas2D
`createRadialGradient`, Cairo radial pattern); the `RecordingTarget` records it for tests.

### FR-17 Linear-gradient fill primitive

The render HAL shall provide a two-stop linear-gradient fill,
`setLinearFill(x0, y0, x1, y1, startColor, endColor)`, which sets the current fill so that a
subsequent `fillPath()` paints a smooth linear gradient running along the axis from `(x0, y0)`
(`startColor`) to `(x1, y1)` (`endColor`), in the current transform space; the gradient is
constant along lines perpendicular to that axis. It replaces any solid/radial fill until the
next `setFill`/`setRadialFill`/`setLinearFill`. Like the radial fill, a smooth linear gradient
**cannot** be expressed by solid fills (stacking translucent bands shows visible steps), so it
is a HAL primitive. It enables depth/shading ramps (panel backgrounds, a receding "floor" under
a 3-D plot, vertical fades to zero opacity). Adapters back it with their native gradient
(Canvas2D `createLinearGradient`, Cairo linear pattern); the `RecordingTarget` records it for
tests.

### FR-14 Segment snap constraint

A `Segment` shall be able to **snap** one of its edges to an edge of another segment with an
offset: `snapTo(target, myEdge, targetEdge, offset)`, where an edge is one of
`Left/Right/CenterX` (horizontal axis) or `Top/Bottom/CenterY` (vertical axis). After each
`advance(nowMs)`, the segment's position shall be recomputed so that `myEdge == targetEdge +
offset` in the parent coordinate space; horizontal edges adjust `x`, vertical edges adjust `y`.
Consequently, when the target moves the snapped segment follows (e.g. the left of B snapped to
the right of A keeps B glued to A as A moves). `clearSnap()` removes the constraint; a null or
self target is ignored. Snap is platform-free geometry (no HAL change).

### FR-15 Linear layout containers

The framework shall provide `Row` and `Column` layout segments that position their **visible**
children along one axis — `Row` left→right (advancing `x`), `Column` top→bottom (advancing `y`) —
separated by a `spacing` gap and inset by `padding`, with the cross-axis offset set to `padding`.
Layout is resolved each `advance(nowMs)`. The container **auto-sizes** to its content (main axis =
sum of child extents + gaps + padding; cross axis = largest child + padding). With no visible
children the container collapses to `2·padding` on each axis. This is platform-free geometry (no
HAL change) and lets a `Column` of `Row`s express grouped control layouts.

### FR-16 Current-path lifecycle (paint primitive contract)

The render HAL maintains a single **current path** built by `beginPath`/`moveTo`/`lineTo`/
`quadTo`/`cubicTo`/`closePath`. The lifecycle shall be identical on every adapter:

- `beginPath()` **clears** the current path and starts a new one.
- `fillPath()` paints the interior of the current path and **preserves** it.
- `strokePath()` paints the outline of the current path and **preserves** it.

Because fill and stroke preserve the path, a shape may fill and then stroke the *same* path —
this is exactly how `applyPaint` realises a `filledStroked` paint (a filled shape with a
border). The path persists until the next `beginPath` (or `clipRect`, which also begins a fresh
path). An adapter that discards the path inside `fillPath`/`strokePath` would silently drop the
border of every filled-and-stroked shape, making that platform diverge from the others; this is
forbidden. The result must be pixel-equivalent across web (Canvas2D), native (Cairo), and the
`RecordingTarget` op stream.

### FR-18 Knob modulation (depth rings + ModBus)

A `Knob` shall support **modulation routings**: each routing names a source (an
integer id), carries a signed **depth** in `[-1,1]` of the knob's full range, and a
colour. Live source values are published on a `ModBus` (`set(id,value)` /
`value(id)`), which the knob reads (`setModBus`) so a target depends only on the bus,
not on any concrete source. The knob's **modulated value** = `base + Σ depthᵢ ·
busValue(sourceᵢ) · range`, clamped to the range.

Each routing renders as a concentric **outer ring** (Serum-style) in the source
colour: a **unipolar** routing arcs from the base value to its reach (`base+depth`); a
**bipolar** routing (a source that swings `[-1,1]`, e.g. an LFO) arcs both ways,
`base ± |depth|`. A live dot marks the current modulated value. Interaction: a press whose radius falls on a ring drags that
ring **vertically to set its depth**; a press on the dial drags the value as before; a
double-click on a ring **removes** that routing (a double-click on the dial still
resets to default, FR-9a). `addModulation(id,colour)` adds a routing (or re-colours an
existing one for that source). Assignment of a source to a target (drag-and-drop) is
performed by the application, which then calls `addModulation`.

### FR-19 Raster image primitive (register / draw) + ImageView

The render HAL shall provide a **raster image** primitive so a photograph (or any
pixel buffer) can be drawn — something no fill/stroke/path/text combination can
reproduce. It uses a **handle/registration** model so a large image is uploaded once,
not re-sent every frame:

- `registerImage(rgba, w, h) -> id` uploads pixels and returns a positive handle
  (0 = failure). Pixel format is fixed: tightly-packed **RGBA8**, 4 bytes/pixel,
  row-major top-to-bottom, stride `w*4`, **straight (non-premultiplied)** alpha, sRGB.
- `updateImage(id, rgba, w, h)` replaces a handle's pixels (and dimensions) — used when
  an edited preview changes; it does **not** allocate a new handle.
- `drawImage(id, dst)` blits the handle into the destination `Rect` in the current
  transform space (no-op if the id is unknown).
- `releaseImage(id)` frees a handle.

The handle model is chosen deliberately: a photo editor redraws at frame rate but the
pixels change only on an edit, so per-frame re-upload would be wasteful. Per the
platform-independence trade-off rule the cost lives on the adapter side (one surface /
offscreen canvas per handle) while the visible result is identical everywhere. Adapters:
Canvas2D keeps an offscreen `<canvas>` per id (`ImageData` is straight RGBA8 top-down —
no conversion); Cairo keeps an ARGB32 surface per id, converting straight RGBA8 to
**premultiplied BGRA** honoring Cairo's stride; the `RecordingTarget` records the call
(id, dimensions, a position-weighted pixel hash, and the draw rect) for tests.

The framework shall also provide an **`ImageView`** segment that owns a copy of an
image's pixels, registers it lazily (re-registering if drawn into a different target),
re-uploads only when the pixels change, and draws it **aspect-fitted** into its bounds
(`Contain` / `Cover` / `Fill`), exposing the fitted rect (`fittedRect()`) so overlays
can align to the displayed image. `ImageView` is platform-free (emits only the HAL
primitives above).

## 4. Non-functional Requirements

### NFR-1 Platform independence

The core shall compile without OS- or backend-specific UI dependencies.

### NFR-2 Testability

The core shall be testable with `RecordingTarget` and platform-free unit tests.

### NFR-3 SOLID alignment

The design shall respect SRP, OCP, LSP, ISP, and DIP.

### NFR-4 Minimal coupling

Controls shall depend on abstract input and render services, not on a concrete adapter.

### NFR-5 Incremental extensibility

New controls, styles, and adapters shall be addable without rewriting existing controls.

## 5. Constraints and Open Items

- `clipToBounds` is currently a semantic flag only; a future render HAL clipping primitive is needed
  to enforce clipping visually.
- Text layout currently uses approximate width estimation because the render HAL does not expose font
  metrics.
- Keyboard events are abstract and backend-neutral; adapters must map native key events into
  `KeyEvent`.

## 6. Verification Outline

- Unit tests verify recursive rendering, focus routing, gesture capture, and baseline control
  behavior.
- Architecture and detailed design documents trace requirements to concrete classes and packages.