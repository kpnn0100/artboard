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