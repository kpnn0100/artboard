# Arstro Artboard — Design Index

This document is now the entry point to the V-model documentation set for Artboard.

## V-model documents

- [requirements.md](requirements.md) defines the software requirements for the graphics and UI core.
- [architecture.md](architecture.md) defines the software architecture, package boundaries, and
  SOLID-oriented responsibilities.
- [detailed_design.md](detailed_design.md) defines class-level design for `Segment`, theme support,
  and the baseline controls (`Button`, `Slider`, `Checkbox`, `TextBox`).
- [architecture.puml](architecture.puml) provides a renderable PlantUML package and class view.

## Scope

Artboard now has two complementary authoring models:

- A `Drawable` scene for freeform graphics, animation, and adapter-neutral rendering.
- A `Segment` tree for interactive UI objects with hierarchy, focus, animated layout properties,
  concrete visual styles, and keyboard/pointer behavior.

The two models share the same geometry, animation, render HAL, and input HAL.
