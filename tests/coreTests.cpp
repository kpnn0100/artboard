#include "MiniTest.h"
#include "../include/artboard/artboard.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

using namespace artboard;

// ───────────────────────── core/Geometry ─────────────────────────
TEST(Transform_translate_scale_apply)
{
    Transform t = Transform::identity();
    t.translate(10, 20).scale(2, 3);
    Point p = t.apply(Point{1, 1});
    CHECK_NEAR(p.x, 10 + 2, 1e-9);
    CHECK_NEAR(p.y, 20 + 3, 1e-9);
}
TEST(Transform_rotation_and_mul)
{
    Transform r = Transform::rotation(M_PI / 2); // 90°
    Point p = r.apply(Point{1, 0});
    CHECK_NEAR(p.x, 0.0, 1e-9);
    CHECK_NEAR(p.y, 1.0, 1e-9);
    Transform m = Transform::translation(5, 0).mul(Transform::translation(0, 7));
    Point q = m.apply(Point{0, 0});
    CHECK_NEAR(q.x, 5.0, 1e-9);
    CHECK_NEAR(q.y, 7.0, 1e-9);
}
TEST(Transform_inverse)
{
    Transform t = Transform::translation(10, 20).mul(Transform::scaling(2, 4));
    Point p = t.apply(Point{3, 5});
    Point original = t.inverse().apply(p);
    CHECK_NEAR(original.x, 3.0, 1e-9);
    CHECK_NEAR(original.y, 5.0, 1e-9);
}
TEST(Rect_helpers)
{
    Rect r(1, 2, 10, 20);
    CHECK_NEAR(r.right(), 11, 1e-9);
    CHECK_NEAR(r.bottom(), 22, 1e-9);
    CHECK(r.contains(Point{5, 5}));
    CHECK(!r.contains(Point{100, 5}));
    CHECK((Point{1, 1} == Point{1, 1}));
    Size s(3, 4); CHECK_NEAR(s.w, 3, 1e-9);
}

// ───────────────────────── core/Color ─────────────────────────
TEST(Color_rgba_and_hex)
{
    Color c = Color::rgba(255, 0, 0);
    CHECK_NEAR(c.r, 1.0, 1e-9); CHECK_NEAR(c.a, 1.0, 1e-9);
    Color h = Color::hex(0x00FF00);
    CHECK_NEAR(h.g, 1.0, 1e-9);
    Color ha = Color::hex(0x80FF0000u, true); // a=0x80, r=0xFF
    CHECK_NEAR(ha.r, 1.0, 1e-9);
    CHECK(ha.a > 0.49 && ha.a < 0.51);
    CHECK(Color::rgba(1, 2, 3) == Color::rgba(1, 2, 3));
}

// ───────────────────────── anim/Easing ─────────────────────────
TEST(Easing_endpoints_pinned_for_all_curves)
{
    // Every curve is 0 at t=0 and 1 at t=1. Iterating the endpoints also exercises
    // the t<0.5 / t>=0.5 branch of every in-out curve and the t==0 / t==1 guards of
    // the expo/elastic curves.
    const Easing all[] = {
        Easing::Linear,
        Easing::EaseInQuad, Easing::EaseOutQuad, Easing::EaseInOutQuad,
        Easing::EaseInCubic, Easing::EaseOutCubic, Easing::EaseInOutCubic,
        Easing::EaseInQuart, Easing::EaseOutQuart, Easing::EaseInOutQuart,
        Easing::EaseInSine, Easing::EaseOutSine, Easing::EaseInOutSine,
        Easing::EaseInExpo, Easing::EaseOutExpo, Easing::EaseInOutExpo,
        Easing::EaseInBack, Easing::EaseOutBack, Easing::EaseInOutBack,
        Easing::EaseInElastic, Easing::EaseOutElastic, Easing::EaseInOutElastic,
        Easing::EaseInBounce, Easing::EaseOutBounce, Easing::EaseInOutBounce,
        Easing::Standard, Easing::StandardDecel, Easing::StandardAccel,
        Easing::EmphasizedDecel, Easing::EmphasizedAccel};
    for (Easing e : all)
    {
        CHECK_NEAR(applyEasing(e, 0.0), 0.0, 1e-9);
        CHECK_NEAR(applyEasing(e, 1.0), 1.0, 1e-9);
    }
    CHECK_NEAR(applyEasing(Easing::Linear, -1.0), 0.0, 1e-9); // clamp low
    CHECK_NEAR(applyEasing(Easing::Linear, 2.0), 1.0, 1e-9);  // clamp high
    CHECK_NEAR(applyEasing((Easing)999, 0.3), 0.3, 1e-9);     // defensive default
}
TEST(Easing_midpoints_and_branches)
{
    // Known closed-form midpoints.
    CHECK_NEAR(applyEasing(Easing::EaseInQuad, 0.5), 0.25, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseOutQuad, 0.5), 0.75, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseInOutQuad, 0.25), 0.125, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseInOutQuad, 0.75), 0.875, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseInCubic, 0.5), 0.125, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseOutCubic, 0.5), 0.875, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseInOutCubic, 0.25), 0.0625, 1e-9);
    CHECK(applyEasing(Easing::EaseInOutCubic, 0.75) > 0.5);
    CHECK_NEAR(applyEasing(Easing::EaseInQuart, 0.5), 0.0625, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseOutQuart, 0.5), 0.9375, 1e-9);
    CHECK(applyEasing(Easing::EaseInOutQuart, 0.25) < 0.1);
    CHECK(applyEasing(Easing::EaseInOutQuart, 0.75) > 0.9);
    // sine
    CHECK_NEAR(applyEasing(Easing::EaseInOutSine, 0.5), 0.5, 1e-9);
    CHECK(applyEasing(Easing::EaseInSine, 0.5) < 0.5);
    CHECK(applyEasing(Easing::EaseOutSine, 0.5) > 0.5);
    // expo (else branches)
    CHECK(applyEasing(Easing::EaseInExpo, 0.5) > 0.0 && applyEasing(Easing::EaseInExpo, 0.5) < 0.1);
    CHECK(applyEasing(Easing::EaseOutExpo, 0.5) > 0.9);
    CHECK(applyEasing(Easing::EaseInOutExpo, 0.25) < 0.1); // t<0.5 branch
    CHECK(applyEasing(Easing::EaseInOutExpo, 0.75) > 0.9); // else branch
    // back overshoot: EaseOutBack rises above 1 before settling
    CHECK(applyEasing(Easing::EaseOutBack, 0.6) > 1.0);
    CHECK(applyEasing(Easing::EaseInBack, 0.4) < 0.0);
    CHECK(applyEasing(Easing::EaseInOutBack, 0.25) < 0.0); // t<0.5 branch
    CHECK(applyEasing(Easing::EaseInOutBack, 0.75) > 1.0); // else branch
    // elastic (else branches)
    CHECK(applyEasing(Easing::EaseInElastic, 0.5) < 0.1);
    CHECK(applyEasing(Easing::EaseOutElastic, 0.5) > 0.9);
    CHECK(applyEasing(Easing::EaseInOutElastic, 0.25) != 0.0); // t<0.5 branch
    CHECK(applyEasing(Easing::EaseInOutElastic, 0.75) != 1.0); // else branch
    // bounce: hit all four segments of the helper via EaseOutBounce
    CHECK(applyEasing(Easing::EaseOutBounce, 0.2) > 0.0);  // seg 1
    CHECK(applyEasing(Easing::EaseOutBounce, 0.5) > 0.0);  // seg 2
    CHECK(applyEasing(Easing::EaseOutBounce, 0.8) > 0.0);  // seg 3
    CHECK(applyEasing(Easing::EaseOutBounce, 0.95) > 0.0); // seg 4
    CHECK(applyEasing(Easing::EaseInBounce, 0.3) >= 0.0);
    CHECK(applyEasing(Easing::EaseInOutBounce, 0.25) >= 0.0); // t<0.5 branch
    CHECK(applyEasing(Easing::EaseInOutBounce, 0.75) <= 1.0); // else branch
}
TEST(Easing_cubicBezier_named_curves_match_hand_computed_points)
{
    // Independent check of the Newton-Raphson/bisection bezier solver: for a cubic bezier
    // P0=(0,0), P1=(x1,y1), P2=(x2,y2), P3=(1,1), the point at parametric u=0.5 is
    //   x(0.5) = 0.375*x1 + 0.375*x2 + 0.125,  y(0.5) = 0.375*y1 + 0.375*y2 + 0.125
    // (expanding 3*(1-u)^2*u, 3*(1-u)*u^2, u^3 at u=0.5). Computed by hand from the raw
    // bezier formula (not from the solver under test) for three curves of increasing
    // asymmetry, so this actually exercises solver correctness rather than restating it.
    // Standard(0.2,0,0,1): x(.5)=0.375*0.2+0.125=0.2, y(.5)=0.375*1+0.125=0.5
    CHECK_NEAR(applyEasing(Easing::Standard, 0.2), 0.5, 1e-4);
    // StandardDecel(0,0,0,1): x(.5)=0.125, y(.5)=0.5
    CHECK_NEAR(applyEasing(Easing::StandardDecel, 0.125), 0.5, 1e-4);
    // EmphasizedAccel(0.3,0,0.8,0.15): x(.5)=0.375*0.3+0.375*0.8+0.125=0.5375,
    // y(.5)=0.375*0+0.375*0.15+0.125=0.18125
    CHECK_NEAR(applyEasing(Easing::EmphasizedAccel, 0.5375), 0.18125, 1e-4);
    // clamp behavior matches every other curve
    CHECK_NEAR(applyEasing(Easing::Standard, -1.0), 0.0, 1e-9);
    CHECK_NEAR(applyEasing(Easing::Standard, 2.0), 1.0, 1e-9);
    // StandardAccel/EmphasizedDecel exercised for endpoint pinning above; check monotonic
    // mid-curve sanity here too (no NaN/inf, stays within a sane eased range).
    const double sa = applyEasing(Easing::StandardAccel, 0.5);
    CHECK(sa > 0.0 && sa < 1.0);
    const double ed = applyEasing(Easing::EmphasizedDecel, 0.5);
    CHECK(ed > 0.0 && ed < 1.0);
    // StandardDecel's x-control-points are both 0, so bezierX(u) = u^3: the derivative
    // 3u^2 goes near-zero close to u=0, which is exactly the regime a tiny t lands the
    // initial guess (u=t) in. t=0.0001 makes the Newton loop's very first iteration see
    // d=3*(1e-4)^2=3e-8 < 1e-6, exercising the "derivative too small" bailout and the
    // bisection fallback loop (otherwise unreached by every other sample point in this
    // file, since none of them land the solver near a flat-derivative region). Verified
    // empirically: applyEasing(StandardDecel, 0.0001) == 0.0062646752 (3*(1-u)*u^2+u^3
    // at u=cuberoot(0.0001)~=0.04642).
    const double tinyResult = applyEasing(Easing::StandardDecel, 0.0001);
    CHECK_NEAR(tinyResult, 0.0062646752, 1e-6);
}
TEST(MotionTokens_named_constants)
{
    // Regression guard: these are consumed by name across the codebase (and, going
    // forward, by shell-side theme code) -- catch an accidental value drift.
    CHECK_NEAR(motion::kDurationShort1, 50.0, 1e-9);
    CHECK_NEAR(motion::kDurationShort4, 200.0, 1e-9);
    CHECK_NEAR(motion::kDurationMedium1, 250.0, 1e-9);
    CHECK_NEAR(motion::kDurationMedium4, 400.0, 1e-9);
    CHECK_NEAR(motion::kDurationLong1, 450.0, 1e-9);
    CHECK_NEAR(motion::kDurationLong4, 600.0, 1e-9);
    CHECK(motion::kSpatialFast > motion::kSpatialDefault);
    CHECK(motion::kSpatialDefault > motion::kSpatialSlow);
    CHECK(motion::kEffectsFast > motion::kEffectsDefault);
    CHECK(motion::kEffectsDefault > motion::kEffectsSlow);
    CHECK_NEAR(motion::kSpatialDefault, 18.0, 1e-9); // matches Slider/Knob's existing omega
}

// ───────────────────────── anim/Animation ─────────────────────────
TEST(Animation_value_and_clamp)
{
    Animation a(0, 100, 1000, Easing::Linear);
    CHECK_NEAR(a.value(500), 50, 1e-9);
    CHECK_NEAR(a.value(-10), 0, 1e-9);   // clamp low
    CHECK_NEAR(a.value(99999), 100, 1e-9); // clamp high
    CHECK(a.finished(1000));
    CHECK(!a.finished(999));
    Animation z(5, 9, 0); // zero duration -> jumps to target
    CHECK_NEAR(z.value(0), 9, 1e-9);
}
TEST(AnimatedProperty_lifecycle)
{
    AnimatedProperty p(0.0);
    CHECK_NEAR(p.update(0), 0.0, 1e-9);   // not animating branch
    CHECK(!p.isAnimating());
    p.animateTo(10.0, 100.0, Easing::Linear, 1000.0);
    CHECK(p.isAnimating());
    CHECK_NEAR(p.update(1050.0), 5.0, 1e-9); // halfway
    CHECK_NEAR(p.update(2000.0), 10.0, 1e-9); // finished -> snaps to target
    CHECK(!p.isAnimating());
    p.set(3.0);
    CHECK_NEAR(p.value(), 3.0, 1e-9);
}

// ───────────────────────── anim/Tween ─────────────────────────
TEST(Tween_basic_and_finish)
{
    Tween t(0, 10, 100);
    CHECK_NEAR(t.at(-5), 0.0, 1e-9);  // before start -> from
    CHECK_NEAR(t.at(0), 0.0, 1e-9);
    CHECK_NEAR(t.at(50), 5.0, 1e-9);  // linear midpoint
    CHECK_NEAR(t.at(100), 10.0, 1e-9); // finished -> to
    CHECK_NEAR(t.at(250), 10.0, 1e-9); // past end clamps
    CHECK_NEAR(t.totalMs(), 100.0, 1e-9);
    CHECK(t.finished(100));
    CHECK(!t.finished(99));
}
TEST(Tween_delay_and_zero_duration)
{
    Tween d(0, 10, 100);
    d.delayMs = 50;
    CHECK_NEAR(d.at(25), 0.0, 1e-9);   // inside delay window
    CHECK_NEAR(d.at(100), 5.0, 1e-9);  // 50ms into the 100ms tween
    CHECK_NEAR(d.at(150), 10.0, 1e-9); // finished
    CHECK_NEAR(d.totalMs(), 150.0, 1e-9);

    Tween z(5, 9, 0); // zero duration -> snap to `to` once started
    CHECK_NEAR(z.at(0), 5.0, 1e-9);    // local<=0 -> from
    CHECK_NEAR(z.at(1), 9.0, 1e-9);    // duration<=0 -> to
    CHECK(z.finished(0));
}
TEST(Tween_repeat_yoyo_infinite)
{
    Tween r(0, 10, 100);
    r.repeat = 1; // two cycles
    CHECK_NEAR(r.at(50), 5.0, 1e-9);   // cycle 0
    CHECK_NEAR(r.at(150), 5.0, 1e-9);  // cycle 1 forward
    CHECK_NEAR(r.at(200), 10.0, 1e-9); // finished, last cycle forward -> to
    CHECK_NEAR(r.totalMs(), 200.0, 1e-9);

    Tween y(0, 10, 100);
    y.repeat = 1; y.yoyo = true;
    CHECK_NEAR(y.at(125), 7.5, 1e-9);  // cycle 1 reversed: phase .25 -> .75
    CHECK_NEAR(y.at(200), 0.0, 1e-9);  // finished, odd last cycle -> back to `from`

    Tween y2(0, 10, 100);
    y2.repeat = 2; y2.yoyo = true;     // three cycles, last one forward
    CHECK_NEAR(y2.at(400), 10.0, 1e-9);

    Tween inf(0, 10, 100);
    inf.repeat = -1;
    CHECK(!inf.finished(1e9));
    CHECK(inf.totalMs() > 1e300);      // infinity
    CHECK_NEAR(inf.at(100050), 5.0, 1e-9); // cycle 1000, phase .5
}
TEST(Tween_factory_helpers)
{
    Tween t = Tween::range(0, 1, 200).withEasing(Easing::EaseOutCubic).after(10).repeats(2).yoyoing();
    CHECK_NEAR(t.from, 0.0, 1e-9);
    CHECK_NEAR(t.to, 1.0, 1e-9);
    CHECK_NEAR(t.durationMs, 200.0, 1e-9);
    CHECK_NEAR(t.delayMs, 10.0, 1e-9);
    CHECK(t.easing == Easing::EaseOutCubic);
    CHECK(t.repeat == 2);
    CHECK(t.yoyo);
    Tween l = Tween::range(0, 1, 50).looping();
    CHECK(l.repeat == -1);
}
TEST(AnimatedProperty_tween_and_onComplete)
{
    AnimatedProperty p(0.0);
    int completed = 0;
    p.animate(Tween(0, 10, 100), 1000.0, [&] { ++completed; });
    CHECK_NEAR(p.value(), 0.0, 1e-9); // initialised to from
    CHECK(p.isAnimating());
    CHECK_NEAR(p.update(1050.0), 5.0, 1e-9);
    CHECK(completed == 0);
    CHECK_NEAR(p.update(1100.0), 10.0, 1e-9); // finished
    CHECK(completed == 1);
    CHECK(!p.isAnimating());
    p.update(1200.0); // already inactive: no double-fire
    CHECK(completed == 1);

    // onComplete is dropped by set()
    AnimatedProperty q(0.0);
    int q_done = 0;
    q.animate(Tween(0, 1, 100), 0.0, [&] { ++q_done; });
    q.set(0.5);
    q.update(1000.0);
    CHECK(q_done == 0);

    // animate without a completion callback still finishes cleanly
    AnimatedProperty r(0.0);
    r.animate(Tween(0, 1, 100), 0.0);
    CHECK_NEAR(r.update(200.0), 1.0, 1e-9);
    CHECK(!r.isAnimating());
}
TEST(Property_animate_tween_passthrough)
{
    Property p(0.0);
    p.animate(Tween(0, 8, 100), 0.0);
    CHECK(p.isAnimating());
    CHECK_NEAR(p.update(50.0), 4.0, 1e-9);
    CHECK_NEAR(p.update(100.0), 8.0, 1e-9);
}

