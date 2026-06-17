#include "MiniTest.h"
#include "../include/artboard/artboard.h"
#include <cmath>

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
        Easing::EaseInBounce, Easing::EaseOutBounce, Easing::EaseInOutBounce};
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
    root->onGesture({Gesture::Type::Down, {60, 20}, {60, 20}, PointerButton::Left});
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
    CHECK(slider->childCount() == 3);
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
    CHECK_NEAR(p->value(), 0.5, 1e-9);
    RecordingTarget mid; p->render(mid);
    CHECK(mid.count(K::FillPath) >= 2); // track + fill

    p->setValue(-1); CHECK_NEAR(p->value(), 0.0, 1e-9); // clamp low
    RecordingTarget zero; p->render(zero);              // no-fill branch
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
    RecordingTarget open; c->render(open); // popup + rows + selected highlight
    CHECK(open.count(K::DrawText) >= 4);   // field + 3 rows

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
    sv->onGesture({Gesture::Type::Move, {50, 50}, {50, 50}, PointerButton::Left}); // non-drag fallback

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
    sl->onGesture({Gesture::Type::Down, {40, 14}, {40, 14}, PointerButton::Left});  // value from x
    CHECK(sl->value() > 0.0);
    sl->onGesture({Gesture::Type::Drag, {80, 14}, {80, 14}, PointerButton::Left});
    sl->onGesture({Gesture::Type::Click, {160, 14}, {160, 14}, PointerButton::Left});
    sl->onGesture({Gesture::Type::Click, {-10, 14}, {-10, 14}, PointerButton::Left}); // clamp low
    sl->onGesture({Gesture::Type::Click, {200, 14}, {200, 14}, PointerButton::Left});  // clamp high
    sl->onGesture({Gesture::Type::Move, {10, 10}, {10, 10}, PointerButton::Left}); // default fallback
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
    sl0->onGesture({Gesture::Type::Down, {5, 5}, {5, 5}, PointerButton::Left});
    CHECK_NEAR(sl0->value(), sl0->minimum(), 1e-9);
}
TEST(AbstractSlider_clamp_reversed_range)
{
    AbstractSlider s(0.0, 5.0, 1.0); // max < min -> clamp returns mMin
    CHECK_NEAR(s.value(), 5.0, 1e-9);
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

int main() { return mini::runAll(); }
