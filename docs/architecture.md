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

- `Animation` defines a pure time-based scalar tween.
- `AnimatedProperty` stores runtime animation state.
- `ui::Property` wraps `AnimatedProperty` for segment-level use.

### 3.3 `render`

- `IRenderTarget` is the only output seam.
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

- `Segment` is the composite interactive base class.
- `InputController` is an abstract input strategy for reusable behavior injection.
- `Theme` contains baseline concrete visual styles.
- `RectangleSegment`, `CircleSegment`, and `LabelSegment` are reusable visual nodes.
- `Button`, `Slider`, `Checkbox`, and `TextBox` are concrete controls.

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

## 5. SOLID Mapping

- SRP: render HAL, input HAL, scene graphics, UI composition, and control state are distinct units.
- OCP: new adapters, segments, and controls can be added without modifying the core abstractions.
- LSP: any `Drawable` can render, and any `Segment` can participate in the same render/input flow.
- ISP: `IRenderTarget` and `InputController` stay narrow.
- DIP: application code depends on `IRenderTarget`, `Segment`, and abstract input contracts rather
  than backend classes.

## 6. Known Architectural Gaps

- No render HAL clipping primitive yet, so `clipToBounds` is declarative only.
- No text measurement service yet, so text box caret placement is approximate.
- No layout containers yet, so sizing and placement remain explicit at the segment level.