// ───────────────────────── anim/Animator ─────────────────────────
TEST(Animator_drives_value_and_completes)
{
    Animator anim;
    double last = -1;
    int done = 0;
    anim.tween(0, 10, 100).easing(Easing::Linear)
        .onUpdate([&](double v) { last = v; })
        .onComplete([&] { ++done; });
    CHECK(anim.activeCount() == 1);
    anim.advance(0.0);   // lazy start
    CHECK_NEAR(last, 0.0, 1e-9);
    anim.advance(50.0);
    CHECK_NEAR(last, 5.0, 1e-9);
    CHECK(anim.activeCount() == 1); // not finished -> no erase
    anim.advance(100.0);
    CHECK_NEAR(last, 10.0, 1e-9);
    CHECK(done == 1);
    CHECK(anim.activeCount() == 0); // finished -> removed
}
TEST(Animator_no_callbacks_and_clear_and_loop)
{
    Animator anim;
    // No onUpdate / no onComplete, instantaneous: still ticks + auto-removes.
    anim.tween(0, 1, 0);
    anim.advance(0.0);
    CHECK(anim.activeCount() == 0);

    // Infinite track never finishes.
    anim.tween(0, 1, 100).loop();
    anim.advance(0.0);
    anim.advance(100000.0);
    CHECK(anim.activeCount() == 1);

    // clear() drops everything without firing completion.
    anim.clear();
    CHECK(anim.activeCount() == 0);
}
TEST(Animator_handle_chaining_covers_all_setters)
{
    Animator anim;
    double v = 0;
    int done = 0;
    anim.tween(0, 1, 100)
        .easing(Easing::EaseInOutSine)
        .delay(10)
        .repeat(1)
        .yoyo(true)
        .onUpdate([&](double x) { v = x; })
        .onComplete([&] { ++done; });
    anim.advance(0.0);
    CHECK_NEAR(v, 0.0, 1e-9); // inside delay
    anim.advance(10000.0);    // well past the end (delay 10 + 2*100)
    CHECK(done == 1);
    CHECK(anim.activeCount() == 0);
}

TEST(Spring_converges_no_overshoot_and_reset)
{
    Spring s(0.0);
    s.setTarget(1.0);
    CHECK_NEAR(s.value(), 0.0, 1e-12);
    CHECK(s.isMoving());
    // monotone rise to the target, never overshooting (critically damped)
    double prev = s.value(), maxV = 0.0;
    for (int i = 0; i < 300; ++i)
    {
        double v = s.advance(0.016);
        CHECK(v >= prev - 1e-9);
        prev = v;
        if (v > maxV) maxV = v;
    }
    CHECK_NEAR(s.value(), 1.0, 1e-3);   // settled
    CHECK(maxV <= 1.0 + 1e-6);          // no overshoot past the target
    CHECK(!s.isMoving());

    // dt <= 0 is a no-op
    const double held = s.value();
    CHECK_NEAR(s.advance(0.0), held, 1e-12);
    CHECK_NEAR(s.advance(-1.0), held, 1e-12);

    // a long stall is clamped to 0.05s (still well-behaved, stays <= target)
    s.reset(0.0);
    s.setTarget(1.0);
    double vClamped = s.advance(0.2);   // dt > 0.05 -> clamped
    CHECK(vClamped > 0.0 && vClamped <= 1.0 + 1e-6);

    // reset snaps value + target and clears velocity
    s.reset(0.25);
    CHECK_NEAR(s.value(), 0.25, 1e-12);
    CHECK_NEAR(s.target(), 0.25, 1e-12);
    CHECK_NEAR(s.velocity(), 0.0, 1e-12);
    CHECK(!s.isMoving());
}

TEST(Spring_framerate_independent)
{
    // Closed-form step: reaching t=0.05s in one 0.05 step equals five 0.01 steps
    // (same value AND velocity) — the smoothing does not depend on frame rate.
    Spring coarse(0.0); coarse.setTarget(1.0);
    coarse.advance(0.05);
    Spring fine(0.0); fine.setTarget(1.0);
    for (int i = 0; i < 5; ++i) fine.advance(0.01);
    CHECK(coarse.value() > 0.0 && coarse.value() < 1.0); // genuinely mid-flight
    CHECK_NEAR(coarse.value(), fine.value(), 1e-9);
    CHECK_NEAR(coarse.velocity(), fine.velocity(), 1e-9);
}

TEST(ReducedMotion_switch_snaps_spring_and_tween)
{
    setReducedMotion(true);
    CHECK(reducedMotion());

    // Spring jumps straight to the target regardless of dt
    Spring s(0.0); s.setTarget(1.0);
    CHECK_NEAR(s.advance(0.001), 1.0, 1e-12);
    CHECK(!s.isMoving());

    // AnimatedProperty::animateTo snaps to the target, inactive
    AnimatedProperty p(0.0);
    p.animateTo(2.0, 300.0, Easing::EaseOutCubic, 0.0);
    CHECK_NEAR(p.value(), 2.0, 1e-12);
    CHECK(!p.isAnimating());

    // AnimatedProperty::animate snaps to the tween's resting value + fires onComplete once
    int done = 0;
    AnimatedProperty q(0.0);
    q.animate(Tween::range(0.0, 5.0, 400.0), 0.0, [&] { ++done; });
    CHECK_NEAR(q.value(), 5.0, 1e-12);
    CHECK(!q.isAnimating());
    CHECK(done == 1);

    setReducedMotion(false); // restore the global for the rest of the suite
    CHECK(!reducedMotion());
}

// ───────────────────────── render + scene ─────────────────────────
using K = DrawOp::Kind;

namespace
{
    class BoxSegment : public Segment
    {
    public:
        explicit BoxSegment(const Color &fillColor) : mFill(fillColor) {}

    protected:
        void onPaint(IRenderTarget &t) const override
        {
            t.beginPath();
            t.moveTo(0, 0);
            t.lineTo(width.value(), 0);
            t.lineTo(width.value(), height.value());
            t.lineTo(0, height.value());
            t.closePath();
            t.setFill(mFill);
            t.fillPath();
        }

    private:
        Color mFill;
    };

    class LoggingController : public InputController
    {
    public:
        std::vector<Gesture::Type> gestures;
        std::vector<KeyEvent::Type> keys;

        bool onGesture(Segment &, const Gesture &gesture, const Point &) override
        {
            gestures.push_back(gesture.type);
            return true;
        }

        bool onKey(Segment &, const KeyEvent &event) override
        {
            keys.push_back(event.type);
            return true;
        }
    };
}

TEST(Rectangle_sharp_and_rounded)
{
    RecordingTarget t;
    Rectangle sharp(Rect{0, 0, 10, 10}, Paint::filled(Color::rgba(255, 0, 0)));
    sharp.render(t);
    CHECK(t.count(K::FillPath) == 1);
    CHECK(t.count(K::QuadTo) == 0);     // no corners
    CHECK(t.count(K::SetTransform) == 1);

    t.clear();
    // radius larger than half -> clamp branch; stroked border too.
    Rectangle round(Rect{0, 0, 10, 20}, Paint::filledStroked(Color::rgba(0, 0, 0), Color::rgba(255, 255, 255), 2), 999);
    round.render(t);
    CHECK(t.count(K::QuadTo) == 4);     // four rounded corners
    CHECK(t.count(K::FillPath) == 1 && t.count(K::StrokePath) == 1);
}

TEST(FilledStroked_shares_one_path_FR16)
{
    // FR-16: fill then stroke paint the SAME path (filled body + border). The op stream must
    // build the path once, then FillPath, then StrokePath — with NO BeginPath in between.
    // Adapters preserve the path across fill/stroke and clear it only on the next beginPath;
    // an adapter that cleared it between would silently drop the border (the native bug fixed
    // by removing cairo_new_path from CairoTarget::fillPath/strokePath).
    RecordingTarget t;
    Rectangle r(Rect{0, 0, 10, 10}, Paint::filledStroked(Color::rgba(0, 0, 0), Color::rgba(255, 255, 255), 2));
    r.render(t);

    const auto &ops = t.ops();
    int begin = -1, fill = -1, stroke = -1;
    for (size_t i = 0; i < ops.size(); ++i)
    {
        if (ops[i].kind == K::BeginPath) begin = (int)i;
        if (ops[i].kind == K::FillPath) fill = (int)i;
        if (ops[i].kind == K::StrokePath) stroke = (int)i;
    }
    CHECK(begin >= 0 && fill > begin && stroke > fill); // begin -> ... -> fill -> stroke
    // exactly one of each, and no second BeginPath separating fill from stroke
    CHECK(t.count(K::BeginPath) == 1 && t.count(K::FillPath) == 1 && t.count(K::StrokePath) == 1);
}

TEST(Line_and_Polyline)
{
    RecordingTarget t;
    Line ln(Point{0, 0}, Point{5, 5}, Color::rgba(0, 0, 0), 1.0);
    ln.render(t);
    CHECK(t.count(K::StrokePath) == 1 && t.count(K::LineTo) == 1);

    t.clear();
    Polyline empty; empty.render(t);          // empty -> early return
    CHECK(t.count(K::BeginPath) == 0);

    t.clear();
    Polyline poly;
    poly.points = {{0, 0}, {1, 1}, {2, 0}};
    poly.closed = true;
    poly.paint = Paint::stroked(Color::rgba(0, 0, 0), 1);
    poly.render(t);
    CHECK(t.count(K::LineTo) == 2 && t.count(K::ClosePath) == 1 && t.count(K::StrokePath) == 1);
}

TEST(Ellipse_four_cubics)
{
    RecordingTarget t;
    Ellipse e(Point{50, 50}, 30, 20, Paint::filled(Color::rgba(10, 20, 30)));
    e.render(t);
    CHECK(t.count(K::CubicTo) == 4);
    CHECK(t.count(K::FillPath) == 1);
}

TEST(Path_segments_spline_and_no_paint)
{
    RecordingTarget t;
    Path p;
    p.moveTo(0, 0).lineTo(1, 1).quadTo(2, 2, 3, 3).cubicTo(4, 4, 5, 5, 6, 6).close();
    // default Paint: neither fill nor stroke -> applyPaint does nothing.
    p.render(t);
    CHECK(t.count(K::MoveTo) == 1 && t.count(K::LineTo) == 1);
    CHECK(t.count(K::QuadTo) == 1 && t.count(K::CubicTo) == 1 && t.count(K::ClosePath) == 1);
    CHECK(t.count(K::FillPath) == 0 && t.count(K::StrokePath) == 0);

    t.clear();
    Path sp;
    sp.spline({}); // <2 points -> early return, no ops
    sp.render(t);
    CHECK(t.count(K::MoveTo) == 0);

    t.clear();
    Path sp2;
    sp2.paint = Paint::stroked(Color::rgba(0, 0, 0), 1);
    sp2.spline({{0, 0}, {10, 10}, {20, 0}, {30, 10}}); // exercises boundary clamps
    sp2.render(t);
    CHECK(t.count(K::MoveTo) == 1 && t.count(K::CubicTo) == 3);
}

TEST(Text_uses_fill_and_drawText)
{
    RecordingTarget t;
    Text txt("hi", Point{4, 8}, 16, Color::rgba(255, 255, 255));
    txt.render(t);
    CHECK(t.count(K::DrawText) == 1 && t.count(K::SetFill) == 1);
    std::string drawn;
    for (const auto &op : t.ops())
        if (op.kind == K::DrawText) drawn = op.text;
    CHECK(drawn == "hi");
}

TEST(DrawText_font_family_and_letter_spacing_default_to_prior_behavior)
{
    RecordingTarget t;
    t.drawText("plain", 1, 2, 10); // old 4-arg call site: must still compile and record defaults
    CHECK(t.ops().size() == 1);
    CHECK(t.ops()[0].fontFamily.empty());
    CHECK_NEAR(t.ops()[0].letterSpacingPx, 0.0, 1e-9);
}

TEST(DrawText_font_family_and_letter_spacing_are_recorded)
{
    RecordingTarget t;
    t.drawText("tracked", 3, 4, 12, "DM Sans Medium", 1.5);
    CHECK(t.ops().size() == 1);
    const auto &op = t.ops()[0];
    CHECK(op.kind == K::DrawText);
    CHECK(op.text == "tracked");
    CHECK(op.fontFamily == "DM Sans Medium");
    CHECK_NEAR(op.letterSpacingPx, 1.5, 1e-9);
}

TEST(Text_drawable_forwards_font_family_and_letter_spacing)
{
    // Drawable::render() appends Save/SetTransform/.../Restore around onDraw(), so
    // the DrawText op is not necessarily last -- find it by kind (as
    // Text_uses_fill_and_drawText above does), don't assume ops().back().
    RecordingTarget t;
    Text txt("hi", Point{4, 8}, 16, Color::rgba(255, 255, 255));
    txt.fontFamily = "JetBrains Mono";
    txt.letterSpacingPx = 0.5;
    txt.render(t);
    CHECK(t.count(K::DrawText) == 1);
    const DrawOp *drawTextOp = nullptr;
    for (const auto &op : t.ops())
        if (op.kind == K::DrawText) drawTextOp = &op;
    CHECK(drawTextOp != nullptr);
    if (drawTextOp)
    {
        CHECK(drawTextOp->fontFamily == "JetBrains Mono");
        CHECK_NEAR(drawTextOp->letterSpacingPx, 0.5, 1e-9);
    }
}

TEST(LabelSegment_forwards_text_style_font_family_and_letter_spacing)
{
    RecordingTarget t;
    LabelSegment label;
    label.text = "SECTION";
    label.style.fontFamily = "DM Sans SemiBold";
    label.style.letterSpacingPx = 1.2;
    label.render(t);
    CHECK(t.count(K::DrawText) == 1);
    const DrawOp *drawTextOp = nullptr;
    for (const auto &op : t.ops())
        if (op.kind == K::DrawText) drawTextOp = &op;
    CHECK(drawTextOp != nullptr);
    if (drawTextOp)
    {
        CHECK(drawTextOp->fontFamily == "DM Sans SemiBold");
        CHECK_NEAR(drawTextOp->letterSpacingPx, 1.2, 1e-9);
    }
}

TEST(Drawable_visibility)
{
    RecordingTarget t;
    Rectangle r(Rect{0, 0, 1, 1}, Paint::filled(Color::rgba(1, 1, 1)));
    r.visible = false;
    r.render(t);
    CHECK(t.ops().empty());  // hidden -> nothing recorded
}

