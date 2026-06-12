# Arstro Artboard

A platform-free 2D drawing framework. Your app builds a **scene of drawables** (shapes, paths,
text — with animation) against one abstract surface; a thin **adapter** renders that surface to a
real device. The drawing core contains **no platform code** — it is a screen HAL for graphics, the
same way an OS HAL abstracts hardware.

```
        your app  ──uses──►  Artboard core  ──emits ops──►  IRenderTarget (HAL)
        (platform-free)      (scene/shapes/anim)                   ▲
                                                                   │ implements
                                                   ┌───────────────┴───────────────┐
                                              Canvas2DTarget (web)      (future: framebuffer,
                                              RecordingTarget (test)     Skia, video frame…)
```

## Design goals

- **Platform-free app.** An Arstro app depends only on the Artboard core and draws through
  `IRenderTarget`. Swapping the device = swapping the adapter, not touching the app.
- **One mechanism, many backends.** Every backend (web Canvas2D today; framebuffer, native,
  **video frames** later) implements the same small `IRenderTarget` interface. The core never
  rasterizes — it emits primitive ops (paths, text, fills); the adapter does the rest.
- **Abstract animation.** Easing + `Animation` (tween) + `AnimatedProperty` are device-agnostic
  and reusable for both live UI and offline video rendering.

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
src/adapter/web/   Canvas2DTarget — the web adapter (WASM + Canvas2D)
include/artboard/artboard.h   aggregate header
tests/         unit tests (100% core coverage) + wasm/node integration
docs/          architecture.puml, design.md
```

The **core** (`core`/`anim`/`render`/`scene`) is platform-free and built into `artboard_core`.
**Adapters** under `src/adapter/<device>/` are selected at build time, so choosing a target picks
its adapter (see the umbrella `arstro` build script; first target: `linux-web-server`).

## Build & test

```bash
cmake -S . -B build && cmake --build build && (cd build && ctest)   # core unit + integration
# web adapter (WASM): built by the arstro umbrella build for the web target (needs emcc)
```

See [docs/design.md](docs/design.md) and [docs/architecture.puml](docs/architecture.puml).
