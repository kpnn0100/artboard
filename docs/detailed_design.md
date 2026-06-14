# Arstro Artboard — Software Detailed Design

## 1. `Segment`

`Segment` is the retained UI base class.

### Responsibilities

- Store animated `x`, `y`, `width`, and `height` properties.
- Render itself and then render children recursively.
- Convert world-space pointer coordinates to local coordinates using inverse transforms.
- Participate in a shared focus group.
- Capture child interaction during a press-drag-drop sequence.

### Important fields

- `Property x, y, width, height`
- `bool enabled, visible, focusable, clipToBounds`
- `int focusIndex`
- `std::vector<std::shared_ptr<Segment>> mChildren`
- `std::shared_ptr<InputController> mInputController`

### Important operations

- `render()` composes parent and local transforms, paints self, then paints children.
- `hitTest()` checks children from topmost to backmost, then checks the local bounds.
- `onGesture()` delegates to `dispatchGesture()`.
- `dispatchKey()` routes keyboard events to the focused segment.
- `requestFocus()` updates the shared focus registry.

## 2. `Property`

`Property` is a thin UI-facing wrapper around `AnimatedProperty`.

### Responsibilities

- Store the current scalar value.
- Start an animation toward a target value.
- Advance the value at a supplied time.

## 3. `InputController`

`InputController` is an abstract behavior strategy.

### Responsibilities

- Consume gestures for a segment.
- Consume keyboard events for a segment.

### Intent

This supports reusable behavior injection when behavior should be shared across multiple segment
types without creating a deep inheritance chain.

## 4. Visual Segment Primitives

### 4.1 `RectangleSegment`

- Draws a rectangle using a `BoxStyle`.
- Uses the segment bounds as its local geometry.

### 4.2 `CircleSegment`

- Draws a circle or ellipse from the segment bounds.
- Uses Bezier approximation so it stays backend-neutral.

### 4.3 `LabelSegment`

- Draws a text string with a `TextStyle`.
- Does not participate in hit-testing by default.

## 5. `Theme`

`Theme` is a concrete style bundle.

### Responsibilities

- Provide baseline styles for the standard controls.
- Keep appearance data separate from control behavior.

### Current baseline theme

- Dark neutral surfaces.
- Warm accent color for active state and caret.
- Shared outline color for control framing.

## 6. `AbstractSlider`

`AbstractSlider` owns slider semantics only.

### State

- `value`
- `minimum`
- `maximum`
- `isAnalog`
- `dragType`

### Rules

- Value is always clamped to the current range.
- Normalized value is derived from current value and range.

## 7. `Slider`

`Slider` is a concrete `Segment` plus `AbstractSlider`.

### Internal composition

- `RectangleSegment mTrack`
- `RectangleSegment mRangeFill`
- `CircleSegment mThumb`

### Behavior

- Pointer down, drag, and click convert local x-position into a slider value.
- Left and right arrow keys decrement or increment the value.

### Rendering sequence

1. Ensure the internal child tree exists.
2. Synchronize child geometry from current segment bounds and slider value.
3. Render the segment tree.

## 8. `Button`

### Internal composition

- `RectangleSegment mBody`
- `LabelSegment mLabel`

### Behavior

- `Down` sets the pressed state.
- `Click` invokes `onClick`.
- `Enter` or `Space` invokes `onClick`.

## 9. `Checkbox`

### Internal composition

- `RectangleSegment mBox`
- `RectangleSegment mIndicator`
- `LabelSegment mLabel`

### Behavior

- `Click`, `Enter`, or `Space` toggles the boolean state.
- The indicator visibility mirrors the checked state.

## 10. `TextBox`

### Internal composition

- `RectangleSegment mBox`
- `LabelSegment mLabel`
- `RectangleSegment mCaret`

### Behavior

- `Down` requests focus.
- `KeyEvent::Text` appends entered text.
- `Backspace` removes one character.
- Read-only mode disables editing.

### Current simplifications

- Caret x-position uses estimated text width.
- Selection, cursor movement, and clipboard behavior are not implemented yet.

## 11. Traceability to Requirements

- FR-3 and FR-10 map to `Segment`.
- FR-4 maps to `Property` and `AnimatedProperty`.
- FR-5 maps to the segment focus registry.
- FR-7 maps to `Theme` and the style structs.
- FR-8 maps to `Button`, `Slider`, `Checkbox`, and `TextBox`.
- FR-9 maps to the split between `AbstractSlider` and `Slider`.