TEST(Segment_recursive_render_and_animation)
{
    RecordingTarget t;

    auto parent = std::make_shared<BoxSegment>(Color::rgba(255, 0, 0));
    parent->x.set(10);
    parent->y.set(20);
    parent->width.set(100);
    parent->height.set(50);

    auto child = std::make_shared<BoxSegment>(Color::rgba(0, 255, 0));
    child->x.set(3);
    child->y.set(4);
    child->width.set(8);
    child->height.set(9);
    child->x.animateTo(13, 100.0, Easing::Linear, 0.0);
    child->advance(50.0);

    parent->addChild(child);
    parent->render(t);

    std::vector<Transform> transforms;
    for (const auto &op : t.ops())
        if (op.kind == K::SetTransform)
            transforms.push_back(op.transform);

    CHECK(transforms.size() == 2);
    CHECK_NEAR(transforms[0].e, 10.0, 1e-9);
    CHECK_NEAR(transforms[0].f, 20.0, 1e-9);
    CHECK_NEAR(transforms[1].e, 18.0, 1e-9);
    CHECK_NEAR(transforms[1].f, 24.0, 1e-9);
}

TEST(Segment_focus_capture_and_key_dispatch)
{
    auto root = std::make_shared<BoxSegment>(Color::rgba(0, 0, 0));
    root->width.set(200);
    root->height.set(200);

    auto left = std::make_shared<BoxSegment>(Color::rgba(255, 0, 0));
    left->width.set(20);
    left->height.set(20);
    left->focusable = true;
    left->focusIndex = 1;
    auto leftController = std::make_shared<LoggingController>();
    left->setInputController(leftController);

    auto right = std::make_shared<BoxSegment>(Color::rgba(0, 255, 0));
    right->x.set(40);
    right->width.set(20);
    right->height.set(20);
    right->focusable = true;
    right->focusIndex = 1;
    auto rightController = std::make_shared<LoggingController>();
    right->setInputController(rightController);

    root->addChild(left);
    root->addChild(right);

    root->onGesture({Gesture::Type::Down, {5, 5}, {5, 5}, PointerButton::Left});
    root->onGesture({Gesture::Type::Drag, {100, 100}, {5, 5}, PointerButton::Left});
    root->onGesture({Gesture::Type::Drop, {100, 100}, {5, 5}, PointerButton::Left});
    CHECK(left->hasFocus());
    CHECK(!right->hasFocus());
    CHECK(leftController->gestures.size() == 3);
    CHECK(leftController->gestures[0] == Gesture::Type::Down);
    CHECK(leftController->gestures[1] == Gesture::Type::Drag);
    CHECK(leftController->gestures[2] == Gesture::Type::Drop);

    root->onGesture({Gesture::Type::Down, {45, 5}, {45, 5}, PointerButton::Left});
    CHECK(!left->hasFocus());
    CHECK(right->hasFocus());
    CHECK(Segment::focusedInGroup(1) == right.get());

    root->dispatchKey({KeyEvent::Type::Down, 65, "a"});
    CHECK(rightController->keys.size() == 1);
    CHECK(rightController->keys[0] == KeyEvent::Type::Down);
}

TEST(Basic_controls_composition_and_interaction)
{
    Theme theme = Theme::basicTheme();

    auto root = std::make_shared<BoxSegment>(Color::rgba(8, 8, 8));
    root->width.set(400.0);
    root->height.set(300.0);

    auto slider = std::make_shared<Slider>(theme.slider);
    slider->setRange(0.0, 100.0);
    slider->x.set(10.0);
    slider->y.set(10.0);
    slider->width.set(200.0);
    root->addChild(slider);
    root->onGesture({Gesture::Type::Down, {60, 20}, {60, 20}, PointerButton::Left});       // capture the slider
    root->onGesture({Gesture::Type::DragStart, {60, 20}, {60, 20}, PointerButton::Left});  // drag sets value from x
    CHECK(slider->value() > 24.0 && slider->value() < 26.0);
    slider->requestFocus();
    root->dispatchKey({KeyEvent::Type::Down, 39, ""});
    CHECK(slider->value() > 29.0 && slider->value() < 31.0);

    auto checkbox = std::make_shared<Checkbox>("Bypass", theme.checkbox);
    checkbox->x.set(10.0);
    checkbox->y.set(50.0);
    root->addChild(checkbox);
    root->onGesture({Gesture::Type::Click, {15, 55}, {15, 55}, PointerButton::Left});
    CHECK(checkbox->checked());

    auto textBox = std::make_shared<TextBox>(theme.textBox);
    textBox->x.set(10.0);
    textBox->y.set(90.0);
    textBox->placeholder = "Name";
    root->addChild(textBox);
    root->onGesture({Gesture::Type::Down, {15, 100}, {15, 100}, PointerButton::Left});
    root->dispatchKey({KeyEvent::Type::Text, 0, "A"});
    root->dispatchKey({KeyEvent::Type::Text, 0, "B"});
    root->dispatchKey({KeyEvent::Type::Down, 8, ""});
    CHECK(textBox->text == "A");

    auto button = std::make_shared<Button>("Apply", theme.button);
    button->x.set(10.0);
    button->y.set(140.0);
    int clicks = 0;
    button->onClick = [&]() { ++clicks; };
    root->addChild(button);
    root->onGesture({Gesture::Type::Down, {15, 145}, {15, 145}, PointerButton::Left});
    root->onGesture({Gesture::Type::Click, {15, 145}, {15, 145}, PointerButton::Left});
    button->requestFocus();
    root->dispatchKey({KeyEvent::Type::Down, 13, ""});
    CHECK(clicks == 2);

    RecordingTarget t;
    slider->render(t);
    CHECK(slider->childCount() == 5);  // track, rangeFill, subFill, subTick, thumb
    CHECK(t.count(K::SetTransform) >= 3);
}

TEST(Artboard_background_children_and_onframe)
{
    RecordingTarget t;
    Artboard ab(Size{100, 80});
    // no background, no onFrame
    ab.render(t, 0);
    CHECK(t.ops().empty());

    ab.setBackground(Color::rgba(0, 0, 0));
    int frames = 0;
    ab.setOnFrame([&](double) { ++frames; });
    ab.add(std::make_shared<Rectangle>(Rect{0, 0, 10, 10}, Paint::filled(Color::rgba(255, 0, 0))));
    CHECK(ab.childCount() == 1);
    ab.add(std::make_shared<Rectangle>(Rect{0, 0, 1, 1}, Paint::filled(Color::rgba(0, 255, 0))));
    CHECK(ab.childCount() == 2);
    ab.clear();
    CHECK(ab.childCount() == 0);
    ab.add(std::make_shared<Rectangle>(Rect{0, 0, 10, 10}, Paint::filled(Color::rgba(255, 0, 0))));
    ab.render(t, 16.0);
    CHECK(frames == 1);
    CHECK(t.count(K::FillPath) == 2); // background + child
    CHECK(t.count(K::SetTransform) >= 2);

    // explicit save/restore/setStroke coverage via direct calls
    RecordingTarget t2;
    t2.save(); t2.restore();
    t2.setStroke(Color::rgba(1, 1, 1), 3);
    CHECK(t2.count(K::Save) == 1 && t2.count(K::Restore) == 1 && t2.count(K::SetStroke) == 1);
}

// ───────────────────────── input: GestureRecognizer ─────────────────────────
using GT = Gesture::Type;
using PB = PointerButton;

static std::vector<Gesture> recordGestures(std::function<void(GestureRecognizer &)> drive)
{
    GestureRecognizer r;
    std::vector<Gesture> out;
    r.setSink([&](const Gesture &g) { out.push_back(g); });
    drive(r);
    return out;
}
static int countG(const std::vector<Gesture> &v, GT t)
{
    int n = 0; for (auto &g : v) if (g.type == t) ++n; return n;
}
static int countOf(const std::vector<GT> &v, GT t)
{
    int n = 0; for (auto x : v) if (x == t) ++n; return n;
}

TEST(Gesture_click)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 120});
    });
    CHECK(countG(g, GT::Down) == 1 && countG(g, GT::Up) == 1 && countG(g, GT::Click) == 1);
    CHECK(countG(g, GT::DoubleClick) == 0);
}

TEST(Gesture_double_click_then_reset)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.setDoubleClickMs(300);
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 120});   // Click
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 250});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 280});   // DoubleClick (within window)
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 320});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 340});   // Click again (reset)
    });
    CHECK(countG(g, GT::DoubleClick) == 1);
    CHECK(countG(g, GT::Click) == 2);
}

TEST(Gesture_alt_modifier_passthrough)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100, true});   // Alt held
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 120, false});  // Alt released
    });
    bool downAlt = false, clickAlt = true;
    for (const auto &e : g)
    {
        if (e.type == GT::Down) downAlt = e.alt;
        if (e.type == GT::Click) clickAlt = e.alt;
    }
    CHECK(downAlt == true);    // modifier carried onto the Down gesture
    CHECK(clickAlt == false);  // and reflects the releasing event's state
}

TEST(Gesture_shift_ctrl_modifier_passthrough)
{
    RawPointer down{RawPointer::Kind::Down, {5, 5}, PB::Left, 100};
    down.shift = true; down.ctrl = true;
    RawPointer up{RawPointer::Kind::Up, {5, 5}, PB::Left, 120};
    up.shift = false; up.ctrl = true;
    auto g = recordGestures([&](GestureRecognizer &r) { r.feed(down); r.feed(up); });
    bool downShift = false, clickShift = true, clickCtrl = false;
    for (const auto &e : g)
    {
        if (e.type == GT::Down) downShift = e.shift;
        if (e.type == GT::Click) { clickShift = e.shift; clickCtrl = e.ctrl; }
    }
    CHECK(downShift == true);
    CHECK(clickShift == false);  // reflects the releasing event
    CHECK(clickCtrl == true);
}

TEST(Gesture_not_double_when_far_or_late)
{
    auto far = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 110});
        r.feed({RawPointer::Kind::Down, {99, 99}, PB::Left, 120});
        r.feed({RawPointer::Kind::Up,   {99, 99}, PB::Left, 130}); // far -> not double
    });
    CHECK(countG(far, GT::DoubleClick) == 0 && countG(far, GT::Click) == 2);

    auto late = recordGestures([](GestureRecognizer &r) {
        r.setDoubleClickMs(100);
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 110});
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 900});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 910});   // late -> not double
    });
    CHECK(countG(late, GT::DoubleClick) == 0 && countG(late, GT::Click) == 2);
}

TEST(Gesture_right_click)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Right, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Right, 110});
    });
    CHECK(countG(g, GT::RightClick) == 1 && countG(g, GT::Click) == 0);
}

TEST(Gesture_drag_and_drop)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.setDragThreshold(5);
        r.feed({RawPointer::Kind::Down, {0, 0}, PB::Left, 0});
        r.feed({RawPointer::Kind::Move, {20, 0}, PB::Left, 10});  // crosses threshold: DragStart + Drag
        r.feed({RawPointer::Kind::Move, {40, 0}, PB::Left, 20});  // Drag
        r.feed({RawPointer::Kind::Up,   {40, 0}, PB::Left, 30});  // Up + Drop (no Click)
    });
    CHECK(countG(g, GT::DragStart) == 1);
    CHECK(countG(g, GT::Drag) == 2);
    CHECK(countG(g, GT::Drop) == 1);
    CHECK(countG(g, GT::Click) == 0);
}

TEST(Gesture_hover_move)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Move, {3, 3}, PB::Left, 5}); // no press -> hover Move
    });
    CHECK(countG(g, GT::Move) == 1);
    // a recognizer with no sink must not crash:
    GestureRecognizer silent;
    silent.feed({RawPointer::Kind::Down, {0, 0}, PB::Left, 0});
}

// ───────────────────────── input: InputRouter ─────────────────────────
TEST(InputRouter_topmost_capture_and_miss)
{
    int aHits = 0;
    std::vector<GT> bSeq;
    RectTarget a(Rect{0, 0, 10, 10}, [&](const Gesture &) { ++aHits; });
    RectTarget b(Rect{0, 0, 10, 10}, [&](const Gesture &g) { bSeq.push_back(g.type); });
    InputRouter router;
    router.add(&a);
    router.add(&b); // b added last -> on top

    // Press sequence captures the topmost (b); drag leaves bounds but still routes to b.
    router.route({GT::Down, {5, 5}, {5, 5}, PB::Left});
    CHECK(router.captured() == &b);
    router.route({GT::DragStart, {50, 50}, {5, 5}, PB::Left});
    router.route({GT::Drag, {80, 80}, {5, 5}, PB::Left});
    router.route({GT::Drop, {80, 80}, {5, 5}, PB::Left});
    CHECK(router.captured() == nullptr);
    CHECK(bSeq.size() == 4 && aHits == 0); // Down,DragStart,Drag,Drop all to b; a never hit

    // Hover (no capture) routes to topmost; a miss routes to nobody.
    bSeq.clear();
    router.route({GT::Move, {5, 5}, {5, 5}, PB::Left});
    CHECK(bSeq.size() == 1);
    router.route({GT::Move, {500, 500}, {500, 500}, PB::Left}); // miss -> nobody
    router.route({GT::Click, {5, 5}, {5, 5}, PB::Left});        // fresh hit-test -> b
    router.route({GT::Click, {500, 500}, {500, 500}, PB::Left}); // miss -> nobody (topAt nullptr)
    CHECK(countOf(bSeq, GT::Move) == 1 && countOf(bSeq, GT::Click) == 1);

    // Down on empty space: no capture, no crash.
    router.route({GT::Down, {999, 999}, {999, 999}, PB::Left});
    CHECK(router.captured() == nullptr);
    router.route({GT::Up, {999, 999}, {999, 999}, PB::Left}); // up with no capture
    router.clear();
}

