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
- `adapter/web`: backend implementation for Canvas2D (Emscripten).
- `adapter/native`: `CairoTarget`, the Cairo `IRenderTarget`. Used by the desktop/GTK hosts and
  **reused unchanged on Android** — the Android host (cosmo) renders the UI into a Cairo image
  surface and blits the ARGB buffer to an `ANativeWindow` via GLES, so the on-screen result is
  identical to desktop. The only Android-specific seam is an opt-in `ARTBOARD_CAIRO_FT` font
  path (fontconfig is absent on Android); see `detailed_design.md`. No `IRenderTarget` change.

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
- Hoverable (a single global hover owner; see 2.6).
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

### 2.6 Hover state and routing (FR-24)

Hover is a cross-tree concern like focus, but simpler: a single pointer hovers at most one segment,
so there is one global hover owner (not an index→segment registry). It is derived entirely from the
existing input HAL — no new primitive:

- **Routing.** `Segment::dispatchGesture` routes a bare `Move` (a `Move` with no active press
  capture) down to the deepest hit-tested handler — the same segment a `Down` at that point would
  reach — and delivers it there. Previously a hover `Move` reached only the root; this closes that
  gap so positional controls (`ComboBox` rows, `TabView` tabs) can track the pointer. A `Move`
  during a press still goes to the captured segment.
- **Ownership.** `Segment::setHovered(seg)` sets the one hovered segment and clears the previous
  owner (hover-leave). `advance(nowMs)` eases a per-segment `hoverAmount()` in `[0,1]` (≈120 ms,
  reduced-motion-safe) that controls read to interpolate their hover look — so hover never pops.
  `isHoverWithin()` lets a container (e.g. `ScrollView`) react to hover over its content.
