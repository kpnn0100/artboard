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

The system does not currently include layout managers, font measurement, accessibility APIs, or a
render HAL clipping primitive.

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