// ───────────────────────── clip / clipToBounds ─────────────────────────
TEST(RecordingTarget_clipRect_records)
{
    RecordingTarget rec;
    rec.clipRect(1, 2, 30, 40);
    CHECK(rec.count(K::ClipRect) == 1);
    CHECK_NEAR(rec.ops()[0].args[0], 1.0, 1e-9);
    CHECK_NEAR(rec.ops()[0].args[3], 40.0, 1e-9);
}
TEST(RecordingTarget_setRadialFill_records)
{
    RecordingTarget rec;
    Color inner = Color::rgba(255, 0, 0, 255);
    Color outer = Color::rgba(255, 0, 0, 0); // fade to zero opacity
    rec.setRadialFill(10, 20, 8, inner, outer);
    CHECK(rec.count(K::SetRadialFill) == 1);
    const auto &op = rec.ops()[0];
    CHECK_NEAR(op.args[0], 10.0, 1e-9);
    CHECK_NEAR(op.args[1], 20.0, 1e-9);
    CHECK_NEAR(op.args[2], 8.0, 1e-9);
    CHECK(op.color == inner);
    CHECK(op.color2 == outer);
}
TEST(RecordingTarget_setLinearFill_records)
{
    RecordingTarget rec;
    Color start = Color::rgba(10, 20, 30, 255);
    Color end = Color::rgba(40, 50, 60, 0); // fade to zero opacity along the axis
    rec.setLinearFill(3, 4, 5, 6, start, end);
    CHECK(rec.count(K::SetLinearFill) == 1);
    const auto &op = rec.ops()[0];
    CHECK_NEAR(op.args[0], 3.0, 1e-9);
    CHECK_NEAR(op.args[1], 4.0, 1e-9);
    CHECK_NEAR(op.args[2], 5.0, 1e-9);
    CHECK_NEAR(op.args[3], 6.0, 1e-9);
    CHECK(op.color == start);
    CHECK(op.color2 == end);
}
TEST(RecordingTarget_clipPath_records)
{
    RecordingTarget rec;
    rec.clipPath();
    CHECK(rec.count(K::ClipPath) == 1);
    CHECK(rec.ops()[0].kind == K::ClipPath);
}
TEST(RecordingTarget_clipPath_after_path_build_full_sequence)
{
    // A realistic call: build a path, then clip to it instead of filling/stroking -- assert
    // the whole op sequence (order + args) so the primitive is proven to compose correctly
    // with the existing path-building ops (FR-16) rather than just recording in isolation.
    RecordingTarget rec;
    rec.save();
    rec.beginPath();
    rec.moveTo(0, 0);
    rec.lineTo(10, 0);
    rec.lineTo(5, 10);
    rec.closePath();
    rec.clipPath();
    rec.beginPath(); // caller starts a fresh path post-clip, matching clipRect's convention
    rec.moveTo(1, 1);
    rec.lineTo(2, 2);

    const auto &ops = rec.ops();
    CHECK(ops.size() == 10);
    CHECK(ops[0].kind == K::Save);
    CHECK(ops[1].kind == K::BeginPath);
    CHECK(ops[2].kind == K::MoveTo);
    CHECK(ops[3].kind == K::LineTo);
    CHECK(ops[4].kind == K::LineTo);
    CHECK(ops[5].kind == K::ClosePath);
    CHECK(ops[6].kind == K::ClipPath);
    CHECK(ops[7].kind == K::BeginPath);
    CHECK(ops[8].kind == K::MoveTo);
    CHECK_NEAR(ops[8].args[0], 1.0, 1e-9);
    CHECK(ops[9].kind == K::LineTo);
    CHECK_NEAR(ops[9].args[0], 2.0, 1e-9);
}
TEST(RecordingTarget_pushLayer_popLayer_records)
{
    RecordingTarget rec;
    rec.pushLayer(0.5);
    rec.beginPath();
    rec.moveTo(0, 0);
    rec.lineTo(10, 10);
    rec.fillPath();
    rec.popLayer();

    CHECK(rec.count(K::PushLayer) == 1);
    CHECK(rec.count(K::PopLayer) == 1);
    const auto &ops = rec.ops();
    CHECK(ops.size() == 6);
    CHECK(ops[0].kind == K::PushLayer);
    CHECK_NEAR(ops[0].args[0], 0.5, 1e-9);
    CHECK(ops[5].kind == K::PopLayer);
}
TEST(RecordingTarget_pushLayer_nested_order)
{
    // Two nested layers must record push/pop in LIFO-consistent call order (the adapter, not
    // RecordingTarget, enforces the actual stack balance -- this proves the op stream faithfully
    // preserves nesting order for that adapter logic to rely on).
    RecordingTarget rec;
    rec.pushLayer(0.8);   // outer
    rec.pushLayer(0.3);   // inner
    rec.popLayer();       // closes inner
    rec.popLayer();       // closes outer

    const auto &ops = rec.ops();
    CHECK(ops.size() == 4);
    CHECK(ops[0].kind == K::PushLayer); CHECK_NEAR(ops[0].args[0], 0.8, 1e-9);
    CHECK(ops[1].kind == K::PushLayer); CHECK_NEAR(ops[1].args[0], 0.3, 1e-9);
    CHECK(ops[2].kind == K::PopLayer);
    CHECK(ops[3].kind == K::PopLayer);
}
TEST(ModBus_set_value_clear)
{
    ModBus b;
    CHECK_NEAR(b.value(7), 0.0, 1e-9); // unknown id -> 0
    b.set(7, 0.6);
    CHECK_NEAR(b.value(7), 0.6, 1e-9);
    b.clear();
    CHECK_NEAR(b.value(7), 0.0, 1e-9);
}
TEST(Knob_modulation_value_and_render)
{
    auto k = std::make_shared<Knob>();
    k->setRange(0.0, 1.0);
    k->setValue(0.5);
    CHECK_NEAR(k->modulatedValue(), 0.5, 1e-9); // no bus -> base value

    ModBus bus;
    k->setModBus(&bus);
    k->addModulation(1, Color::rgba(255, 0, 0), 0.25);
    CHECK((int)k->modulations().size() == 1);
    k->addModulation(1, Color::rgba(0, 255, 0), 0.9); // same source -> recolour, no dup
    CHECK((int)k->modulations().size() == 1);
    k->setModDepth(1, 0.4);
    k->setModDepth(123, 0.5); // unknown source -> no-op
    bus.set(1, 1.0);
    CHECK_NEAR(k->modulatedValue(), 0.9, 1e-9); // 0.5 + 0.4*1.0*range(1)
    bus.set(1, 5.0);
    CHECK_NEAR(k->modulatedValue(), 1.0, 1e-9); // clamp high
    bus.set(1, -5.0);
    CHECK_NEAR(k->modulatedValue(), 0.0, 1e-9); // clamp low

    auto bare = std::make_shared<Knob>();
    RecordingTarget rb; bare->render(rb);
    const int baseStrokes = rb.count(K::StrokePath);
    RecordingTarget r1; k->render(r1);
    CHECK(r1.count(K::StrokePath) > baseStrokes); // ring arc adds a stroke
    k->setModDepth(1, -0.3);                       // reach < base branch in onPaint
    RecordingTarget r2; k->render(r2);
    CHECK(r2.count(K::StrokePath) > baseStrokes);

    // bipolar routing: ring spans base ± |depth| (the LFO 2-direction case)
    auto kb = std::make_shared<Knob>();
    kb->setRange(0.0, 1.0); kb->setValue(0.5);
    kb->addModulation(3, Color::rgba(180, 120, 255), 0.3, /*bipolar=*/true);
    CHECK(kb->modulations()[0].bipolar);
    RecordingTarget rbp; kb->render(rbp);
    CHECK(rbp.count(K::StrokePath) > baseStrokes);
}
TEST(Knob_modulation_ring_drag_remove_and_value_drag)
{
    using T = Gesture::Type;
    auto k = std::make_shared<Knob>();
    k->width.set(80.0); k->height.set(80.0); // no label -> r = 37, ring0 at radius 41
    k->setRange(0.0, 1.0); k->setValue(0.5); k->setDefault(0.5);
    k->addModulation(2, Color::rgba(0, 0, 255), 0.0);

    const Point onRing{40.0, -1.0};   // 41px above centre (40,40) -> ring0 band
    k->onGesture({T::Down, onRing, onRing, PointerButton::Left});
    k->onGesture({T::DragStart, onRing, onRing, PointerButton::Left});
    k->onGesture({T::Drag, {40.0, -21.0}, onRing, PointerButton::Left}); // dy = +20 -> depth up
    CHECK(k->modulations()[0].depth > 0.0);

    k->onGesture({T::DoubleClick, onRing, onRing, PointerButton::Left}); // remove the routing
    CHECK(k->modulations().empty());

    const Point onDial{40.0, 40.0};
    k->onGesture({T::Down, onDial, onDial, PointerButton::Left});       // radius 0 -> value drag
    k->onGesture({T::DragStart, onDial, onDial, PointerButton::Left});
    const double before = k->value();
    k->onGesture({T::Drag, {40.0, 20.0}, onDial, PointerButton::Left}); // dy +20 -> value up
    CHECK(k->value() > before);

    k->setValue(0.2);
    k->onGesture({T::DoubleClick, onDial, onDial, PointerButton::Left}); // dial dbl-click -> reset
    CHECK_NEAR(k->value(), 0.5, 1e-9);
}
TEST(Segment_clipToBounds_emits_clip_around_children)
{
    auto root = std::make_shared<Segment>();
    root->width.set(100);
    root->height.set(50);
    auto child = std::make_shared<RectangleSegment>();
    child->width.set(40); child->height.set(40);
    root->addChild(child);

    RecordingTarget off;
    root->render(off);
    CHECK(off.count(K::ClipRect) == 0); // no clip by default

    root->clipToBounds = true;
    RecordingTarget on;
    root->render(on);
    CHECK(on.count(K::ClipRect) == 1); // clip wraps the child subtree
}

// ───────────────────────── widgets ─────────────────────────
TEST(Knob_drag_keys_and_render)
{
    auto k = std::make_shared<Knob>();
    k->label = "DRIVE";
    double last = -1;
    k->onChange = [&](double v) { last = v; };

    // vertical drag up by 80px over sensitivity 160 -> +0.5 of [0,1]
    k->onGesture({Gesture::Type::DragStart, {0, 0}, {0, 0}, PointerButton::Left});
    k->onGesture({Gesture::Type::Drag, {0, -80}, {0, 0}, PointerButton::Left});
    CHECK_NEAR(k->value(), 0.5, 1e-9);
    CHECK_NEAR(last, 0.5, 1e-9);

    // keyboard step (focused)
    k->requestFocus();
    k->dispatchKey({KeyEvent::Type::Down, 39}); // right -> +1/20
    CHECK_NEAR(k->value(), 0.55, 1e-9);
    k->dispatchKey({KeyEvent::Type::Down, 37}); // left -> -1/20
    CHECK_NEAR(k->value(), 0.5, 1e-9);
    CHECK(!k->dispatchKey({KeyEvent::Type::Down, 65})); // non-arrow ignored
    k->onGesture({Gesture::Type::Click, {0, 0}, {0, 0}, PointerButton::Left}); // default branch

    // FR-9a: double-click resets to the default value and emits onChange.
    k->setDefault(0.25);
    last = -1;
    k->onGesture({Gesture::Type::DoubleClick, {0, 0}, {0, 0}, PointerButton::Left});
    CHECK_NEAR(k->value(), 0.25, 1e-9);
    CHECK_NEAR(last, 0.25, 1e-9);

    RecordingTarget rec;
    k->render(rec);
    CHECK(rec.count(K::StrokePath) >= 3); // track + value + indicator
    CHECK(rec.count(K::DrawText) == 1);   // label

    // value 0 + no label exercises the "no value arc" and "no label" branches
    auto k0 = std::make_shared<Knob>();
    RecordingTarget rec0;
    k0->render(rec0);
    CHECK(rec0.count(K::DrawText) == 0);

    // smooth value animation: advance() springs the display toward the target.
    auto ks = std::make_shared<Knob>();
    ks->setValue(0.0);
    ks->advance(0.0);     // first tick: dt<=0 path, seeds display
    ks->setValue(1.0);    // jump the target
    ks->advance(16.0);    // dt>0 spring step
    double mid = ks->value(); // (target is 1; display lags but value() is the target)
    CHECK_NEAR(mid, 1.0, 1e-9);
    ks->advance(999.0);   // huge dt -> clamped to 0.05
    for (int i = 0; i < 60; ++i) ks->advance(1000.0 + i * 16.0); // settle
    RecordingTarget recS; ks->render(recS);
    CHECK(recS.count(K::StrokePath) >= 3);

    // displayNormalized span<=0 branch (min==max) renders without a value arc
    auto kz = std::make_shared<Knob>();
    kz->setRange(5.0, 5.0);
    kz->advance(0.0);
    RecordingTarget recZ; kz->render(recZ);
    CHECK(recZ.count(K::StrokePath) >= 2); // track + indicator (no value arc)
}
TEST(ToggleSwitch_toggle_animate_render)
{
    auto sw = std::make_shared<ToggleSwitch>();
    int changes = 0; bool lastState = false;
    sw->onChange = [&](bool on) { ++changes; lastState = on; };

    sw->advance(0); // stamp time
    RecordingTarget off; sw->render(off); // trackOff branch (t01==0)

    sw->onGesture({Gesture::Type::Click, {10, 10}, {10, 10}, PointerButton::Left});
    CHECK(sw->on());
    CHECK(changes == 1 && lastState == true);
    sw->advance(80);   // mid animation
    sw->advance(200);  // settled at 1
    RecordingTarget on; sw->render(on); // trackOn branch (t01>=0.5)
    CHECK(on.count(K::FillPath) >= 1);

    sw->onGesture({Gesture::Type::Down, {10, 10}, {10, 10}, PointerButton::Left}); // non-Click fallback
    sw->requestFocus();
    sw->dispatchKey({KeyEvent::Type::Down, 32}); // space toggles off
    CHECK(!sw->on());
    CHECK(changes == 2);
    CHECK(!sw->dispatchKey({KeyEvent::Type::Down, 65})); // non-confirm ignored
    sw->setOn(true);
    CHECK(sw->on());
}
TEST(ProgressBar_value_clamp_and_render)
{
    auto p = std::make_shared<ProgressBar>();
    p->setValue(0.5);
    CHECK_NEAR(p->value(), 0.5, 1e-9);           // logical value is immediate
    RecordingTarget preAdv; p->render(preAdv);
    CHECK(preAdv.count(K::FillPath) == 1);        // shown level eases from 0: track only, no fill yet
    for (int i = 0; i <= 40; ++i) p->advance(i * 25.0);  // step frames so the spring settles to 0.5
    RecordingTarget mid; p->render(mid);
    CHECK(mid.count(K::FillPath) >= 2);           // track + fill once eased in

    p->setValue(-1); CHECK_NEAR(p->value(), 0.0, 1e-9); // clamp low
    for (int i = 41; i <= 90; ++i) p->advance(i * 25.0); // ease back down toward 0
    RecordingTarget zero; p->render(zero);              // no-fill branch (near-zero level)
    CHECK(zero.count(K::FillPath) == 1);
    p->setValue(2); CHECK_NEAR(p->value(), 1.0, 1e-9);  // clamp high
    CHECK(!p->hitTest({5, 5}));                          // input passes through
}
TEST(ComboBox_open_select_and_render)
{
    auto c = std::make_shared<ComboBox>();
    c->setOptions({"Sine", "Saw", "Square"});
    int picked = -1;
    c->onChange = [&](int i) { picked = i; };

    RecordingTarget closed; c->render(closed);
    CHECK(!c->isOpen());

    c->onGesture({Gesture::Type::Click, {10, 10}, {10, 10}, PointerButton::Left}); // open
    CHECK(c->isOpen());
    for (double tt = 0.0; tt <= 200.0; tt += 16.0) c->advance(tt); // past the open reveal
    RecordingTarget open; c->render(open);       // main pass: field only (no rows)
    CHECK(open.count(K::DrawText) == 1);         // just the selected-option label
    RecordingTarget ov; c->renderOverlay(ov);    // overlay pass: the dropdown rows
    CHECK(ov.count(K::DrawText) >= 3);            // 3 option rows drawn on top

    // click row 1 (Saw): y in popup band
    c->onGesture({Gesture::Type::Click, {10, 32 + 28 + 5}, {0, 0}, PointerButton::Left});
    CHECK(c->selectedIndex() == 1);
    CHECK(picked == 1);
    CHECK(!c->isOpen());

    // open then click the field area closes; open then click below rows closes
    c->onGesture({Gesture::Type::Click, {10, 10}, {0, 0}, PointerButton::Left}); // open
    c->onGesture({Gesture::Type::Click, {10, 5}, {0, 0}, PointerButton::Left});  // field -> close
    CHECK(!c->isOpen());
    c->onGesture({Gesture::Type::Click, {10, 10}, {0, 0}, PointerButton::Left}); // open
    c->onGesture({Gesture::Type::Click, {10, 9999}, {0, 0}, PointerButton::Left}); // beyond -> close
    CHECK(!c->isOpen());

    // hitTest geometry (closed) + open-popup band + selection guards
    CHECK(c->hitTest({5, 5}));        // field band
    CHECK(!c->hitTest({-1, 5}));      // outside x
    CHECK(!c->hitTest({5, 9999}));    // below field while closed
    c->onGesture({Gesture::Type::Click, {10, 10}, {0, 0}, PointerButton::Left}); // open
    CHECK(c->hitTest({10, 36}));      // inside the popup band
    c->onGesture({Gesture::Type::Click, {10, 5}, {0, 0}, PointerButton::Left});  // close again
    c->setSelectedIndex(2); CHECK(c->selectedIndex() == 2);
    c->setSelectedIndex(99); CHECK(c->selectedIndex() == 2); // out of range ignored
    c->setOptions({"only"}); CHECK(c->selectedIndex() == 0);  // shrink resets

    auto empty = std::make_shared<ComboBox>();
    RecordingTarget e; empty->render(e); // empty options branch
    empty->onGesture({Gesture::Type::Down, {1, 1}, {1, 1}, PointerButton::Left}); // non-Click branch
}
TEST(ComboBox_overlay_on_top_and_raise)
{
    auto root = std::make_shared<Segment>();
    root->width.set(300.0); root->height.set(300.0);
    auto combo = std::make_shared<ComboBox>();
    combo->setOptions({"a", "b", "c"});
    combo->x.set(0.0); combo->y.set(0.0); combo->width.set(160.0); combo->height.set(32.0);
    int picked = -1; combo->onChange = [&](int i) { picked = i; };
    // a sibling that OVERLAPS the dropdown band (y 32..116), added AFTER the combo
    auto sib = std::make_shared<Button>("x");
    sib->x.set(0.0); sib->y.set(40.0); sib->width.set(160.0); sib->height.set(50.0);
    bool sibClicked = false; sib->onClick = [&] { sibClicked = true; };
    root->addChild(combo); root->addChild(sib);

    RecordingTarget closed; root->renderOverlay(closed);
    CHECK(closed.count(K::DrawText) == 0);  // nothing in the overlay pass while closed

    root->onGesture({Gesture::Type::Click, {10, 10}, {10, 10}, PointerButton::Left});  // open
    CHECK(combo->isOpen());
    CHECK(root->children().back().get() == combo.get());  // raised to front of input/z

    for (double tt = 0.0; tt <= 200.0; tt += 16.0) root->advance(tt); // past the open reveal
    RecordingTarget ov; root->renderOverlay(ov);
    CHECK(ov.count(K::DrawText) >= 3);      // dropdown rows drawn on top

    // click a row that also sits inside the sibling's bounds -> combo wins, not the sibling
    root->onGesture({Gesture::Type::Click, {10, 46}, {10, 46}, PointerButton::Left});
    CHECK(picked == 0);
    CHECK(!sibClicked);
}

