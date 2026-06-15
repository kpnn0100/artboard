# Arstro Artboard

A platform-free 2D drawing and UI framework. Your app builds a **scene of drawables** for pure
graphics and a **tree of segments** for interactive controls against one abstract surface; a thin
adapter renders that surface to a real device. The drawing core contains **no platform code** — it
is a screen HAL for graphics and input, the same way an OS HAL abstracts hardware.

```
           your app  ──uses──►  Artboard core  ──emits ops──►  IRenderTarget (HAL)
           (platform-free)      (scene + ui + anim)                     ▲
                                    │ implements
                             ┌───────────────┴───────────────┐
                           Canvas2DTarget (web)      RecordingTarget (test)
```

## Design goals

- **Platform-free app.** An Arstro app depends only on the Artboard core and draws through
  `IRenderTarget`. Swapping the device = swapping the adapter, not touching the app.
- **One mechanism, many backends.** Every backend (web Canvas2D today; framebuffer, native,
  **video frames** later) implements the same small `IRenderTarget` interface. The core never
  rasterizes — it emits primitive ops (paths, text, fills); the adapter does the rest.
- **Abstract animation.** Easing + `Animation` (tween) + `AnimatedProperty` are device-agnostic
  and reusable for both live UI and offline video rendering.
- **Composite UI.** `Segment` adds recursive composition, focus routing, animated layout
  properties, and themeable controls without coupling UI behavior to a specific backend.

## Layout

```
src/core/      Geometry (Point/Size/Rect/Transform), Color           — value types
src/anim/      Easing, Animation, AnimatedProperty                   — abstract animation
src/render/    IRenderTarget (the OUTPUT HAL), RecordingTarget (test/serialize)
src/input/     RawPointer/Gesture, GestureRecognizer, InputRouter    — the INPUT HAL
               (adapter feeds raw down/up/move+button; core derives click /
               double-click / right-click / drag / drop, then hit-tests + routes)
src/scene/     Drawable + shapes (Rectangle, Line, Polyline, Ellipse,
               Path[bezier/spline], Text), Artboard (scene root)
src/ui/base/      Segment, InputController, Property, Theme, AbstractSlider,
                  Rectangle/Circle/Label segments     — foundations (one class per file)
src/ui/concrete/  Button, Slider, Checkbox, TextBox, Knob, ToggleSwitch,
                  ProgressBar, ComboBox, TabView, ScrollView, LineGraph — controls/widgets
src/adapter/web/   Canvas2DTarget — the web adapter (WASM + Canvas2D)
include/artboard/artboard.h   aggregate header
tests/         unit tests (100% core coverage) + wasm/node integration
docs/          requirements.md, architecture.md, detailed_design.md, architecture.puml
```

The **core** (`core`/`anim`/`render`/`input`/`scene`/`ui`) is platform-free and built into
`artboard_core`. **Adapters** under `src/adapter/<device>/` are selected at build time, so choosing
a target picks its adapter (see the umbrella `arstro` build script; first target:
`linux-web-server`).

## Build & test

```bash
cmake -S . -B build && cmake --build build && ./build/artboard_tests
# web adapter (WASM): built by the arstro umbrella build for the web target (needs emcc)
```

Documentation follows a V-model split:

- [docs/requirements.md](docs/requirements.md) — software requirements
- [docs/architecture.md](docs/architecture.md) — software architecture
- [docs/detailed_design.md](docs/detailed_design.md) — software detailed design
- [docs/architecture.puml](docs/architecture.puml) — PlantUML class/package view