- **Shared treatment.** `ui/base/Interaction.h` defines the one hover appearance (brighten fill,
  pull border toward the control's emphasis colour) plus colour/paint/box lerp helpers, so hover
  reads identically across controls and works for any theme without new theme fields. This is the
  "consistency lock" for interaction feedback.

### 2.6a Kinetic scrolling (FR-29)

`ScrollView` distinguishes three offset regimes: **direct drag** (1:1 with the pointer, with
rubber-band resistance once past `[0, maxOffset()]` — a compressed fraction of the excess, not a
hard clamp), **ballistic fling** (a `Fling` gesture, FR-28, seeds a velocity that decays each
frame by an exponential friction factor until it drops below a stop threshold), and **snap-back**
(once out of range and not being dragged — whether from a released overscroll or a fling that
carried it past the edge — a `Spring`, FR-4d, eases the offset to the nearest boundary, honoring
reduced motion for free since `Spring::advance` already does). Only the first regime is exempt
from FR-25 (the pointer is the animation); the recovery is genuinely spring-driven, reusing the
framework's existing glide-to-target primitive rather than a new hand-rolled integrator.

### 2.7 Animated state transitions (FR-25)

Controls never change a visible property in a single frame. Each interactive control drives its
state through an animation primitive: a `Property`/`AnimatedProperty` for one-shot transitions
(`Button` press, `Checkbox` check grow-in, `TabView` per-tab active fade, `TextBox` focus border +
caret) and a `Spring` for followers (`ProgressBar` level, `LineGraph` series morph, `ComboBox`
gliding row highlight, `ScrollView` scrollbar emphasis). Direct-manipulation transforms that track
the pointer 1:1 (slider/knob drag value, scroll-drag offset, `ImageView` pan/zoom whose
`fittedRect()` is authoritative immediate geometry) are exempt. All of it collapses to the final
state under `reducedMotion()`.

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
- Five additional `Easing` entries (`Standard`, `StandardDecel`, `StandardAccel`,
  `EmphasizedDecel`, `EmphasizedAccel`) are named cubic-bezier curves, evaluated by a generic
  bezier-solve helper (Newton-Raphson + bisection fallback) rather than a closed-form formula,
  so an arbitrary control-point shape is expressible through the same `Easing`/`applyEasing`
  seam as every other curve — no second "curve lookup" mechanism.
- `anim/MotionTokens.h` names a shared duration scale (`kDurationShort1..4/Medium1..4/Long1..4`)
  and `Spring` settle-speed presets (`kSpatialFast/Default/Slow`, `kEffectsFast/Default/Slow`),
  so applications reuse one motion vocabulary instead of hand-picking constants per control. It
  is pure data (header-only, like `Observable`), with no new primitive underneath — durations are
  plain milliseconds and the spring presets are just named `omega` values for the existing
  `Spring` (FR-4d).
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
- It also exposes a **path clip** primitive, `clipPath()`, which intersects the current clip with
  the current path (nonzero winding) instead of an axis-aligned rectangle — the primitive rounded
  rects, circles, and free-form clip regions need, since `clipRect` only covers rectangles. Like
  `clipRect`, it is scoped by `save()`/`restore()` and leaves a cleared ("fresh") path afterward.
  Cairo's `cairo_clip()` clears the path as a side effect already; `Canvas2DTarget` clears it with
  an explicit trailing `beginPath()` (`ctx.clip()` alone does not clear Canvas2D's path) so both
  adapters leave the identical postcondition — the platform-independence trade-off rule in action.
- It also exposes an **opacity-group** primitive, `pushLayer(alpha)` / `popLayer()`, that composites
  everything drawn between the two calls as one group at `alpha` — so overlapping shapes inside the
  layer blend with each other first and only the combined result fades, unlike fading each shape
  individually (which double-blends overlaps). `CairoTarget` maps it directly to
  `cairo_push_group`/`cairo_pop_group_to_source` + `cairo_paint_with_alpha` (which already brackets
  its own save/restore-equivalent state scope). `Canvas2DTarget` has no native group primitive, so
  it builds one from an offscreen `<canvas>` per layer: drawing calls are redirected to the
  offscreen context (copying the destination's current transform/paint state in first) until
  `popLayer()`, which composites the offscreen canvas back with `globalAlpha` under an identity
  transform. `RecordingTarget` records both calls (`PushLayer` carries `alpha`; `PopLayer` takes
  none) so tests can assert the whole bracketed sequence.
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
- **Touch (FR-28).** `RawPointer`/`Gesture` carry a `touch` flag through the same mechanism as
  the existing modifier flags. `GestureRecognizer` gains a time-tick, `advance(nowMs)` (the input
  module's counterpart to `Segment`/`Spring`/`Animator`'s own `advance`), so it can emit a
  `LongPress` after a held, non-dragging press crosses a duration threshold without waiting for
  another pointer event. It also tracks a short rolling window of recent drag samples and, on
  release, emits a `Fling` (carrying a computed velocity) alongside the existing `Drop` when the
  release speed clears a threshold — the seam a kinetic `ScrollView` (FR-29) continues motion
  from. `Segment::dispatchGesture` treats a touch-flagged bare `Move` like any other for hit-test
  routing but does not set ambient hover from it (a touchscreen has no "resting over" concept).

### 3.5 `scene`

- `Drawable` is the retained drawing base.
- `Rectangle`, `Line`, `Polyline`, `Ellipse`, `Path`, and `Text` implement primitive graphics.
- `Artboard` is the scene root for ordered drawing.
- `drawShadow`/`drawElevation` (`Shapes.h`, alongside `drawRoundedRect`/`drawCircle`) paint a soft
  rounded-rect drop shadow composed **only** from the existing linear/radial gradient primitives
  (FR-30) — no blur HAL primitive exists, so this is the platform-free approximation: four
  straight-edge strips (linear gradient) plus four corner wedges (radial gradient, centred at
  each rounded corner's arc centre). The corner wedges are not perfectly seam-continuous with the
  edge strips (a true annulus gradient would fix this but isn't an existing primitive) — a known,
  documented limitation (requirements.md §5), minor in practice and hidden under the object's own
  opaque fill drawn on top.

### 3.6 `ui`

The `ui` module is split by role into two folders, **one class per file** for maintainability:

- **`ui/base/`** — framework foundations and reusable building blocks:
  - `Segment` (composite interactive base; animated group `opacity` fades the whole subtree through
    the HAL `pushLayer`/`popLayer` (FR-32) and animated `rotation`/`scaleX`/`scaleY` about
    `pivotX`/`pivotY` feed `localTransform()` (FR-33); `clipToBounds` clips children via the HAL `clipRect`;
    `snapTo()` constrains one edge to another segment's edge + offset, resolved each `advance()`
    so a segment follows the one it is snapped to; owns hover state + an animated `hoverAmount()`
    and routes bare `Move` gestures to the hovered handler — FR-24),
  - `Interaction` (the one shared hover treatment + colour/paint/box lerp helpers, so every
    control's hover reads identically and works for any theme without new theme fields — FR-24),
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
  - `RectangleSegment`, `CircleSegment`, `LabelSegment`, `PathSegment` (reusable visual nodes;
    `PathSegment` hosts a `Path` via its new `emit()` so path geometry has one implementation, FR-37).
  - `VisualLoop` (FR-34) and `ProgressIndicator` (FR-35): authorable bases that own a custom
    component's *lifecycle/state* and draw nothing, so an application (or a Genesis-generated class)
    subclasses them and supplies only the picture. `ProgressBar` is now one such subclass.
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

- Behavior: value, range, drag semantics, keyboard step adjustments. Optional
  **secondary reference reach** (`setSubValueOffset`/`setSubValueColor`): a coloured
  fill from the thumb to `value + offset` plus a thin end tick, each easing on its own
  follower, for showing a value's effective total when an external contribution is
  stacked on top (e.g. cosmo's group-stacked value; negative offset reaches left).
- Visual composition: track rectangle + range fill rectangle + reach fill + reach tick
  + thumb circle.

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
- Clipping covers axis-aligned rects (`clipRect`) and arbitrary paths (`clipPath`, nonzero
  winding); soft/anti-aliased masks (e.g. a blurred clip edge) are not yet supported.