TEST(Knob_mod_ring_grow_in_and_reduced_motion)
{
    auto k = std::make_shared<Knob>();
    k->setRange(0.0, 1.0); k->setValue(0.5);
    k->advance(0.0); // seed the display spring (mLastMs)
    k->addModulation(7, Color::rgba(0, 200, 255), 0.4);
    CHECK_NEAR(k->modulations()[0].appear.value(), 0.0, 1e-9); // ring starts hidden
    for (double tt = 16.0; tt <= 400.0; tt += 16.0) k->advance(tt);
    CHECK_NEAR(k->modulations()[0].appear.value(), 1.0, 1e-2); // grew in to full
    // re-colouring an already-routed source must NOT restart the grow-in
    k->addModulation(7, Color::rgba(255, 0, 0), 0.4);
    CHECK((int)k->modulations().size() == 1);
    CHECK_NEAR(k->modulations()[0].appear.value(), 1.0, 1e-2);

    // reduced motion: a new ring appears fully on the first advance (no grow-in)
    setReducedMotion(true);
    auto k2 = std::make_shared<Knob>();
    k2->setRange(0.0, 1.0); k2->setValue(0.5);
    k2->addModulation(1, Color::rgba(0, 255, 0), 0.5);
    k2->advance(0.0);
    CHECK_NEAR(k2->modulations()[0].appear.value(), 1.0, 1e-9);
    setReducedMotion(false);
}

TEST(ComboBox_reveal_animation_and_reduced_motion)
{
    auto c = std::make_shared<ComboBox>();
    c->setOptions({"A", "B", "C"});

    // opening: the overlay is empty on the very first frame (progress ~0), then reveals
    c->onGesture({Gesture::Type::Click, {5, 5}, {5, 5}, PointerButton::Left});
    CHECK(c->isOpen());
    RecordingTarget f0; c->renderOverlay(f0);
    CHECK(f0.count(K::DrawText) == 0);       // reveal not advanced -> nothing drawn yet
    c->advance(0.0); c->advance(80.0);       // mid-reveal
    RecordingTarget mid; c->renderOverlay(mid);
    CHECK(mid.count(K::DrawText) >= 3);       // rows now visible (faded/sliding in)
    for (double tt = 96.0; tt <= 300.0; tt += 16.0) c->advance(tt);
    RecordingTarget full; c->renderOverlay(full);
    CHECK(full.count(K::DrawText) >= 3);

    // closing animates out, then the overlay is empty
    c->onGesture({Gesture::Type::Click, {5, 5}, {5, 5}, PointerButton::Left}); // field click closes
    CHECK(!c->isOpen());
    for (double tt = 320.0; tt <= 620.0; tt += 16.0) c->advance(tt);
    RecordingTarget closed; c->renderOverlay(closed);
    CHECK(closed.count(K::DrawText) == 0);    // fully closed -> nothing in the overlay

    // reduced motion: opens fully immediately (no reveal frame)
    setReducedMotion(true);
    auto c2 = std::make_shared<ComboBox>();
    c2->setOptions({"X", "Y"});
    c2->onGesture({Gesture::Type::Click, {5, 5}, {5, 5}, PointerButton::Left});
    c2->advance(0.0);
    RecordingTarget r2; c2->renderOverlay(r2);
    CHECK(r2.count(K::DrawText) >= 2);
    setReducedMotion(false);
}

TEST(TabView_pages_and_tab_clicks)
{
    auto tv = std::make_shared<TabView>();
    auto p0 = std::make_shared<RectangleSegment>();
    auto p1 = std::make_shared<RectangleSegment>();
    int tab = -1;
    tv->onChange = [&](int i) { tab = i; };
    tv->addPage("One", p0);
    tv->addPage("Two", p1);
    CHECK(tv->pageCount() == 2);
    CHECK(p0->visible && !p1->visible);

    RecordingTarget rec; tv->render(rec);
    CHECK(rec.count(K::DrawText) == 2); // two tab labels

    // width 300, 2 tabs -> tw 150; click x=160 selects tab 1
    tv->onGesture({Gesture::Type::Click, {160, 10}, {160, 10}, PointerButton::Left});
    CHECK(tv->selectedIndex() == 1);
    CHECK(tab == 1);
    CHECK(!p0->visible && p1->visible);

    tv->onGesture({Gesture::Type::Down, {160, 10}, {160, 10}, PointerButton::Left}); // non-Click branch
    tv->setSelectedIndex(99); CHECK(tv->selectedIndex() == 1);  // ignored

    auto empty = std::make_shared<TabView>();
    RecordingTarget e; empty->render(e);                         // n==0 onPaint return
    empty->onGesture({Gesture::Type::Click, {10, 10}, {10, 10}, PointerButton::Left}); // n==0 guard
}

TEST(TabView_active_indicator_bar)
{
    // FR-21: activeIndicatorHeight opts into a thin top bar on the active tab only;
    // the default (0) draws nothing extra, so existing themes are unaffected.
    TabStyle style;
    style.tabIdle = {Paint::filled(Color::rgba(40, 40, 40)), 4.0};
    style.tabActive = {Paint::filled(Color::rgba(80, 80, 80)), 4.0};
    style.label = {Color::rgba(255, 255, 255), 12.0};

    auto tv = std::make_shared<TabView>(style);
    tv->addPage("One", std::make_shared<RectangleSegment>());
    tv->addPage("Two", std::make_shared<RectangleSegment>());

    RecordingTarget plain; tv->render(plain);
    CHECK(plain.count(K::FillPath) == 2);  // just the two tab bodies -- no indicator by default

    style.activeIndicatorColor = Color::rgba(79, 126, 247);
    style.activeIndicatorHeight = 2.0;
    tv->setStyle(style);

    RecordingTarget withBar; tv->render(withBar);
    CHECK(withBar.count(K::FillPath) == 3);  // + one indicator bar, on the active tab only

    // the active tab's title is coloured via labelActive, not label
    style.label = {Color::rgba(150, 150, 150), 12.0};
    style.labelActive = {Color::rgba(79, 126, 247), 12.0};
    tv->setStyle(style);
    RecordingTarget colored; tv->render(colored);
    bool sawIdleColor = false, sawActiveColor = false;
    for (const auto &op : colored.ops())
    {
        if (op.kind != K::SetFill) continue;
        if (op.color == style.label.color) sawIdleColor = true;
        if (op.color == style.labelActive.color) sawActiveColor = true;
    }
    CHECK(sawIdleColor && sawActiveColor);
}

TEST(ScrollView_clip_drag_and_thumb)
{
    auto sv = std::make_shared<ScrollView>();
    sv->width.set(200); sv->height.set(200);
    auto content = std::make_shared<RectangleSegment>();
    content->width.set(180); content->height.set(400);
    sv->setContent(content);
    sv->setContentHeight(400);
    CHECK_NEAR(sv->maxOffset(), 200.0, 1e-9);

    RecordingTarget rec; sv->render(rec);
    CHECK(rec.count(K::ClipRect) >= 1); // clipToBounds + scrollable

    // content drag: drag up 40px -> offset 40
    sv->onGesture({Gesture::Type::DragStart, {50, 50}, {50, 50}, PointerButton::Left});
    sv->onGesture({Gesture::Type::Drag, {50, 10}, {50, 50}, PointerButton::Left});
    CHECK_NEAR(sv->offset(), 40.0, 1e-9);
    // drag far the other way clamps to 0
    sv->onGesture({Gesture::Type::DragStart, {50, 50}, {50, 50}, PointerButton::Left});
    sv->onGesture({Gesture::Type::Drag, {50, 400}, {50, 50}, PointerButton::Left});
    CHECK_NEAR(sv->offset(), 0.0, 1e-9);
    // thumb drag (x in scrollbar zone): scale = 400/200 = 2
    sv->onGesture({Gesture::Type::DragStart, {195, 10}, {195, 10}, PointerButton::Left});
    sv->onGesture({Gesture::Type::Drag, {195, 60}, {195, 10}, PointerButton::Left});
    CHECK_NEAR(sv->offset(), 100.0, 1e-9);
    // content drag past the end clamps to maxOffset (clamp-high branch)
    sv->onGesture({Gesture::Type::DragStart, {50, 50}, {50, 50}, PointerButton::Left});
    sv->onGesture({Gesture::Type::Drag, {50, -400}, {50, 50}, PointerButton::Left});
    CHECK_NEAR(sv->offset(), 200.0, 1e-9);
    // A bare Move over the scrollbar gutter (x>content width, so it misses the content child)
    // reaches ScrollView's own handler and falls through to the base (non-drag fallback).
    sv->onGesture({Gesture::Type::Move, {195, 50}, {195, 50}, PointerButton::Left});

    // non-scrollable: maxOffset 0, onPaint skips the scrollbar branch
    auto sv2 = std::make_shared<ScrollView>();
    sv2->width.set(200); sv2->height.set(200);
    sv2->setContent(std::make_shared<RectangleSegment>()); // replace path (clearChildren)
    sv2->setContentHeight(100);
    CHECK_NEAR(sv2->maxOffset(), 0.0, 1e-9);
    RecordingTarget rec2; sv2->render(rec2);
}
TEST(LineGraph_series_and_modes)
{
    auto g = std::make_shared<LineGraph>();
    g->setRange(-1, 1);
    g->setGridLines(4);
    g->setSeries({0.0, 0.5, -0.5, 1.0, -1.0});
    RecordingTarget filled; g->render(filled);
    CHECK(filled.count(K::FillPath) >= 2); // background + area fill
    CHECK(filled.count(K::StrokePath) >= 4); // 3 grid lines + series line

    g->setFilled(false);
    RecordingTarget noFill; g->render(noFill);
    CHECK(noFill.count(K::StrokePath) >= 4);

    // span<=0 guard + still draws
    auto g2 = std::make_shared<LineGraph>();
    g2->setRange(1, 1);
    g2->setSeries({1.0, 1.0});
    RecordingTarget flat; g2->render(flat);

    // empty + single-point series take the early-out
    auto g3 = std::make_shared<LineGraph>();
    g3->setSeries({});
    RecordingTarget empty; g3->render(empty);
    g3->setSeries({0.5});
    RecordingTarget one; g3->render(one);
    CHECK(!g3->hitTest({5, 5})); // non-interactive
}

// ───────────────── Segment base paths (guards/recursion/focus) ─────────────────
TEST(Segment_core_paths)
{
    auto root = std::make_shared<Segment>();
    root->width.set(100); root->height.set(100);
    root->addChild(nullptr);            // null-child guard
    CHECK(root->childCount() == 0);

    auto child = std::make_shared<Segment>();
    child->width.set(40); child->height.set(40);
    root->addChild(child);
    CHECK(root->hitTest({10, 10}));     // child hit (children loop)
    CHECK(!root->hitTest({500, 500}));  // all miss -> self bounds false

    root->advance(16);                  // recurses into child->advance
    root->onGesture({Gesture::Type::Down, {60, 60}, {60, 60}, PointerButton::Left}); // topmostChildAt -> nullptr
    root->clearChildren();              // child->mParent reset
    CHECK(root->childCount() == 0);

    // inputTransparent segment: hitTest short-circuits + handleGesture returns false
    auto transparent = std::make_shared<Segment>();
    transparent->width.set(50); transparent->height.set(50);
    transparent->inputTransparent = true;
    CHECK(!transparent->hitTest({5, 5}));
    transparent->onGesture({Gesture::Type::Down, {5, 5}, {5, 5}, PointerButton::Left});

    // requestFocus on a non-focusable segment is a no-op
    auto plain = std::make_shared<Segment>();
    plain->requestFocus();
    CHECK(!plain->hasFocus());

    // focused-but-disabled segment: handleKey returns false
    auto k = std::make_shared<Segment>();
    k->focusable = true;
    k->requestFocus();
    CHECK(k->hasFocus());
    k->enabled = false;
    CHECK(!k->dispatchKey({KeyEvent::Type::Down, 65}));

    // hidden segment: dispatchGesture early-outs
    auto hidden = std::make_shared<Segment>();
    hidden->visible = false;
    hidden->onGesture({Gesture::Type::Down, {1, 1}, {1, 1}, PointerButton::Left});

    // a stack-allocated Segment exercises the destructor directly
    { Segment local; local.width.set(1.0); }
}

// ───────────────── baseline controls (render + interaction) ─────────────────
TEST(Button_full)
{
    auto btn = std::make_shared<Button>("OK");
    int clicks = 0;
    btn->onClick = [&] { ++clicks; };
    RecordingTarget r; btn->render(r);            // ensureVisualTree + body + label onPaint
    CHECK(r.count(K::DrawText) == 1);
    btn->onGesture({Gesture::Type::Down, {5, 5}, {5, 5}, PointerButton::Left});
    btn->onGesture({Gesture::Type::Up, {5, 5}, {5, 5}, PointerButton::Left});
    btn->onGesture({Gesture::Type::Click, {5, 5}, {5, 5}, PointerButton::Left});
    CHECK(clicks == 1);
    btn->onGesture({Gesture::Type::Drop, {5, 5}, {5, 5}, PointerButton::Left});  // reset pressed
    btn->onGesture({Gesture::Type::Move, {5, 5}, {5, 5}, PointerButton::Left});  // default fallback
    btn->requestFocus();
    btn->dispatchKey({KeyEvent::Type::Down, 13});  // Enter -> onClick
    CHECK(clicks == 2);
    CHECK(!btn->dispatchKey({KeyEvent::Type::Down, 65})); // non-confirm
    btn->setStyle(Theme::basicTheme().button);
}
TEST(Slider_full)
{
    auto sl = std::make_shared<Slider>();
    RecordingTarget r; sl->render(r);
    CHECK(r.count(K::FillPath) >= 2);
    sl->onGesture({Gesture::Type::DragStart, {40, 14}, {40, 14}, PointerButton::Left});  // value from x
    CHECK(sl->value() > 0.0);
    sl->onGesture({Gesture::Type::Drag, {80, 14}, {80, 14}, PointerButton::Left});
    sl->onGesture({Gesture::Type::Drag, {160, 14}, {160, 14}, PointerButton::Left});
    sl->onGesture({Gesture::Type::Drag, {-10, 14}, {-10, 14}, PointerButton::Left}); // clamp low
    sl->onGesture({Gesture::Type::Drag, {200, 14}, {200, 14}, PointerButton::Left});  // clamp high
    sl->onGesture({Gesture::Type::Move, {10, 10}, {10, 10}, PointerButton::Left}); // default fallback
    // FR-9a: double-click resets to the default value.
    sl->setValue(0.9);
    sl->setDefault(0.3);
    sl->onGesture({Gesture::Type::DoubleClick, {120, 14}, {120, 14}, PointerButton::Left});
    CHECK_NEAR(sl->value(), 0.3, 1e-9);
    sl->requestFocus();
    sl->setAnalog(true);
    sl->dispatchKey({KeyEvent::Type::Down, 39}); // right
    sl->dispatchKey({KeyEvent::Type::Down, 37}); // left
    sl->setAnalog(false);
    sl->dispatchKey({KeyEvent::Type::Down, 39}); // step=1 branch
    CHECK(!sl->dispatchKey({KeyEvent::Type::Up, 39})); // type != Down -> false
    sl->dispatchKey({KeyEvent::Type::Down, 65});       // non-arrow -> Segment::handleKey
    sl->setRange(5.0, 5.0);    // span 0 -> normalizedValue() 0 branch on next render
    sl->render(r);
    sl->setStyle(Theme::basicTheme().slider);
    // width 0 -> valueForLocalX returns minimum()
    auto sl0 = std::make_shared<Slider>();
    sl0->width.set(0.0);
    sl0->onGesture({Gesture::Type::Drag, {5, 5}, {5, 5}, PointerButton::Left});  // width 0 -> minimum
    CHECK_NEAR(sl0->value(), sl0->minimum(), 1e-9);
}
TEST(Slider_double_click_resets_despite_click_jumps)
{
    // Feed a real double-click (down/up/down/up) at a NON-default position through the
    // recognizer with click-jumps on; the value must end at the default, not the cursor.
    auto root = std::make_shared<Segment>();
    root->width.set(200); root->height.set(40);
    auto sl = std::make_shared<Slider>();  // width 160, range 0..1
    sl->x.set(0); sl->y.set(0);
    sl->setValue(0.1); sl->setDefault(0.25);
    double maxSeen = 0.0; sl->onChange = [&](double v) { if (v > maxSeen) maxSeen = v; };
    root->addChild(sl);
    GestureRecognizer rec; rec.setSink([&](const Gesture &g) { root->onGesture(g); });
    rec.feed({RawPointer::Kind::Down, {160, 14}, PB::Left, 0});
    rec.feed({RawPointer::Kind::Up, {160, 14}, PB::Left, 10});   // -> Click (deferred, not committed)
    rec.feed({RawPointer::Kind::Down, {160, 14}, PB::Left, 60});
    rec.feed({RawPointer::Kind::Up, {160, 14}, PB::Left, 70});   // -> DoubleClick cancels + resets
    CHECK_NEAR(sl->value(), 0.25, 1e-9);  // ends at the default, not the clicked cursor
    CHECK(maxSeen <= 0.25 + 1e-9);         // the cursor value (1.0) never fired -> no flash/re-render
}

