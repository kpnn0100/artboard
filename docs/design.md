# Arstro Artboard — Design

## The HAL: `IRenderTarget`

The single seam between the platform-free core and any device. A **minimal** interface (ISP):
graphics state, a path API, and text. Shapes express themselves as paths + text, so an adapter
only implements these primitives — never per-shape code.

```cpp
struct IRenderTarget {
  virtual void save() = 0;  virtual void restore() = 0;
  virtual void setTransform(const Transform&) = 0;          // world transform
  virtual void setFill(const Color&) = 0;
  virtual void setStroke(const Color&, double width) = 0;
  virtual void beginPath() = 0;
  virtual void moveTo(double,double) = 0;  virtual void lineTo(double,double) = 0;
  virtual void quadTo(double,double,double,double) = 0;     // 1 control point
  virtual void cubicTo(double,double,double,double,double,double) = 0; // 2 control points
  virtual void closePath() = 0;
  virtual void fillPath() = 0;  virtual void strokePath() = 0;
  virtual void drawText(const std::string&, double x, double y, double sizePx) = 0;
};
```

Implementations:
- **`RecordingTarget`** — records every call as a `DrawOp`. Used by unit tests (assert the exact
  primitive stream) and as a serialization point (replay/inspect). Platform-free → 100% testable.
- **`Canvas2DTarget`** (`src/adapter/web/`) — maps each call to an HTML Canvas2D context via
  Emscripten. This is the device adapter for the web target.
- *Future:* framebuffer, native, or a **video-frame** target (same interface → the scene renders
  to encoded frames instead of a screen).

## Scene & shapes

`Drawable` is the base: a local `Transform`, visibility/opacity, and `onDraw(IRenderTarget&)`.
`render()` brackets `onDraw` with `save()/setTransform/restore`. Concrete shapes
(`Rectangle` with corner radius + border, `Line`, `Polyline`, `Ellipse`, `Path`, `Text`) only
emit path/text ops — no rasterization. `Path` supports line/quad/cubic segments and a
Catmull-Rom **spline → cubic bezier** helper. `Artboard` is the scene root (size, background,
ordered children); `render(target, nowMs)` advances animation then draws the tree.

## Abstract animation

- `Easing` — pure functions (linear, quad/cubic in/out/in-out, …) over t∈[0,1].
- `Animation<double>` (a tween) — `from`, `to`, `durationMs`, `easing`; `value(elapsedMs)` clamped,
  `finished(elapsed)`. Device- and time-source-agnostic (you pass elapsed/now).
- `AnimatedProperty` — a value that may have an active tween; `update(nowMs)` advances it. Shapes
  hold these for animated position/opacity/etc. The same primitives drive offline video later.

## Input HAL (symmetric with the render HAL)

Output abstracts *drawing*; input abstracts *pointing*. A platform adapter feeds only the
**lowest-level** facts it has — pointer `Down`/`Up`/`Move` with a button and a timestamp — into a
`GestureRecognizer`. The recognizer (pure, platform-free) synthesizes the high-level gestures every
UI needs: **Click, DoubleClick, RightClick, DragStart, Drag, Drop** (plus raw Down/Up/Move). So a
device adapter never implements click/double-click/drag logic — exactly as it never rasterizes.

```cpp
enum class PointerButton { Left, Right, Middle };
struct RawPointer { enum class Kind { Down, Up, Move }; Kind kind; Point pos; PointerButton button; double timeMs; };
struct Gesture    { enum class Type { Down, Up, Move, Click, DoubleClick, RightClick, DragStart, Drag, Drop };
                    Type type; Point pos; Point start; PointerButton button; };

GestureRecognizer r;            // feed raw events; sink receives synthesized Gestures
r.setSink([](const Gesture&g){ ... });
r.feed({RawPointer::Kind::Down, {x,y}, PointerButton::Left, t});
```

Hit-testing + routing is a second small unit:

```cpp
struct InputTarget { virtual bool hitTest(const Point&) const; virtual void onGesture(const Gesture&); };
class  RectTarget : InputTarget { Rect bounds; std::function<void(const Gesture&)> handler; };  // convenience
class  InputRouter { add(InputTarget*); route(const Gesture&); };  // z-order hit-test + press capture
```

`InputRouter` delivers a press's whole sequence (Down → Moves/Drag → Up/Drop) to the **captured**
target hit on Down, and hit-tests post-press gestures (Click/DoubleClick/RightClick) fresh — so a
knob keeps receiving drag samples even if the pointer leaves its bounds. The app wires
`RectTarget`s (a key → note on Down/off on Up; a knob → value on Drag, default on DoubleClick,
random on RightClick). Adapters (web today) only translate native events to `RawPointer`.

## SOLID

- **SRP:** value types, animation, the HAL, and the scene are separate units.
- **OCP:** new shape = new `Drawable`; new device = new `IRenderTarget` adapter. Core unchanged.
- **LSP:** every shape honours `onDraw(IRenderTarget&)`; every adapter honours `IRenderTarget`.
- **ISP:** the HAL is the minimal primitive set shapes actually need.
- **DIP:** the app and shapes depend on `IRenderTarget`, never on a concrete backend.

## Build selection

`artboard_core` is platform-free. Adapters live under `src/adapter/<device>/` and are compiled in
only for the matching target, so the umbrella build picks the adapter from the chosen target
(first target: `linux-web-server` → web/Canvas2D via WASM).
