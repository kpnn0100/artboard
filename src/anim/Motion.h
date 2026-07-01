/*
 *  Arstro Artboard — reduced-motion accessibility switch (FR-4e).
 *
 *  A single global flag consulted by every motion primitive (`Spring`,
 *  `AnimatedProperty`). When set, all framework motion collapses to instant so a
 *  host that detects a user "reduce motion" preference presents final states with
 *  no animation. Kept as free functions (not a class) because it is one process-
 *  wide setting with no per-instance state — the reduce-motion decision lives in
 *  exactly one place instead of being re-checked in every control.
 */
#pragma once

namespace artboard
{
    /** Enable/disable global reduced motion (default: disabled). */
    void setReducedMotion(bool on);
    /** True when motion should collapse to instant. */
    bool reducedMotion();
}