TEST(Slider_slow_double_click_no_flash_toward_cursor)
{
    // A SLOW double-click (near the recognizer's 300ms window) with frames ticking
    // between the two clicks: the deferred click-jump must NOT commit before the
    // second click cancels it (guard >= double-click window), so the value never
    // flashes toward the cursor and ends at the default. Regression for guard 240 <
    // window 300, where the jump committed mid-double-click.
    auto root = std::make_shared<Segment>();
    root->width.set(200); root->height.set(40);
    auto sl = std::make_shared<Slider>();  // width 160, range 0..1
    sl->setValue(0.1); sl->setDefault(0.25);
    double maxSeen = 0.0; sl->onChange = [&](double v) { if (v > maxSeen) maxSeen = v; };
    root->addChild(sl);
    GestureRecognizer rec; rec.setSink([&](const Gesture &g) { root->onGesture(g); });
    double now = 0; auto adv = [&](double to) { while (now < to) { now += 16; root->advance(now); } };
    rec.feed({RawPointer::Kind::Down, {160, 14}, PB::Left, 0});
    rec.feed({RawPointer::Kind::Up,   {160, 14}, PB::Left, 10});   // Click (deferred)
    adv(280);                                                      // frames tick; guard must NOT fire yet
    CHECK(sl->value() < 0.9);                                      // no early commit toward the cursor (1.0)
    rec.feed({RawPointer::Kind::Down, {160, 14}, PB::Left, 280});  // 2nd press cancels the pending jump
    rec.feed({RawPointer::Kind::Up,   {160, 14}, PB::Left, 290});  // DoubleClick -> reset
    adv(1600);
    CHECK_NEAR(sl->value(), 0.25, 1e-9);   // ends at the default
    CHECK(maxSeen <= 0.25 + 1e-9);          // never flashed to the cursor value
}

TEST(Slider_onChange_fires_on_interaction)
{
    auto sl = std::make_shared<Slider>();  // width 160, range 0..1
    double last = -1.0; int calls = 0;
    sl->onChange = [&](double v) { last = v; ++calls; };

    sl->onGesture({Gesture::Type::DragStart, {80, 14}, {80, 14}, PointerButton::Left});  // drag sets immediately
    CHECK(calls == 1);
    CHECK_NEAR(last, sl->value(), 1e-9);          // reports the new value
    CHECK_NEAR(sl->value(), 0.5, 1e-9);
    sl->onGesture({Gesture::Type::Drag, {160, 14}, {160, 14}, PointerButton::Left});
    CHECK_NEAR(last, sl->value(), 1e-9);

    sl->setDefault(0.25);
    sl->onGesture({Gesture::Type::DoubleClick, {10, 14}, {10, 14}, PointerButton::Left});
    CHECK_NEAR(last, 0.25, 1e-9);                  // double-click reset fires onChange

    sl->requestFocus();
    sl->dispatchKey({KeyEvent::Type::Down, 39});   // right arrow
    sl->dispatchKey({KeyEvent::Type::Down, 37});   // left arrow
    CHECK(calls >= 5);

    const int before = calls;
    sl->setValue(0.9);                             // programmatic -> no callback
    CHECK(calls == before);
}

TEST(Slider_click_jumps_and_display_springs)
{
    auto sl = std::make_shared<Slider>();  // width 160, range 0..1, clickJumps default on
    double last = -1.0;
    sl->onChange = [&](double v) { last = v; };
    sl->advance(0.0);  // seed the display at the current value (as the frame loop does)

    // a click to the far right is DEFERRED (double-click guard), then commits
    sl->onGesture({Gesture::Type::Click, {160, 14}, {160, 14}, PointerButton::Left});
    sl->advance(20.0);
    CHECK_NEAR(sl->value(), 0.0, 1e-9);          // not committed yet (deferred)
    for (double t = 40.0; t <= 320.0; t += 20.0) sl->advance(t);  // past the guard
    CHECK_NEAR(sl->value(), 1.0, 1e-9);          // committed to the cursor
    CHECK_NEAR(last, 1.0, 1e-9);

    // the DISPLAYED value then springs there over a few frames
    const double early = sl->displayValue();
    CHECK(early < 1.0);                           // mid-glide, not snapped
    for (double t = 340.0; t <= 900.0; t += 16.0) sl->advance(t);
    CHECK_NEAR(sl->displayValue(), 1.0, 1e-2);   // settles at the target
}

TEST(Slider_gradient_track)
{
    auto sl = std::make_shared<Slider>();
    sl->width.set(120.0); sl->setRange(2000.0, 9000.0);

    RecordingTarget plain;
    sl->render(plain);
    CHECK(plain.count(DrawOp::Kind::SetLinearFill) == 0);  // solid track: no gradient

    sl->setTrackGradient(Color{0.2f, 0.4f, 1.0f, 1.0f}, Color{1.0f, 0.85f, 0.3f, 1.0f});
    RecordingTarget grad;
    sl->render(grad);
    CHECK(grad.count(DrawOp::Kind::SetLinearFill) == 1);   // one gradient track
    CHECK(grad.count(DrawOp::Kind::FillPath) >= 1);        // filled as a path
}

TEST(Slider_clickJumps_off)
{
    auto sl = std::make_shared<Slider>();  // width 160, range 0..1
    sl->setClickJumps(false);
    sl->setValue(0.4);
    sl->setDefault(0.2);
    double last = -1.0; int calls = 0;
    sl->onChange = [&](double v) { last = v; ++calls; };

    // a bare press does NOT change the value (no click-to-position)
    sl->onGesture({Gesture::Type::Down, {120, 14}, {120, 14}, PointerButton::Left});
    CHECK_NEAR(sl->value(), 0.4, 1e-9);
    CHECK(calls == 0);
    // dragging DOES change it
    sl->onGesture({Gesture::Type::DragStart, {80, 14}, {80, 14}, PointerButton::Left});
    CHECK_NEAR(sl->value(), 0.5, 1e-9);
    sl->onGesture({Gesture::Type::Drag, {40, 14}, {40, 14}, PointerButton::Left});
    CHECK_NEAR(sl->value(), 0.25, 1e-9);
    CHECK_NEAR(last, 0.25, 1e-9);
    // double-click reliably resets to the default
    sl->onGesture({Gesture::Type::DoubleClick, {150, 14}, {150, 14}, PointerButton::Left});
    CHECK_NEAR(sl->value(), 0.2, 1e-9);
}

namespace
{
    // The world-space x-span [lo,hi] of the 3rd rect segment rendered by a Slider
    // (track, then rangeFill, then the thumb circle) -- rangeFill's cornerRadius
    // must be 0 (a plain rect) so its path is exactly 4 corners with clean x's.
    std::pair<double, double> rangeFillXSpan(const std::vector<DrawOp> &ops)
    {
        std::vector<size_t> transforms;
        for (size_t i = 0; i < ops.size(); ++i)
            if (ops[i].kind == K::SetTransform) transforms.push_back(i);
        if (transforms.size() < 3) return {0.0, 0.0};
        const size_t start = transforms[2];
        const size_t end = transforms.size() > 3 ? transforms[3] : ops.size();
        const double worldX = ops[start].transform.e;
        double lo = 1e18, hi = -1e18;
        for (size_t i = start; i < end; ++i)
        {
            if (ops[i].kind == K::MoveTo || ops[i].kind == K::LineTo)
            {
                lo = std::min(lo, ops[i].args[0]);
                hi = std::max(hi, ops[i].args[0]);
            }
        }
        return {worldX + lo, worldX + hi};
    }

    size_t transformCount(const std::vector<DrawOp> &ops)
    {
        size_t c = 0;
        for (const auto &op : ops) if (op.kind == K::SetTransform) ++c;
        return c;
    }
    // World x (translation) of the n-th SetTransform — one per rendered child, in
    // add order: [0]=slider, [1]=track, [2]=rangeFill, [3]=subFill, [4]=subTick, [5]=thumb.
    double nthTransformX(const std::vector<DrawOp> &ops, size_t n)
    {
        size_t c = 0;
        for (const auto &op : ops)
            if (op.kind == K::SetTransform) { if (c == n) return op.transform.e; ++c; }
        return -1e18;
    }
}

TEST(Slider_range_fill_anchors_at_zero_when_range_spans_it)
{
    // FR-8: a range spanning zero fills from the zero-crossing, not the left edge, so
    // the bar reads as "distance from neutral" for e.g. a -100..100 exposure slider.
    SliderStyle style = Theme::basicTheme().slider;
    style.rangeFill.cornerRadius = 0.0;  // plain rect -> unambiguous path x's

    auto sl = std::make_shared<Slider>(style);
    sl->width.set(200.0);
    sl->setRange(-100.0, 100.0);

    // The rendered fill follows the spring-smoothed DISPLAY value (FR-9a), not the
    // target directly -- advance well past the settle time after each setValue so
    // the display has actually caught up before checking the rendered span.
    double now = 0.0;
    auto settle = [&] { for (int i = 0; i < 40; ++i) { now += 20.0; sl->advance(now); } };

    sl->setValue(0.0);
    settle();
    RecordingTarget t0; sl->render(t0);
    auto [lo0, hi0] = rangeFillXSpan(t0.ops());
    CHECK_NEAR(lo0, 100.0, 1.0);  // no width yet: anchored exactly at the zero-crossing (50%)
    CHECK_NEAR(hi0, 100.0, 1.0);

    sl->setValue(50.0);          // normalized 0.75 -> x=150; fills right, from the anchor
    settle();
    RecordingTarget tp; sl->render(tp);
    auto [loP, hiP] = rangeFillXSpan(tp.ops());
    CHECK_NEAR(loP, 100.0, 1.0);
    CHECK_NEAR(hiP, 150.0, 1.0);

    sl->setValue(-50.0);         // normalized 0.25 -> x=50; fills left, up to the anchor
    settle();
    RecordingTarget tn; sl->render(tn);
    auto [loN, hiN] = rangeFillXSpan(tn.ops());
    CHECK_NEAR(loN, 50.0, 1.0);
    CHECK_NEAR(hiN, 100.0, 1.0);
}

TEST(Slider_range_fill_still_left_anchored_when_range_excludes_zero)
{
    // A range that doesn't span zero (e.g. 0..100) keeps the classic left-edge fill.
    SliderStyle style = Theme::basicTheme().slider;
    style.rangeFill.cornerRadius = 0.0;

    auto sl = std::make_shared<Slider>(style);
    sl->width.set(200.0);
    sl->setRange(0.0, 100.0);
    sl->setValue(50.0);

    RecordingTarget t; sl->render(t);
    auto [lo, hi] = rangeFillXSpan(t.ops());
    CHECK_NEAR(lo, 0.0, 1e-6);
    CHECK_NEAR(hi, 100.0, 1.0);
}

TEST(Slider_sub_value_reach)
{
    // The secondary reference reach: a coloured fill from the thumb to (value+offset)
    // plus a thin end tick, both easing on their own follower. Range -100..100, width
    // 200 -> value 0 sits at x=100; +50 at x=150; -50 at x=50.
    auto sl = std::make_shared<Slider>(Theme::basicTheme().slider);
    sl->width.set(200.0);
    sl->setRange(-100.0, 100.0);
    double now = 0.0;
    auto settle = [&] { for (int i = 0; i < 70; ++i) { now += 20.0; sl->advance(now); } };

    // No offset -> no reach: only slider, track, rangeFill, thumb are drawn (4 transforms).
    sl->setValue(0.0); sl->setSubValueOffset(0.0); settle();
    { RecordingTarget t; sl->render(t); CHECK(transformCount(t.ops()) == 4); }

    // +50 offset -> reach right: fill starts at the thumb (x=100), tick at the end (x=150).
    sl->setSubValueOffset(50.0); settle();
    {
        RecordingTarget t; sl->render(t);
        CHECK(transformCount(t.ops()) == 6);                    // + subFill + subTick
        CHECK_NEAR(nthTransformX(t.ops(), 3), 100.0, 1.5);      // subFill x = min(thumb, sub) = thumb
        CHECK_NEAR(nthTransformX(t.ops(), 4) + 0.8, 150.0, 1.5); // tick centre = reach end (value+offset)
    }

    // -50 offset -> reach left: fill starts at the sub (x=50), tick at x=50 (negative works).
    sl->setSubValueOffset(-50.0); settle();
    {
        RecordingTarget t; sl->render(t);
        CHECK(transformCount(t.ops()) == 6);
        CHECK_NEAR(nthTransformX(t.ops(), 3), 50.0, 1.5);       // subFill x = min(thumb, sub) = sub
        CHECK_NEAR(nthTransformX(t.ops(), 4) + 0.8, 50.0, 1.5);  // tick centre = reach end
    }

    // An offset past the range clamps the reach to the track edge (x=200).
    sl->setSubValueOffset(500.0); settle();
    {
        RecordingTarget t; sl->render(t);
        CHECK_NEAR(nthTransformX(t.ops(), 4) + 0.8, 200.0, 1.5);
    }
}

TEST(AbstractSlider_clamp_reversed_range)
{
    AbstractSlider s(0.0, 5.0, 1.0); // max < min -> clamp returns mMin
    CHECK_NEAR(s.value(), 5.0, 1e-9);
}
TEST(AbstractSlider_default_value)
{
    AbstractSlider s(0.7, 0.0, 1.0);
    CHECK_NEAR(s.defaultValue(), 0.7, 1e-9); // default seeds from the initial value

    s.setValue(0.2);
    s.resetToDefault();
    CHECK_NEAR(s.value(), 0.7, 1e-9);

    s.setDefault(2.0); // clamped into [0,1]
    CHECK_NEAR(s.defaultValue(), 1.0, 1e-9);

    s.setDefault(0.4);
    s.setRange(0.0, 0.25); // shrinking the range re-clamps the default
    CHECK_NEAR(s.defaultValue(), 0.25, 1e-9);
}
TEST(Checkbox_full)
{
    auto cb = std::make_shared<Checkbox>("Bypass");
    RecordingTarget r; cb->render(r);
    cb->onGesture({Gesture::Type::Click, {5, 5}, {5, 5}, PointerButton::Left});
    CHECK(cb->checked());
    cb->render(r); // indicator visible branch
    cb->requestFocus();
    cb->dispatchKey({KeyEvent::Type::Down, 32}); // space toggles off
    CHECK(!cb->checked());
    CHECK(!cb->dispatchKey({KeyEvent::Type::Down, 65})); // non-confirm
    cb->onGesture({Gesture::Type::Move, {5, 5}, {5, 5}, PointerButton::Left}); // default fallback
    cb->setChecked(true);
    cb->setStyle(Theme::basicTheme().checkbox);
}
TEST(TextBox_full)
{
    auto tb = std::make_shared<TextBox>();
    tb->placeholder = "type";
    RecordingTarget r; tb->render(r); // placeholder + idle (unfocused)
    tb->onGesture({Gesture::Type::Down, {5, 5}, {5, 5}, PointerButton::Left});
    CHECK(tb->hasFocus());
    tb->render(r); // focused box + caret visible
    tb->dispatchKey({KeyEvent::Type::Text, 0, "a"});
    CHECK(tb->text == "a");
    tb->render(r); // text (non-placeholder) branch
    tb->dispatchKey({KeyEvent::Type::Down, 8});  // backspace removes 'a'
    CHECK(tb->text.empty());
    tb->dispatchKey({KeyEvent::Type::Down, 8});  // backspace on empty -> fallthrough
    tb->dispatchKey({KeyEvent::Type::Down, 65}); // other key -> fallthrough
    tb->onGesture({Gesture::Type::Move, {5, 5}, {5, 5}, PointerButton::Left}); // default fallback
    tb->readOnly = true;
    CHECK(!tb->dispatchKey({KeyEvent::Type::Text, 0, "x"})); // read-only -> false
    tb->setStyle(Theme::basicTheme().textBox);
}

