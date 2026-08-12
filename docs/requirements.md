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
- Authorable bases for custom animated components: `VisualLoop` (indeterminate lifecycle) and
  `ProgressIndicator` (determinate state), plus protected signal hooks on the basic controls.
- A rectangular and path clip primitive on the render HAL, and clip-to-bounds for segments.
- An opacity-group compositing primitive (`pushLayer`/`popLayer`) on the render HAL, and
  animated per-segment group opacity, rotation, scale, and pivot built on it.
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

### FR-4d Framerate-independent spring follower

The framework shall provide a `Spring` — a critically-damped follower that advances a scalar toward
a target by a real time delta (`advance(dtSeconds, omega)`) using a **closed-form** step, so the
smoothing trajectory is framerate-independent (advancing one large step yields the same result as
many small steps, within tolerance) and never overshoots. It replaces the ad-hoc per-frame Euler
integrators previously hand-copied into individual controls (a single source of truth for display
smoothing). A `Spring` shall have no backend or timing-source dependency.

### FR-4e Reduced-motion switch

The framework shall expose a global accessibility switch (`setReducedMotion(bool)` / `reducedMotion()`)
that, when enabled, collapses **all** framework motion to instant: a `Spring` jumps to its target and
an `AnimatedProperty` snaps to the tween's resting value (firing `onComplete`). This lets a host that
detects a user "reduce motion" preference present final states with no animation. The default is off.

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
- `Slider` — a horizontal ranged control; drag/click/arrow-keys set the value and emit
  `onChange(value)` (only on user interaction, never on a programmatic `setValue`), matching
  `Knob`. Double-click resets to default (FR-9a). `setClickJumps(false)` makes the value change
  **only on drag** (a bare press/click no longer jumps to the cursor); the default is `true`
  (a click jumps to the cursor). A click-to-position jump is **deferred** and committed in
  `advance()` only after a guard elapses; the guard MUST be **>= the GestureRecognizer
  double-click window** (default 300ms) so the jump cannot commit between the two clicks of a
  slow double-click. Both the double-click itself AND the next press (the second click's Down)
  cancel the pending jump, so a double-click resets to default cleanly and never flashes the
  value toward the cursor first, at any double-click speed. Dragging sets the value immediately.
  Like `Knob`, the Slider's **displayed**
  thumb/fill is a spring-smoothed follower of the real value (FR-9a), so a click or reset glides
  to the new position instead of snapping, while `onChange` still reports the final value.
  `setTrackGradient(left,right)` renders the track as a horizontal gradient (e.g. a temperature
  blue->yellow ramp) instead of the solid track + accent fill, so the slider previews what each
  end of the range looks like; the thumb still marks the position. When the range spans zero
  (`minimum() < 0 < maximum()`), the range fill is anchored at the zero-crossing instead of the
  left edge -- it grows right for positive values and left for negative ones, so the fill reads
  as *distance from neutral* rather than *distance from the minimum*. A range that doesn't span
  zero (e.g. `0..100`) fills from the left edge, unchanged.
  `setSubValueOffset(offset)` (+ `setSubValueColor`) draws an optional **secondary reference
  reach**: a coloured fill from the thumb to `value + offset` plus a thin end tick, each
  spring-eased on its own follower (so both glide, no snap). The thumb still marks the *own*
  value; the reach shows the *effective* total when an external contribution is added on top
  (e.g. cosmo's group-stacked value). A negative offset reaches left; `0` hides it.
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
  selecting one closes it and emits `onChange(index)`. The open list is drawn in the **overlay
  pass** (`onOverlay`) over an opaque backing, so it sits on top of every other control and is
  never clipped by its owning panel; opening also `raise()`s the box so dropdown clicks are
  hit-tested before sibling controls beneath the list. Opening and closing **animate** the list
  (fade + short downward slide) via an `AnimatedProperty` advanced each frame; logical open state
  (used for hit-testing) flips immediately so the list is clickable during the reveal. Honors the
  reduced-motion switch (FR-4e).

### FR-OVERLAY Overlay render pass
`Segment::renderOverlay()` is a second tree traversal the app runs on the root after
`render()`. It composes transforms exactly like `render()` but calls `onOverlay()` (default
empty) and applies **no clipping**, so popups/dropdowns escape their parent's bounds and draw
above the entire scene. `Segment::raise()` moves a segment to the end of its parent's child
list (drawn last among siblings, hit-tested first).
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
existing one for that source). A **newly added** routing's ring **grows in** from zero
depth (a `Spring`, FR-4d), so a freshly assigned modulation animates outward instead of
appearing at full size; re-colouring an already-routed source does not re-animate. The
grow-in honors the reduced-motion switch (FR-4e). Assignment of a source to a target
(drag-and-drop) is performed by the application, which then calls `addModulation`.

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
can align to the displayed image. It also supports **zoom + pan**: `zoomAbout(factor,
local)` multiplies the zoom (clamped 1..8) keeping the image point under `local` fixed
and clamps the pan so the image still covers the view; `fittedRect()` reflects the
zoomed/panned rect (so overlays follow), and the draw is clipped to the view so a
zoomed image never spills out. `resetView()` restores 1x. When zoomed (>1x) the view
is **interactive**: it hit-tests true and a drag pans the image (`panBy(dx,dy)`,
clamped to keep the image covering the view); at 1x it is display-only (click-through).
`ImageView` is platform-free (emits only the HAL primitives above).

### FR-20 Pointer modifiers (Alt / Shift / Ctrl)

`RawPointer` and the synthesized `Gesture` carry boolean **`alt`**, **`shift`**, and **`ctrl`**
flags (modifier keys held). The adapter sets them on each raw event; the `GestureRecognizer`
carries the current event's state onto every emitted gesture. This lets controls offer
modifier affordances without a separate keyboard channel: alt-drag (e.g. bezier tangent
handles), shift-click (range selection), and ctrl/cmd-click (toggle selection) in lists such
as a filmstrip.

### FR-21 Tab/content unification

`TabView` shall render the **active** tab as visually continuous with the page below it: the
active tab is full height and extends past the tab strip (the page, drawn on top, covers the
overhang), while inactive tabs are recessed (shorter, inset). The page sits directly under the
strip (no gap). The result reads as one connected surface for the selected section.

`TabStyle` may additionally carry an **active-tab indicator**: a thin bar of
`activeIndicatorColor` drawn across the top edge of the active tab only, `activeIndicatorHeight`
tall. Default height is `0` (no indicator drawn), so existing themes are unaffected until they
opt in. `TabStyle::labelActive` colours the active tab's title separately from `label` (the idle
title colour); it defaults to a copy of `label` so an uncustomized theme reads identically.

### FR-22 Text font family and letter-spacing

`IRenderTarget::drawText` shall accept an optional **font family** name and an optional
**letter-spacing** (extra advance between glyphs, in px). Both default to the framework's
prior behavior (empty family = adapter's generic sans; `0` spacing = normal tracking), so
every existing 4-argument call site is unaffected.

- **Font family** selects a family name the adapter's own text stack resolves — Fontconfig on
  the native (Cairo) adapter, the CSS font stack on the web (Canvas2D) adapter. The HAL does
  **not** load font files itself (out of scope: that is host/adapter bootstrap, e.g. an app
  registering its own bundled fonts via Fontconfig before creating its window). A distinct
  **static weight** (Medium, SemiBold, ...) is selected by passing *that weight's own family
  name* (e.g. `"DM Sans Medium"`), not a separate weight enum — real static weights are
  distinct font files/family names at the OS text-stack level, and Cairo's built-in weight
  enum only distinguishes two values, so a numeric weight parameter would not reliably select
  the intended glyphs anyway.
- **Letter-spacing** adds `letterSpacingPx` of extra advance after every glyph (uppercase
  "tracking" is a common request for small section-header labels). An adapter with no native
  tracking support falls back to drawing glyph-by-glyph with manual advance.
- `ui::TextStyle` (and the `scene::Text` drawable) carry the same two fields so `LabelSegment`
  and any control built on a `TextStyle` (`Button`, `Checkbox`, `TextBox`, `TabView`,
  `ComboBox`, `Knob`, ...) can opt into a themed family/tracking without every control
  re-deriving its own text-drawing code.

### FR-23 Observable state link

`ui::Observable<T>` shall hold one value and notify registered observers when it changes, so
several UI nodes can bind to ONE source of truth instead of each caching its own copy of a
shared state (the class of bug where a toggle button's highlight and the panel it controls
disagree — e.g. the rail is open but its toggle isn't highlighted).

- `get()` returns the current value; `set(v)` replaces it and, **only if the value actually
  changed**, notifies every observer in registration order. Setting the current value is a
  no-op, so two observers that write back into the same `Observable` cannot recurse forever.
- `observe(fn, fireNow = true)` registers an observer; by default it fires immediately with
  the current value so the view initialises IN SYNC (fixing start-up drift). A null observer
  is ignored (never stored, never called), so `set()` never has to guard for one.
- Platform-free and drawing-free: it is a value plus a list of callbacks — no HAL, no OS. It
  sits beside `Property` (an animated scalar) and `ModBus` (a modulation-value bus) as the
  third small state primitive in `ui/base`; the framework provides the primitive, callers wire
  which fields (a button's `active`, a panel's `visible`/target width) observe it.

### FR-24 Pointer hover state and routing

The framework shall track a **hover** state so any interactive control can render a distinct
appearance while the pointer rests over it (without a press). Hover is platform-free (no HAL
change): it is derived from the existing `Gesture::Type::Move` stream the adapter already feeds.

- **Routing.** A bare `Move` (no active press capture) shall be routed by `Segment` down to the
  **deepest hit-tested handler** under the cursor — the same segment a `Down` at that point would
  reach — and delivered to that segment's gesture handler so positional controls (e.g. a
  `ComboBox` row, a `TabView` tab) can track *where* the pointer is. This closes a prior gap in
  which a hover `Move` reached only the root and never the control under the cursor. A `Move`
  emitted *during* a press (before the drag threshold) continues to go to the captured segment,
  unchanged.
- **Ownership.** At most **one** segment is hovered at a time (a single pointer). Moving onto a
  segment sets its hover and clears the previously-hovered one (hover-leave); moving onto empty
  background clears any control's hover. Hover shall never be set on a `disabled`/hidden segment.
  A destroyed segment relinquishes hover so no dangling hover owner remains.
- **Animated hover factor.** `Segment` shall expose an animated `hoverAmount()` in `[0,1]` that
  eases toward `1` while hovered and `0` otherwise (short, ≈120 ms, eased), advanced once per
  frame in `advance(nowMs)`. It honors the reduced-motion switch (FR-4e): under reduced motion it
  snaps to its endpoint. Controls read `hoverAmount()` to interpolate their hover appearance so a
  hover never pops on or off.
- **Consistent treatment.** A single shared hover treatment (brighten the fill, pull the border
  toward the control's emphasis colour) is defined once (`ui::interaction`) and reused by every
  control, so hover reads identically framework-wide and works for any theme without new theme
  fields (consistency lock). Every clickable control — `Button`, `Checkbox`, `ToggleSwitch`,
  `Slider`, `Knob`, `ComboBox` (field + a gliding per-row highlight), `TabView` (per-tab),
  `ScrollView` (scrollbar thumb), `TextBox` — shall present this animated hover feedback.

### FR-25 Animated state transitions (no snapping)

Every visible state change in a control shall reach its new value through an animation primitive
(`AnimatedProperty`/`Property`/`Spring`), never by assigning the visible value in one frame, and
shall collapse to the final state instantly only under reduced motion (FR-4e). Direct-manipulation
tracking (a transform that follows the pointer 1:1 — the slider/knob drag value, scroll-drag
offset, and the `ImageView` pan-drag / zoom-about-a-point, whose `fittedRect()` must remain the
authoritative *immediate* geometry that overlays and hit-testing align to) is exempt, because the
pointer itself is the animation. Concretely:

- `Button` — the press/idle appearance **crossfades** (it does not swap in one frame).
- `Checkbox` — the check indicator **grows in / out** (it does not pop).
- `ToggleSwitch` — the track colour **blends** continuously between off and on with the thumb
  (no hard swap at the midpoint).
- `TabView` — selecting a tab **animates** the active/idle transition (tab geometry + colour ease;
  an active indicator glides to the selected tab).
- `TextBox` — the focus border **blends** in/out and the caret **fades** (it does not pop).
- `ProgressBar` — the displayed level **eases** toward the set value.
- `LineGraph` — the plotted series **morphs** toward a newly set series (fast, so live/streaming
  data still tracks); a change in point count lands immediately (a morph across differing counts
  is ill-defined).

### FR-26 Path clip primitive

The render HAL shall provide a path clip primitive `clipPath()` that intersects the current clip
region with the **current path** (built by `beginPath`/`moveTo`/`lineTo`/`quadTo`/`cubicTo`/
`closePath`, per FR-16), using the nonzero winding rule, in the current transform space. Like
`clipRect` (FR-11), the clip is scoped by the `save()`/`restore()` state stack. After `clipPath()`
the current path is cleared — the same "fresh path" postcondition `clipRect` already leaves — so
drawing after a clip starts from empty. This is a HAL extension because an arbitrary clip region
(rounded-rect, circular/squircle icon mask, free-form shape) cannot be expressed by `clipRect` or
by any fill/stroke/path combination alone — only a true clip primitive restricts where later
drawing is visible.

### FR-27 Opacity layer (group compositing)

The render HAL shall provide an opacity-group primitive: `pushLayer(alpha)` begins redirecting
all subsequent drawing into an intermediate group, and the matching `popLayer()` composites that
whole group into the destination at `alpha` in one operation. Overlapping shapes drawn inside the
layer therefore blend with each other at full opacity first, and only the combined result fades
by `alpha` — unlike drawing each shape individually at reduced alpha, which double-blends any
overlap. `pushLayer`/`popLayer` bracket their own graphics-state scope (transform, clip, and paint
changes made inside do not leak past the matching `popLayer()`), mirroring `save()`/`restore()`.
Calls nest: each `pushLayer` must be matched by exactly one `popLayer()`, innermost-first. This is
a HAL extension because compositing an overlapping group as one unit at a shared alpha cannot be
expressed by per-primitive fill/stroke alpha alone.

### FR-31 Motion token vocabulary

The framework shall expose a shared named vocabulary of durations, easing curves, and spring
settle-speed presets so applications share one motion language instead of hand-picking ad hoc
constants per control:

- `Easing` gains five additional named cubic-bezier curves — `Standard`, `StandardDecel`,
  `StandardAccel`, `EmphasizedDecel`, `EmphasizedAccel` — evaluated by solving the bezier's
  `x(u) = t` for the parameter `u` (Newton-Raphson with a bisection fallback) and returning
  `y(u)`, so arbitrary (not just the existing closed-form) easing shapes are expressible; they
  honor the same `[0,1]` input clamp and `0`-at-`0`/`1`-at-`1` pinning as every other curve
  (FR-4a).
- `motion::kDurationShort1..4`, `kDurationMedium1..4`, `kDurationLong1..4` name a short/medium/
  long millisecond duration scale.
- `motion::kSpatialFast/Default/Slow` and `motion::kEffectsFast/Default/Slow` name `Spring`
  settle-speed (`omega`) presets — "spatial" for position/size motion, "effects" for fades/colour
  (faster) — reusing `Spring`'s existing critically-damped model (FR-4d) rather than adding a new
  motion primitive.

### FR-30 Elevation shadow helper

The framework shall provide a `drawShadow(target, rect, cornerRadius, color, blurPx, offsetX,
offsetY)` core helper that paints a soft drop shadow for a rounded rect, composed **only** from
the existing linear- and radial-gradient fill primitives (FR-13, FR-17) — four straight-edge
strips (a linear gradient, full `color` alpha at the shadowed rect's edge fading to zero at
`blurPx` beyond it) and four corner wedges (a radial gradient centered at each rounded corner's
arc centre, from `color` at the centre fading to zero at `cornerRadius + blurPx`). It draws only
the shadow layer — the caller draws the object's own opaque fill on top afterward (e.g. via
`drawRoundedRect`), which covers the wedge's inner region where the two gradients do not
perfectly agree (see Constraints, §5). A convenience `drawElevation(target, rect, cornerRadius,
elevationDp)` paints a two-layer Material-style shadow (a tighter, darker "key" layer plus a
softer, lighter "ambient" layer) scaled by `elevationDp`, both via `drawShadow`. This is
platform-free (no HAL change): it is expressible entirely from primitives every adapter already
implements, so it renders identically everywhere per NFR-1.

### FR-28 Touch input: velocity, fling, and long-press

`RawPointer` carries a `touch` flag (adapter-set; false for mouse/pointer input, true for a
touchscreen source) alongside its existing `alt`/`shift`/`ctrl` modifiers; `GestureRecognizer`
carries it onto every synthesized `Gesture` the same way. This is the seam a touch-first shell
needs: a control can tell "no ambient hover exists for this event" without a separate input
channel.

- **Long press.** The recognizer gains `advance(nowMs)`, ticked once per frame by the host
  (mirroring `Segment`/`Spring`/`Animator`). While a press is held without crossing the drag
  threshold, `advance` emits one `Gesture::Type::LongPress` at the press position after
  `longPressMs` (default 500ms, `setLongPressMs`) elapses; it fires at most once per press. A
  press that long-presses does not also emit `Click`/`DoubleClick` on release (only `Up`).
- **Fling.** While dragging, the recognizer keeps a short rolling window (default 100ms,
  `setVelocityWindowMs`) of recent `(time, position)` samples. On release, if the window spans a
  measurable time and the resulting speed exceeds `flingVelocityThreshold` (default 400px/s,
  `setFlingVelocityThreshold`), the recognizer emits `Gesture::Type::Fling` (carrying the
  computed `velocity` in px/s, in addition to the existing terminal `Drop`) so a kinetic
  scrolling control (FR-29) can continue the motion after release.
- **Touch suppresses ambient hover (FR-24 addendum).** A bare `Move` with `touch == true` is
  still routed to the deepest hit-tested handler (so a control can react to raw touch position),
  but `Segment::dispatchGesture` does **not** call `setHovered` for it — a touchscreen has no
  ambient "pointer resting over a control" concept, so a touch drag must not leave a control
  looking permanently hovered afterward.

### FR-29 Kinetic scrolling

`ScrollView` shall continue scrolling after a `Fling` gesture (FR-28) with framerate-independent
exponential velocity decay (each frame, `velocity *= frictionPerSecond^dt`; stops once below a
small threshold), and shall allow the offset to move **past** `[0, maxOffset()]` during an active
drag with **rubber-band resistance** (the displayed excess is a fraction of the raw pulled
distance) rather than hard-clamping — both a direct drag and a fling may overshoot the range.
Once released (or once ballistic motion first carries it out of range), the offset **eases back**
to the nearest boundary via the existing `Spring` (FR-4d, not a hand-rolled per-frame integrator),
honoring reduced motion (an instant snap, per FR-4e — `Spring::advance` already does this).
Direct-manipulation drag/rubber-band positioning remains exempt from FR-25 (the pointer itself is
the animation, per its existing exemption clause); only the post-release recovery is
spring-driven.

### FR-32 Segment group opacity

`Segment` shall expose an animated `opacity` property (an `artboard::Property`, default `1.0`,
meaningful range `[0,1]`) that fades the segment **and its whole child subtree as one group**.

- When the effective opacity is `>= 1`, rendering is unchanged (no layer is opened, so the common
  fully-opaque case costs nothing).
- When it is strictly between `0` and `1`, `render()` shall bracket the segment's own paint **and**
  all of its children in the existing `pushLayer(alpha)` / `popLayer()` HAL primitive (FR-27), so
  overlapping descendants blend with each other at full opacity first and only the combined result
  fades. Fading each descendant's colours individually is explicitly **not** equivalent: it
  double-blends every overlap.
- When it is `<= 0` (within `kOpacityEpsilon`), the segment and its subtree shall not be drawn at
  all, and `hitTest` shall return `false` — a faded-out panel must not keep swallowing pointer
  input. A partially transparent segment remains hit-testable.
- Nested opacities compose multiplicatively, because a child's own layer composites into its
  parent's layer.
- `renderOverlay()` (the second, unclipped pass) shall honor opacity by the same rules, so a fading
  popup fades its overlay content too.
- `advance(nowMs)` shall tick the property, so `opacity.animate(...)` / `animateTo(...)` work like
  every other animated segment field and honor reduced motion (FR-4e).

This satisfies FR-25 (animated state transitions) for show/hide: a segment appears and disappears by
fading its opacity, never by flipping `visible` in a single frame.

### FR-33 Segment animated rotation, scale, and pivot

`Segment` shall expose animated `rotation` (radians, default `0`), `scaleX` / `scaleY` (default
`1`), and `pivotX` / `pivotY` (local-space pivot, default `0,0`) properties, composed into
`localTransform()` as:

`translate(x, y) · translate(pivotX, pivotY) · rotate(rotation) · scale(scaleX, scaleY) ·
translate(-pivotX, -pivotY) · transform`

so a segment rotates and scales **about its own pivot** while the free `Drawable::transform` field
remains the innermost, caller-owned transform (existing behaviour is preserved exactly when all five
new properties are at their defaults). Because hit testing already maps world points through
`worldTransform().inverse()`, a rotated or scaled segment is hit-tested in its rotated/scaled frame
with no extra code. `advance(nowMs)` shall tick all five properties.

This gives FR-25-compliant motion for the transform channel: spin, pop, and squash animations are
animated properties, not per-frame transform arithmetic re-derived by every application.

### FR-34 VisualLoop authorable base

The framework shall provide `VisualLoop`, a `Segment` subclass that owns the **lifecycle** of an
indeterminate, looping visual (spinner, busy pulse, loading screen) and draws nothing itself:

- `start(nowMs)` begins the loop and fires `onLoopStart()`; it is idempotent while running.
  `stop(nowMs)` ends it and fires `onLoopEnd()`; it is a no-op when stopped. `start()` resets the
  cycle counter.
- `setCycleMs(ms)` sets the loop period. Each completed period fires `onCycle(index)` exactly once,
  counting from 1 — including when several periods elapse inside a single long frame, so
  `cycleCount()` stays exact and a subclass keyed to `onCycle` never skips a beat. A period `<= 0`
  disables cycle signals; the loop still runs.
- `cyclePhase()` reports progress through the current cycle in `[0,1)`; `elapsedMs()` reports time
  since `start()`. Both read `0` when stopped.
- `now()` (protected) exposes the last host timestamp so a subclass can pass it straight to
  `Property::animate` from inside a signal.
- It is `inputTransparent` by default: a busy indicator is chrome, not a control.

The class shall contain no timing source of its own — the host's `advance(nowMs)` is the only clock,
keeping it platform-free and deterministic under test.

### FR-35 ProgressIndicator authorable base

The framework shall provide `ProgressIndicator`, the determinate counterpart to FR-34: a `Segment`
subclass that owns progress **state** and draws nothing.

- `setValue(v)` clamps to `[0,1]`, retargets a `Spring` display follower (FR-4d) so the drawn level
  eases and never snaps, and fires `onValueChanged(v)`. `onComplete()` is edge-triggered: it fires
  once when the value first reaches `1`, and re-arms when the value drops below `1`.
- `displayValue()` is the spring-smoothed level a subclass paints; `setDisplayOmega()` tunes the
  settle speed from the FR-31 motion tokens.
- `setIndeterminate(on)` switches modes and fires `onIndeterminate()` / `onDeterminate()` on the
  edge only. While indeterminate, `phase()` free-runs in `[0,1)` at `periodMs()` per sweep; a
  period `<= 0` freezes it.
- It is non-interactive (`hitTestSelf` is false): progress is a readout, not a control.

`ProgressBar` shall be re-based on `ProgressIndicator` and retain only its appearance, gaining an
indeterminate look in which a shuttle of `setShuttleFraction()` of the track width sweeps across and
is clipped to the track. This is the same separation of state from appearance that FR-9 already
requires of `AbstractSlider`/`Slider`.

### FR-36 Control signal hooks

Every interactive control shall expose its state changes as **protected virtual hooks** in addition
to the public `std::function` callbacks a caller subscribes to. The callbacks serve *users* of a
control; the hooks serve *subclasses* of it, which is the authoring model a generated or
hand-written custom control needs. Hooks fire before the corresponding public callback so a
subclass's own state is settled when the caller's handler runs.

- `Segment`: `onHoverChanged(bool)` and `onFocusChanged(bool)`, both edge-triggered. No signal is
  emitted while a segment is being destroyed.
- `Button`: `onPressDown()`, `onRelease()`, `onCancel()` (press abandoned), `onClicked()` (pointer
  or keyboard confirm).
- `Slider`: `onDragStart()` (once per drag, not per move), `onValueChanged(v)`, `onDragEnd()`.
  Every value-changing path — drag, deferred click-jump, keyboard step, double-click reset — routes
  through one internal notifier so the hook and the public callback cannot diverge.
- `Checkbox`: `onCheckedChanged(bool)`, fired by user interaction only, not by programmatic
  `setChecked()`.

### FR-37 PathSegment

The framework shall provide `PathSegment`, a `Segment` that hosts a `Path` drawable so freeform
geometry is a first-class interactive node alongside `RectangleSegment`, `CircleSegment`, and
`LabelSegment` — able to participate in layout, hover, group opacity (FR-32), and the transform
channel (FR-33). The path is built in the segment's local space.

To avoid a second implementation of path emission, `Path` shall expose `emit(target)`, which writes
its ops and paint into the target's *current* transform space without touching graphics state;
`Path::onDraw` and `PathSegment::onPaint` both call it. `Path` shall also expose `clear()` and
`segmentCount()` so authored geometry can be rebuilt and inspected.

### FR-38 Text entry caret and editing

`TextBox` shall maintain a caret position within its text and support the editing gestures a
single-line field is expected to have, so a field holding a real value (a path, an
expression, a name) can be corrected rather than only retyped from the end:

- Typed text is inserted **at the caret**, which then advances past it.
- Backspace deletes the codepoint **before** the caret; Delete deletes the one **after**.
  Neither ever splits a multi-byte UTF-8 codepoint.
- Left/Right move the caret by one codepoint; Home/End move it to the ends. All four clamp
  to the text.
- A press positions the caret at the nearest inter-character boundary to the pointer,
  measured with `IRenderTarget::measureText` (FR-22) so it lands where the glyphs actually
  are on the adapter in use, rather than at an estimate.
- The caret is drawn at its position (not always at the end) and continues to fade with
  focus. Setting `text` programmatically clamps the caret into range.

`readOnly` shall continue to reject every mutation while still allowing focus and caret
movement, so a value can be read and inspected without being changed.

### FR-39 Control text fits its box

No control shall draw text outside its own bounds.

- `TextBox` shall clip its content to its bounds and **scroll horizontally to keep the caret
  visible**: when the caret would fall outside the padded field, the text offset shifts by
  the minimum amount that brings it back inside. A value longer than the field therefore
  stays fully editable instead of spilling into whatever is drawn beside it.
- `ComboBox` shall shorten its selected-option label with a trailing ellipsis when the label
  does not fit between the field's left padding and its caret triangle, measured with
  `IRenderTarget::measureText` (FR-22).

Both fit against the target's own metrics, so a label that fits on one adapter is not
clipped on another.

### FR-40 Disabled state rendering

`Segment` shall expose an animated `disabledAmount()` in `[0,1]` that eases whenever
`enabled` changes (reduced-motion safe, like `hoverAmount()`), and every themed control
shall dim its drawn colours by it — a disabled control must LOOK unavailable, not merely
ignore input. A control that renders identically enabled and disabled is an unfinished
state, not a styling preference.

### FR-41 Suppressing a control's built-in appearance

A composed control (`Button`, `Checkbox`, `Slider`, `TextBox`, `ToggleSwitch`, `ComboBox`,
`ProgressBar`, `LineGraph`) supplies both BEHAVIOUR and a default APPEARANCE. A subclass
that draws its own appearance — a themed control, or one generated from a design tool —
needs the behaviour without the default picture, or the two pile up on top of each other.

`Segment` shall therefore expose `drawsBuiltInVisuals` (default `true`). When it is false,
a control shall contribute **no appearance of its own**: controls composed from child visual
nodes hide those nodes, and self-drawn controls skip their `onPaint`. Behaviour — hit
testing, gestures, value/press/check state, signals (FR-36) — is unaffected, because the
appearance and the behaviour are separate concerns.

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
- `drawShadow`'s corner wedges use a point-centred radial gradient (the only gradient the HAL
  offers), so the exact alpha at the rounded corner's arc is not perfectly continuous with the
  adjacent straight-edge strip's linear gradient at their shared seam (a true "annulus" gradient,
  which would fix this, is not an existing primitive). The visible effect is minor for typical
  blur/corner-radius ratios and is hidden under the object's own opaque fill; a true blur/annulus
  primitive would remove it but is out of scope here (composing from existing primitives only).

## 6. Verification Outline

- Unit tests verify recursive rendering, focus routing, gesture capture, and baseline control
  behavior.
- Architecture and detailed design documents trace requirements to concrete classes and packages.