// ───────────────────────── Segment snap ─────────────────────────
TEST(Segment_snap_edges_and_follow)
{
    using SE = Segment::SnapEdge;
    auto aSeg = std::make_shared<Segment>();
    aSeg->x.set(10); aSeg->y.set(20); aSeg->width.set(100); aSeg->height.set(40);
    auto b = std::make_shared<Segment>();
    b->width.set(60); b->height.set(30);

    // Left of b -> Right of a + 5
    b->snapTo(aSeg.get(), SE::Left, SE::Right, 5);
    CHECK(b->hasSnap());
    b->advance(0);
    CHECK_NEAR(b->x.value(), 115.0, 1e-9); // 10+100+5
    aSeg->x.set(60); b->advance(0);
    CHECK_NEAR(b->x.value(), 165.0, 1e-9); // follows the move

    // Right of b -> Left of a  (b ends just left of a)
    b->snapTo(aSeg.get(), SE::Right, SE::Left, 0); b->advance(0);
    CHECK_NEAR(b->x.value(), aSeg->x.value() - b->width.value(), 1e-9);

    // CenterX -> CenterX
    b->snapTo(aSeg.get(), SE::CenterX, SE::CenterX, 0); b->advance(0);
    CHECK_NEAR(b->x.value() + b->width.value() * 0.5, aSeg->edgeCoord(SE::CenterX), 1e-9);

    // Vertical edges
    b->snapTo(aSeg.get(), SE::Top, SE::Bottom, 4); b->advance(0);
    CHECK_NEAR(b->y.value(), aSeg->edgeCoord(SE::Bottom) + 4, 1e-9);
    b->snapTo(aSeg.get(), SE::Bottom, SE::Top, 0); b->advance(0);
    CHECK_NEAR(b->y.value(), aSeg->y.value() - b->height.value(), 1e-9);
    b->snapTo(aSeg.get(), SE::CenterY, SE::CenterY, 0); b->advance(0);
    CHECK_NEAR(b->y.value() + b->height.value() * 0.5, aSeg->edgeCoord(SE::CenterY), 1e-9);

    // edgeCoord variants + defensive default
    CHECK_NEAR(aSeg->edgeCoord(SE::Left), aSeg->x.value(), 1e-9);
    CHECK_NEAR(aSeg->edgeCoord(SE::Top), aSeg->y.value(), 1e-9);
    CHECK_NEAR(aSeg->edgeCoord((SE)99), 0.0, 1e-9);

    // invalid myEdge -> edgeInset default + non-horizontal branch (sets y)
    b->snapTo(aSeg.get(), (SE)99, SE::Bottom, 0); b->advance(0);
    CHECK_NEAR(b->y.value(), aSeg->edgeCoord(SE::Bottom), 1e-9);

    // guards: clear, null, self are all no-ops
    b->clearSnap(); CHECK(!b->hasSnap());
    b->snapTo(nullptr, SE::Left, SE::Right, 0); CHECK(!b->hasSnap());
    b->snapTo(b.get(), SE::Left, SE::Right, 0); CHECK(!b->hasSnap());
    b->advance(0); // resolveSnap early-out with no target
}

// ───────────────────────── Row / Column layout ─────────────────────────
TEST(Row_and_Column_layout)
{
    auto mk = [](double w, double h) {
        auto s = std::make_shared<Segment>();
        s->width.set(w); s->height.set(h);
        return s;
    };

    // Row: left -> right with spacing, inset by padding; auto-sizes to content.
    auto row = std::make_shared<Row>();
    row->spacing = 10.0; row->padding = 5.0;
    auto a = mk(30, 20), b = mk(50, 40);
    row->addChild(a); row->addChild(b);
    row->advance(0);
    CHECK_NEAR(a->x.value(), 5.0, 1e-9);   // padding
    CHECK_NEAR(a->y.value(), 5.0, 1e-9);
    CHECK_NEAR(b->x.value(), 45.0, 1e-9);  // 5 + 30 + 10
    CHECK_NEAR(row->width.value(), 100.0, 1e-9);  // 5 +30+10+50 +5
    CHECK_NEAR(row->height.value(), 50.0, 1e-9);  // max(20,40) + 2*5

    // invisible children are skipped
    b->visible = false;
    row->advance(0);
    CHECK_NEAR(row->width.value(), 40.0, 1e-9); // 5 + 30 + 5

    // Column: top -> bottom
    auto col = std::make_shared<Column>();
    col->spacing = 4.0; col->padding = 2.0;
    auto c = mk(60, 12), d = mk(20, 18);
    col->addChild(c); col->addChild(d);
    col->advance(0);
    CHECK_NEAR(c->y.value(), 2.0, 1e-9);
    CHECK_NEAR(d->y.value(), 18.0, 1e-9);  // 2 + 12 + 4
    CHECK_NEAR(col->height.value(), 38.0, 1e-9); // 2 +12+4+18 +2
    CHECK_NEAR(col->width.value(), 64.0, 1e-9);  // max(60,20) + 2*2

    // empty container collapses to 2*padding on both axes
    auto empty = std::make_shared<Row>();
    empty->padding = 3.0;
    empty->advance(0);
    CHECK_NEAR(empty->width.value(), 6.0, 1e-9);
    CHECK_NEAR(empty->height.value(), 6.0, 1e-9);
}

// ───────────────────────── render/raster primitive ─────────────────────────
TEST(RecordingTarget_registerImage_records)
{
    RecordingTarget t;
    uint8_t px[2 * 2 * 4];
    for (int i = 0; i < (int)sizeof(px); ++i) px[i] = (uint8_t)i;
    int id = t.registerImage(px, 2, 2);
    CHECK(id > 0);
    CHECK(t.count(K::RegisterImage) == 1);
    const DrawOp *op = nullptr;
    for (const auto &o : t.ops()) if (o.kind == K::RegisterImage) op = &o;
    CHECK(op && op->imageId == id && op->imgW == 2 && op->imgH == 2);
    CHECK(op->pixelHash != 0);
    // a second register hands out a distinct id
    int id2 = t.registerImage(px, 2, 2);
    CHECK(id2 != id);
}

TEST(RecordingTarget_update_and_draw_records)
{
    RecordingTarget t;
    uint8_t a[2 * 2 * 4]; for (int i = 0; i < (int)sizeof(a); ++i) a[i] = 10;
    uint8_t b[2 * 2 * 4]; for (int i = 0; i < (int)sizeof(b); ++i) b[i] = 20;
    int id = t.registerImage(a, 2, 2);
    t.updateImage(id, b, 2, 2);
    t.drawImage(id, Rect{10, 20, 100, 80});

    CHECK(t.count(K::UpdateImage) == 1 && t.count(K::DrawImage) == 1);
    const DrawOp *upd = nullptr, *drw = nullptr, *reg = nullptr;
    for (const auto &o : t.ops())
    {
        if (o.kind == K::RegisterImage) reg = &o;
        if (o.kind == K::UpdateImage) upd = &o;
        if (o.kind == K::DrawImage) drw = &o;
    }
    CHECK(upd && upd->imageId == id);
    CHECK(reg && upd->pixelHash != reg->pixelHash);  // different pixels recorded
    CHECK(drw && drw->imageId == id);
    CHECK_NEAR(drw->args[0], 10, 1e-9);
    CHECK_NEAR(drw->args[1], 20, 1e-9);
    CHECK_NEAR(drw->args[2], 100, 1e-9);
    CHECK_NEAR(drw->args[3], 80, 1e-9);
}

TEST(RecordingTarget_releaseImage_records)
{
    RecordingTarget t;
    uint8_t px[4] = {1, 2, 3, 4};
    int id = t.registerImage(px, 1, 1);
    t.releaseImage(id);
    CHECK(t.count(K::ReleaseImage) == 1);
    for (const auto &o : t.ops()) if (o.kind == K::ReleaseImage) CHECK(o.imageId == id);
    // empty/zero-size register still returns an id but hashes to 0
    int z = t.registerImage(nullptr, 0, 0);
    CHECK(z > 0);
}

TEST(ImageView_fits_and_emits_drawImage)
{
    RecordingTarget t;
    ImageView v;
    v.width.set(100); v.height.set(100);
    std::vector<uint8_t> px((size_t)4 * 2 * 4, 200);  // 4x2 image
    v.setImage(px.data(), 4, 2);
    CHECK(v.hasImage() && v.imageWidth() == 4 && v.imageHeight() == 2);

    // contain: scale = min(100/4, 100/2) = 25 -> 100x50 centered at y=25
    Rect fit = v.fittedRect();
    CHECK_NEAR(fit.x, 0, 1e-9);
    CHECK_NEAR(fit.y, 25, 1e-9);
    CHECK_NEAR(fit.w, 100, 1e-9);
    CHECK_NEAR(fit.h, 50, 1e-9);

    v.render(t);
    CHECK(t.count(K::RegisterImage) == 1 && t.count(K::DrawImage) == 1);
    const DrawOp *drw = nullptr;
    for (const auto &o : t.ops()) if (o.kind == K::DrawImage) drw = &o;
    CHECK(drw && drw->imageId > 0);
    CHECK_NEAR(drw->args[1], 25, 1e-9);
    CHECK_NEAR(drw->args[3], 50, 1e-9);
}

TEST(ImageView_reupload_on_change_and_fits)
{
    RecordingTarget t;
    ImageView v;
    v.width.set(40); v.height.set(40);
    std::vector<uint8_t> px((size_t)2 * 2 * 4, 100);
    v.setImage(px.data(), 2, 2);
    v.render(t);  // registers
    v.setImage(px.data(), 2, 2);  // same dims, new pixels -> dirty
    v.render(t);  // should UPDATE, not register again
    CHECK(t.count(K::RegisterImage) == 1);
    CHECK(t.count(K::UpdateImage) == 1);
    CHECK(t.count(K::DrawImage) == 2);

    // Cover fit: scale = max(40/2,40/2)=20 -> 40x40 (square here)
    v.setFit(ImageView::Fit::Cover);
    Rect cover = v.fittedRect();
    CHECK_NEAR(cover.w, 40, 1e-9);
    // Fill fit: exactly the bounds
    v.setFit(ImageView::Fit::Fill);
    Rect fill = v.fittedRect();
    CHECK_NEAR(fill.w, 40, 1e-9);
    CHECK_NEAR(fill.h, 40, 1e-9);
}

TEST(ImageView_empty_and_clear)
{
    RecordingTarget t;
    ImageView v;
    v.width.set(50); v.height.set(50);
    v.render(t);  // no image -> nothing
    CHECK(t.count(K::DrawImage) == 0);
    CHECK_NEAR(v.fittedRect().w, 0, 1e-9);

    std::vector<uint8_t> px((size_t)1 * 1 * 4, 255);
    v.setImage(px.data(), 1, 1);
    CHECK(v.hasImage());
    v.clearImage();
    CHECK(!v.hasImage());
    v.setImage(nullptr, 0, 0);  // invalid clears too
    CHECK(!v.hasImage());
}

TEST(ImageView_zoom_about_point)
{
    ImageView v;
    v.width.set(100); v.height.set(100);
    std::vector<uint8_t> px((size_t)100 * 100 * 4, 200);
    v.setImage(px.data(), 100, 100);
    CHECK_NEAR(v.fittedRect().w, 100.0, 1e-9);  // 1x fills the square view

    v.zoomAbout(2.0, {50, 50});                 // zoom 2x about the centre
    CHECK_NEAR(v.zoom(), 2.0, 1e-9);
    Rect f = v.fittedRect();
    CHECK_NEAR(f.w, 200.0, 1e-9);
    // the centre point stays fixed: fitted.x + 0.5*w == 50
    CHECK_NEAR(f.x + 0.5 * f.w, 50.0, 1e-6);
    // pan is clamped so the image still covers the 100x100 view
    CHECK(f.x <= 1e-9 && f.x + f.w >= 100.0 - 1e-9);

    v.zoomAbout(0.1, {50, 50});                 // clamps to the 1x minimum
    CHECK_NEAR(v.zoom(), 1.0, 1e-9);
    CHECK_NEAR(v.fittedRect().x, 0.0, 1e-9);    // reset pan at 1x

    // draw is clipped to the view (a zoomed image must not spill out)
    v.zoomAbout(3.0, {20, 20});
    RecordingTarget t; v.render(t);
    CHECK(t.count(K::ClipRect) >= 1);
    CHECK(t.count(K::DrawImage) == 1);

    // when zoomed, the view is interactive and a drag pans it
    v.resetView();
    v.zoomAbout(2.0, {50, 50});             // 2x about the centre -> fitted {-50,-50,200,200}
    CHECK(v.hitTest({50, 50}));
    const double panX0 = v.fittedRect().x;  // -50
    v.onGesture({Gesture::Type::Down, {50, 50}, {50, 50}, PointerButton::Left});
    v.onGesture({Gesture::Type::Drag, {70, 50}, {50, 50}, PointerButton::Left});  // drag right +20
    Rect pf = v.fittedRect();
    CHECK(pf.x > panX0 + 1.0);              // image panned right
    CHECK(pf.x <= 1e-9 && pf.x + pf.w >= 100.0 - 1e-9);  // still covers the view

    v.resetView();
    CHECK_NEAR(v.zoom(), 1.0, 1e-9);
    CHECK(!v.hitTest({50, 50}));            // display-only again at 1x
    v.onGesture({Gesture::Type::Drag, {70, 50}, {50, 50}, PointerButton::Left});  // ignored at 1x
    CHECK_NEAR(v.fittedRect().x, 0.0, 1e-9);
    v.panBy(10, 0);                          // panBy is a no-op at 1x
    CHECK_NEAR(v.fittedRect().x, 0.0, 1e-9);
}

// ───────────────────────── ui/base/Observable ─────────────────────────
TEST(Observable_default_and_set_notifies_only_on_change)
{
    Observable<int> d;                        // default ctor -> value-initialised
    CHECK(d.get() == 0);

    Observable<int> s(1);
    int fires = 0, last = -1;
    s.observe([&](const int &v) { ++fires; last = v; }, /*fireNow*/ false);
    s.set(1);                                 // same value -> no notify
    CHECK(fires == 0);
    s.set(5);                                 // changed -> notify observers
    CHECK(fires == 1);
    CHECK(last == 5);
    CHECK(s.get() == 5);
}

TEST(Observable_observe_fires_now_and_links_views)
{
    // Two views bound to one boolean state (a toggle button's `active` and the
    // panel it shows) stay in sync -- the state-link use case that prevents the
    // "button highlighted but panel hidden" class of bug.
    Observable<bool> railOpen(true);
    bool buttonActive = false, panelVisible = false;
    railOpen.observe([&](const bool &v) { buttonActive = v; });   // fireNow -> initialise in sync
    railOpen.observe([&](const bool &v) { panelVisible = v; });
    CHECK(buttonActive == true);              // both initialised from the current value
    CHECK(panelVisible == true);
    CHECK(railOpen.observerCount() == 2);
    railOpen.set(false);                      // one flip updates every observer
    CHECK(buttonActive == false);
    CHECK(panelVisible == false);
}

TEST(Observable_null_observer_ignored)
{
    Observable<int> s(0);
    s.observe(nullptr);                       // null -> ignored, no crash, does not fire
    s.observe(Observable<int>::Observer{}, false);
    CHECK(s.observerCount() == 0);
    s.set(3);                                 // no observers -> still fine
    CHECK(s.get() == 3);
}

// ───────────────────────── hover + animated state (FR-24 / FR-25) ─────────────────────────
static bool sameColor(const Color &a, const Color &b)
{
    return std::fabs(a.r - b.r) < 1e-9 && std::fabs(a.g - b.g) < 1e-9 &&
           std::fabs(a.b - b.b) < 1e-9 && std::fabs(a.a - b.a) < 1e-9;
}
static Color firstFill(const RecordingTarget &t)
{
    for (const auto &o : t.ops())
        if (o.kind == K::SetFill)
            return o.color;
    return Color{};
}
static bool anyFillDiffers(const RecordingTarget &x, const RecordingTarget &y)
{
    std::vector<Color> fx, fy;
    for (const auto &o : x.ops()) if (o.kind == K::SetFill) fx.push_back(o.color);
    for (const auto &o : y.ops()) if (o.kind == K::SetFill) fy.push_back(o.color);
    if (fx.size() != fy.size()) return true;
    for (size_t i = 0; i < fx.size(); ++i) if (!sameColor(fx[i], fy[i])) return true;
    return false;
}
// Order-weighted signature of the path geometry: sensitive to WHICH vertex moved
// (a plain coordinate sum is invariant under reordering, so it would miss a morph
// between two series with the same total).
static double geomSig(const RecordingTarget &t)
{
    double s = 0.0;
    int k = 1;
    for (const auto &o : t.ops())
        if (o.kind == K::LineTo || o.kind == K::MoveTo)
        {
            s += (o.args[0] + o.args[1] * 3.0) * k;
            ++k;
        }
    return s;
}

TEST(Interaction_color_and_box_helpers)
{
    const Color black = Color::rgba(0, 0, 0), white = Color::rgba(255, 255, 255);
    CHECK_NEAR(lerpColor(black, white, 0.5).r, 0.5, 1e-9);
    Color br = brighten(Color::rgba(0, 0, 0, 128), 0.5);
    CHECK_NEAR(br.r, 0.5, 1e-9);
    CHECK_NEAR(br.a, 128 / 255.0, 1e-9);          // alpha preserved

    Paint fs = lerpPaint(Paint::filledStroked(black, black, 2.0),
                         Paint::filledStroked(white, white, 4.0), 0.5);
    CHECK(fs.hasFill && fs.hasStroke);
    CHECK_NEAR(fs.strokeWidth, 3.0, 1e-9);
    Paint fo = lerpPaint(Paint::filled(black), Paint::filled(white), 0.5);
    CHECK(fo.hasFill && !fo.hasStroke);           // stroke branch skipped
    Paint so = lerpPaint(Paint::stroked(black, 1.0), Paint::stroked(white, 3.0), 0.5);
    CHECK(!so.hasFill && so.hasStroke);           // fill branch skipped

    BoxStyle bA{Paint::filledStroked(black, black, 1.0), 4.0};
    BoxStyle bB{Paint::filledStroked(white, white, 1.0), 8.0};
    CHECK_NEAR(lerpBox(bA, bB, 0.5).cornerRadius, 6.0, 1e-9);

    BoxStyle hs = hoverBox(bA, white, 1.0);
    CHECK(hs.paint.hasStroke && !sameColor(hs.paint.fill, bA.paint.fill));   // brightened + stroke pulled
    BoxStyle hf = hoverBox(BoxStyle{Paint::filled(black), 0.0}, white, 1.0);
    CHECK(!hf.paint.hasStroke);                                              // no-stroke branch
    CHECK(sameColor(hoverBox(bA, white, 0.0).paint.fill, bA.paint.fill));    // t=0 is identity
}

TEST(Segment_hover_routing_and_owner)
{
    auto root = std::make_shared<Segment>(); root->width.set(200); root->height.set(100);
    auto a = std::make_shared<Segment>(); a->width.set(50); a->height.set(50);
    auto b = std::make_shared<Segment>(); b->width.set(50); b->height.set(50); b->x.set(100);
    root->addChild(a); root->addChild(b);

    root->onGesture({GT::Move, {10, 10}, {10, 10}, PB::Left});     // hover a
    CHECK(a->isHovered() && !b->isHovered());
    CHECK(Segment::hoveredSegment() == a.get());
    CHECK(root->isHoverWithin() && a->isHoverWithin() && !b->isHoverWithin());

    root->onGesture({GT::Move, {110, 10}, {110, 10}, PB::Left});   // hover b, leave a
    CHECK(b->isHovered() && !a->isHovered());

    root->onGesture({GT::Move, {10, 80}, {10, 80}, PB::Left});     // over root only (gap)
    CHECK(root->isHovered() && !a->isHovered() && !b->isHovered());

    Segment::setHovered(nullptr);                                  // clear (seg==null path)
    CHECK(Segment::hoveredSegment() == nullptr && !root->isHoverWithin());
    Segment::setHovered(a.get());
    Segment::setHovered(a.get());                                  // same -> early return
    CHECK(a->isHovered());
    Segment::setHovered(nullptr);
}

TEST(Segment_hover_move_during_press_and_destructor)
{
    auto root = std::make_shared<Segment>(); root->width.set(100); root->height.set(100);
    auto a = std::make_shared<Segment>(); a->width.set(100); a->height.set(100);
    root->addChild(a);
    root->onGesture({GT::Down, {10, 10}, {10, 10}, PB::Left});     // captures a
    root->onGesture({GT::Move, {12, 12}, {10, 10}, PB::Left});     // move during press -> captured a
    CHECK(a->isHovered());
    root->onGesture({GT::Up, {12, 12}, {10, 10}, PB::Left});

    {
        auto tmp = std::make_shared<Segment>();
        Segment::setHovered(tmp.get());
        CHECK(Segment::hoveredSegment() == tmp.get());
    }
    CHECK(Segment::hoveredSegment() == nullptr);                   // destructor relinquished hover

    auto hidden = std::make_shared<Segment>(); hidden->visible = false;
    RecordingTarget rt; hidden->renderOverlay(rt);                 // !visible overlay early-out
    CHECK(rt.ops().empty());
}

TEST(Segment_hover_amount_animates)
{
    auto btn = std::make_shared<Button>();
    btn->onGesture({GT::Move, {5, 5}, {5, 5}, PB::Left});
    CHECK(btn->isHovered());
    btn->advance(0.0);                                             // baseline + animateTo(1)
    CHECK_NEAR(btn->hoverAmount(), 0.0, 1e-6);
    btn->advance(60.0);                                            // no-change path + mid value
    CHECK(btn->hoverAmount() > 0.1 && btn->hoverAmount() < 1.0);
    btn->advance(200.0);
    CHECK_NEAR(btn->hoverAmount(), 1.0, 1e-6);
    Segment::setHovered(nullptr);
    btn->advance(260.0);                                           // animateTo(0)
    btn->advance(500.0);
    CHECK_NEAR(btn->hoverAmount(), 0.0, 1e-6);
    btn->advance(560.0);                                           // steady no-change path
    CHECK_NEAR(btn->hoverAmount(), 0.0, 1e-6);
}

TEST(Button_press_and_hover_animate)
{
    auto b = std::make_shared<Button>();
    RecordingTarget idle; b->render(idle);
    const Color idleFill = firstFill(idle);
    b->onGesture({GT::Down, {5, 5}, {5, 5}, PB::Left});
    for (int i = 0; i <= 20; ++i) b->advance(i * 10.0);            // press crossfades to pressed
    RecordingTarget pressed; b->render(pressed);
    CHECK(!sameColor(firstFill(pressed), idleFill));
    b->onGesture({GT::Click, {5, 5}, {5, 5}, PB::Left});
    b->onGesture({GT::Up, {5, 5}, {5, 5}, PB::Left});
    for (int i = 21; i <= 45; ++i) b->advance(i * 10.0);
    RecordingTarget released; b->render(released);
    CHECK(sameColor(firstFill(released), idleFill));              // back to idle (not hovered)
    b->onGesture({GT::Move, {5, 5}, {5, 5}, PB::Left});
    for (int i = 46; i <= 70; ++i) b->advance(i * 10.0);
    RecordingTarget hov; b->render(hov);
    CHECK(!sameColor(firstFill(hov), idleFill));                  // hover nudges body toward pressed
    b->onGesture({GT::Drop, {5, 5}, {5, 5}, PB::Left});           // cover Drop branch
    b->advance(800.0);
}

TEST(Checkbox_check_grows_and_hover)
{
    auto c = std::make_shared<Checkbox>("x");
    RecordingTarget off; c->render(off);
    CHECK(off.count(K::FillPath) == 1);                           // box only (indicator size 0)
    c->onGesture({GT::Click, {5, 5}, {5, 5}, PB::Left});          // toggle -> animate in
    RecordingTarget preAdv; c->render(preAdv);
    CHECK(preAdv.count(K::FillPath) == 1);                        // still hidden pre-advance (no pop)
    for (int i = 0; i <= 20; ++i) c->advance(i * 10.0);
    RecordingTarget on; c->render(on);
    CHECK(on.count(K::FillPath) >= 2);                            // box + grown indicator
    c->onGesture({GT::Move, {5, 5}, {5, 5}, PB::Left});
    for (int i = 21; i <= 45; ++i) c->advance(i * 10.0);
    RecordingTarget hov; c->render(hov);
    CHECK(!sameColor(firstFill(hov), firstFill(off)));           // box brightened on hover
    c->setChecked(false);                                        // programmatic snap
    RecordingTarget cleared; c->render(cleared);
    CHECK(cleared.count(K::FillPath) == 1);
}

TEST(ToggleSwitch_hover_brightens_track)
{
    auto sw = std::make_shared<ToggleSwitch>();
    RecordingTarget idle; sw->render(idle);
    sw->onGesture({GT::Move, {5, 5}, {5, 5}, PB::Left});
    for (int i = 0; i <= 20; ++i) sw->advance(i * 10.0);
    RecordingTarget hov; sw->render(hov);
    CHECK(!sameColor(firstFill(hov), firstFill(idle)));          // track brightened on hover
}

TEST(Slider_hover_grows_thumb)
{
    auto s = std::make_shared<Slider>(); s->setValue(0.5);
    for (int i = 0; i <= 12; ++i) s->advance(i * 16.0);           // settle display
    RecordingTarget idle; s->render(idle);
    s->onGesture({GT::Move, {80, 14}, {80, 14}, PB::Left});
    for (int i = 13; i <= 45; ++i) s->advance(i * 16.0);
    RecordingTarget hov; s->render(hov);
    CHECK(anyFillDiffers(idle, hov));                            // thumb brightened/grown
}

TEST(Knob_hover_brightens_dial)
{
    auto k = std::make_shared<Knob>(); k->setValue(0.5);
    for (int i = 0; i <= 12; ++i) k->advance(i * 16.0);
    RecordingTarget idle; k->render(idle);
    k->onGesture({GT::Move, {32, 32}, {32, 32}, PB::Left});
    for (int i = 13; i <= 45; ++i) k->advance(i * 16.0);
    RecordingTarget hov; k->render(hov);
    CHECK(anyFillDiffers(idle, hov));
}

TEST(ComboBox_row_hover_glides)
{
    auto c = std::make_shared<ComboBox>();
    c->setOptions({"a", "b", "c"});
    c->advance(0.0);
    c->onGesture({GT::Move, {20, 70}, {20, 70}, PB::Left});       // closed -> row -1 branch
    c->onGesture({GT::Click, {5, 5}, {5, 5}, PB::Left});          // open
    CHECK(c->isOpen());
    c->onGesture({GT::Move, {20, 70}, {20, 70}, PB::Left});       // hover row 1 (open)
    for (int i = 1; i <= 45; ++i) c->advance(i * 16.0);           // reveal + highlight settle
    RecordingTarget ovHi; c->renderOverlay(ovHi);
    RecordingTarget field; c->render(field);
    (void)field;
    c->onGesture({GT::Move, {20, 10}, {20, 10}, PB::Left});       // back onto field -> row -1
    for (int i = 46; i <= 90; ++i) c->advance(i * 16.0);
    RecordingTarget ovNone; c->renderOverlay(ovNone);
    CHECK(ovHi.count(K::FillPath) > ovNone.count(K::FillPath));   // gliding highlight appeared then faded
}

TEST(TabView_select_animates_and_hover)
{
    TabStyle st = Theme::basicTheme().tab;
    st.activeIndicatorColor = Color::rgba(255, 0, 0);
    st.activeIndicatorHeight = 3.0;
    auto tv = std::make_shared<TabView>(st);
    tv->addPage("one", std::make_shared<Segment>());
    tv->addPage("two", std::make_shared<Segment>());
    tv->advance(0.0);
    RecordingTarget sel0; tv->render(sel0);
    tv->setSelectedIndex(1);
    for (int i = 1; i <= 30; ++i) tv->advance(i * 16.0);
    RecordingTarget sel1; tv->render(sel1);
    CHECK(std::fabs(geomSig(sel0) - geomSig(sel1)) > 1e-6);       // active tab geometry eased to tab 1
    tv->onGesture({GT::Move, {10, 10}, {10, 10}, PB::Left});      // hover tab 0 (on strip)
    for (int i = 31; i <= 60; ++i) tv->advance(i * 16.0);
    RecordingTarget hov; tv->render(hov);
    CHECK(anyFillDiffers(sel1, hov));
    tv->onGesture({GT::Move, {10, 100}, {10, 100}, PB::Left});    // off strip -> hover tab -1
    for (int i = 61; i <= 90; ++i) tv->advance(i * 16.0);
}

TEST(ScrollView_scrollbar_hover)
{
    auto sv = std::make_shared<ScrollView>();
    sv->width.set(100); sv->height.set(100);
    auto content = std::make_shared<Segment>(); content->width.set(100); content->height.set(300);
    sv->setContent(content);
    sv->setContentHeight(300);
    sv->advance(0.0);
    RecordingTarget idle; sv->render(idle);
    sv->onGesture({GT::Move, {50, 50}, {50, 50}, PB::Left});      // hover the content
    CHECK(sv->isHoverWithin());
    for (int i = 1; i <= 30; ++i) sv->advance(i * 16.0);
    RecordingTarget hov; sv->render(hov);
    CHECK(anyFillDiffers(idle, hov));                            // scrollbar emphasised on hover
}

TEST(TextBox_focus_and_caret_animate)
{
    auto tb = std::make_shared<TextBox>();
    RecordingTarget blur; tb->render(blur);
    const int blurFills = blur.count(K::FillPath);
    tb->onGesture({GT::Down, {5, 5}, {5, 5}, PB::Left});          // focus
    CHECK(tb->hasFocus());
    for (int i = 0; i <= 20; ++i) tb->advance(i * 10.0);          // focus border + caret fade in
    RecordingTarget foc; tb->render(foc);
    CHECK(foc.count(K::FillPath) > blurFills);                    // caret now drawn
    auto other = std::make_shared<TextBox>();
    other->requestFocus();                                       // steal focus -> tb blurs
    CHECK(!tb->hasFocus());
    for (int i = 21; i <= 60; ++i) tb->advance(i * 10.0);
    RecordingTarget after; tb->render(after);
    CHECK(after.count(K::FillPath) == blurFills);                // caret faded out (no pop)
}

TEST(LineGraph_series_morph)
{
    auto g = std::make_shared<LineGraph>();
    g->setRange(0.0, 1.0);
    g->setSeries({0.2, 0.4, 0.6, 0.8, 0.5});                     // 0 -> 5 : assign+reset
    g->advance(0.0);
    RecordingTarget r0; g->render(r0);
    const int strokes5 = r0.count(K::StrokePath);
    g->setSeries({0.8, 0.6, 0.4, 0.2, 0.5});                     // 5 -> 5 : setTarget (morph)
    RecordingTarget mid; g->render(mid);                         // pre-advance: still old shape
    for (int i = 1; i <= 30; ++i) g->advance(i * 16.0);
    RecordingTarget settled; g->render(settled);
    CHECK(std::fabs(geomSig(mid) - geomSig(settled)) > 1e-6);    // series morphed
    g->setSeries({0.5});                                         // 5 -> 1 : polyline skipped
    RecordingTarget one; g->render(one);
    CHECK(one.count(K::StrokePath) < strokes5);
    g->setSeries({});                                            // 1 -> 0 : empty
    RecordingTarget none; g->render(none);
    CHECK(none.count(K::StrokePath) == one.count(K::StrokePath));
}

// measureText: the read side of drawText. RecordingTarget (headless) uses the base
// IRenderTarget estimate — ~0.5em per UTF-8 codepoint + letterSpacing between glyphs.
// (Real adapters — Cairo/Canvas2D — override this with their text stack's metrics.)
TEST(RenderTarget_measureText_headless_estimate)
{
    RecordingTarget t;
    CHECK_NEAR(t.measureText("cosmo", 10.0), 25.0, 1e-9);            // 5 * 10 * 0.5
    CHECK_NEAR(t.measureText("cosmo", 10.0, "DM Sans", 2.0), 33.0, 1e-9);  // + 4 gaps * 2
    CHECK_NEAR(t.measureText("", 10.0), 0.0, 1e-9);
    CHECK(t.measureText("AB", 20.0) > t.measureText("A", 20.0));     // grows with length
}

int main() { return mini::runAll(); }
